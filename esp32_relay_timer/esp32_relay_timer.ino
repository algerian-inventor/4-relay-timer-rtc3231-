#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

#include "config.h"
#include "model.h"
#include "storage.h"
#include "event_log.h"
#include "button_controller.h"
#include "display_controller.h"
#include "web_ui.h"

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
WebServer server(80);
DNSServer dnsServer;

PersistedSettings settings;
EventLog eventLog;
ButtonController buttons;
DisplayController display(lcd);

bool rtcReady = false;
unsigned long bootMs = 0;

static String twoDigits(uint8_t value) {
  return value < 10 ? "0" + String(value) : String(value);
}

static String timestampNow() {
  if (!rtcReady) return "RTC unavailable";
  DateTime now = rtc.now();
  return twoDigits(now.hour()) + ":" + twoDigits(now.minute()) + ":" + twoDigits(now.second());
}

static void beep(uint16_t ms = 45) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(ms);
  digitalWrite(BUZZER_PIN, LOW);
}

static void writeRelayPin(uint8_t index, bool on) {
  const uint8_t level = (on == RELAY_ACTIVE_HIGH) ? HIGH : LOW;
  digitalWrite(RELAY_PINS[index], level);
}

static void setRelayState(uint8_t index, bool on, const char* reason) {
  if (index >= CHANNEL_COUNT) return;
  RelayChannel& channel = settings.channels[index];
  if (channel.outputState == on) return;

  writeRelayPin(index, on);
  channel.outputState = on;

  String message = String(channel.name) + (on ? " turned ON" : " turned OFF");
  if (reason && strlen(reason)) message += " • " + String(reason);
  eventLog.add(timestampNow(), message);
}

static void updateRelays() {
  uint8_t day = 0;
  uint16_t minuteOfDay = 0;

  if (rtcReady) {
    DateTime now = rtc.now();
    day = now.dayOfTheWeek();
    minuteOfDay = now.hour() * 60 + now.minute();
  }

  for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
    RelayChannel& channel = settings.channels[i];
    bool desired = false;
    const char* reason = "manual";

    if (channel.mode == RelayMode::FORCE_ON) {
      desired = true;
    } else if (channel.mode == RelayMode::FORCE_OFF) {
      desired = false;
    } else {
      desired = rtcReady && autoShouldRun(channel, day, minuteOfDay);
      reason = "schedule";
    }

    setRelayState(i, desired, reason);
  }
}

static const char* modeName(RelayMode mode) {
  switch (mode) {
    case RelayMode::FORCE_ON: return "ON";
    case RelayMode::FORCE_OFF: return "OFF";
    default: return "AUTO";
  }
}

static String minuteText(uint16_t value) {
  return twoDigits(value / 60) + ":" + twoDigits(value % 60);
}

static void addChannelJson(JsonObject obj, uint8_t index) {
  const RelayChannel& channel = settings.channels[index];
  obj["index"] = index;
  obj["name"] = channel.name;
  obj["mode"] = static_cast<uint8_t>(channel.mode);
  obj["modeName"] = modeName(channel.mode);
  obj["state"] = channel.outputState;

  JsonArray schedules = obj["schedules"].to<JsonArray>();
  for (uint8_t s = 0; s < MAX_SCHEDULES; ++s) {
    const ScheduleRule& rule = channel.schedules[s];
    JsonObject row = schedules.add<JsonObject>();
    row["slot"] = s;
    row["enabled"] = rule.enabled;
    row["daysMask"] = rule.daysMask;
    row["startMinute"] = rule.startMinute;
    row["endMinute"] = rule.endMinute;
    row["start"] = minuteText(rule.startMinute);
    row["end"] = minuteText(rule.endMinute);
  }
}

static String statusJson() {
  JsonDocument doc;
  doc["device"] = DEVICE_NAME;
  doc["firmware"] = FIRMWARE_VERSION;
  doc["schema"] = SETTINGS_SCHEMA_VERSION;
  doc["ip"] = WiFi.softAPIP().toString();
  doc["ssid"] = AP_SSID;
  doc["clients"] = WiFi.softAPgetStationNum();
  doc["uptimeSec"] = millis() / 1000;
  doc["rtcReady"] = rtcReady;

  if (rtcReady) {
    DateTime now = rtc.now();
    doc["time"] = twoDigits(now.hour()) + ":" + twoDigits(now.minute()) + ":" + twoDigits(now.second());
    doc["date"] = twoDigits(now.day()) + "/" + twoDigits(now.month()) + "/" + String(now.year());
    doc["temperature"] = rtc.getTemperature();
    doc["dayOfWeek"] = now.dayOfTheWeek();
  } else {
    doc["time"] = "--:--:--";
    doc["date"] = "RTC unavailable";
    doc["temperature"] = 0;
    doc["dayOfWeek"] = 0;
  }

  JsonArray channels = doc["channels"].to<JsonArray>();
  for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
    addChannelJson(channels.add<JsonObject>(), i);
  }

  String out;
  serializeJson(doc, out);
  return out;
}

static String eventsJson() {
  JsonDocument doc;
  JsonArray rows = doc["events"].to<JsonArray>();

  for (uint8_t i = 0; i < eventLog.count(); ++i) {
    const EventRecord& event = eventLog.newest(i);
    JsonObject row = rows.add<JsonObject>();
    row["time"] = event.time;
    row["message"] = event.message;
  }

  String out;
  serializeJson(doc, out);
  return out;
}

static bool readJson(JsonDocument& doc) {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"missing_body\"}");
    return false;
  }

  const DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    server.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return false;
  }

  return true;
}

static bool validChannelIndex(int value) {
  return value >= 0 && value < CHANNEL_COUNT;
}

static void sendOk() {
  server.send(200, "application/json", "{\"ok\":true}");
}

static void setupApi() {
  server.on("/api/status", HTTP_GET, []() {
    server.send(200, "application/json", statusJson());
  });

  server.on("/api/events", HTTP_GET, []() {
    server.send(200, "application/json", eventsJson());
  });

  server.on("/api/channel/mode", HTTP_POST, []() {
    JsonDocument doc;
    if (!readJson(doc)) return;

    const int channelIndex = doc["channel"] | -1;
    const int mode = doc["mode"] | -1;

    if (!validChannelIndex(channelIndex) || mode < 0 || mode > 2) {
      server.send(400, "application/json", "{\"error\":\"invalid_request\"}");
      return;
    }

    settings.channels[channelIndex].mode = static_cast<RelayMode>(mode);
    Storage::save(settings);
    eventLog.add(timestampNow(), String(settings.channels[channelIndex].name) + " mode → " + modeName(settings.channels[channelIndex].mode));
    updateRelays();
    sendOk();
  });

  server.on("/api/channel/name", HTTP_POST, []() {
    JsonDocument doc;
    if (!readJson(doc)) return;

    const int channelIndex = doc["channel"] | -1;
    const String name = doc["name"] | "";

    if (!validChannelIndex(channelIndex) || name.length() < 1 || name.length() > 23) {
      server.send(400, "application/json", "{\"error\":\"invalid_name\"}");
      return;
    }

    strlcpy(settings.channels[channelIndex].name, name.c_str(), sizeof(settings.channels[channelIndex].name));
    Storage::save(settings);
    eventLog.add(timestampNow(), "Channel " + String(channelIndex + 1) + " renamed to " + name);
    sendOk();
  });

  server.on("/api/schedule", HTTP_POST, []() {
    JsonDocument doc;
    if (!readJson(doc)) return;

    const int channelIndex = doc["channel"] | -1;
    const int slot = doc["slot"] | -1;
    const int startMinute = doc["startMinute"] | -1;
    const int endMinute = doc["endMinute"] | -1;
    const int daysMask = doc["daysMask"] | -1;
    const bool enabled = doc["enabled"] | false;

    if (!validChannelIndex(channelIndex) ||
        slot < 0 || slot >= MAX_SCHEDULES ||
        startMinute < 0 || startMinute >= 1440 ||
        endMinute < 0 || endMinute >= 1440 ||
        daysMask < 0 || daysMask > 127) {
      server.send(400, "application/json", "{\"error\":\"invalid_schedule\"}");
      return;
    }

    ScheduleRule& rule = settings.channels[channelIndex].schedules[slot];
    rule.enabled = enabled;
    rule.daysMask = static_cast<uint8_t>(daysMask);
    rule.startMinute = static_cast<uint16_t>(startMinute);
    rule.endMinute = static_cast<uint16_t>(endMinute);

    Storage::save(settings);
    eventLog.add(timestampNow(), String(settings.channels[channelIndex].name) + " schedule " + String(slot + 1) + " updated");
    updateRelays();
    sendOk();
  });

  server.on("/api/time", HTTP_POST, []() {
    JsonDocument doc;
    if (!readJson(doc)) return;

    const int year = doc["year"] | 0;
    const int month = doc["month"] | 0;
    const int day = doc["day"] | 0;
    const int hour = doc["hour"] | -1;
    const int minute = doc["minute"] | -1;
    const int second = doc["second"] | -1;

    if (!rtcReady || year < 2024 || year > 2099 ||
        month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
      server.send(400, "application/json", "{\"error\":\"invalid_time\"}");
      return;
    }

    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    eventLog.add(timestampNow(), "RTC synchronized from phone");
    sendOk();
  });

  server.on("/api/factory-reset", HTTP_POST, []() {
    JsonDocument doc;
    if (!readJson(doc)) return;

    const bool confirm = doc["confirm"] | false;
    if (!confirm) {
      server.send(400, "application/json", "{\"error\":\"confirmation_required\"}");
      return;
    }

    for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
      writeRelayPin(i, false);
    }

    Storage::factoryReset(settings);
    eventLog.clear();
    eventLog.add(timestampNow(), "Factory settings restored");
    updateRelays();
    sendOk();
  });
}

static void redirectToDashboard() {
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
  server.send(302, "text/plain", "");
}

static void setupWeb() {
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  setupApi();

  server.on("/generate_204", HTTP_ANY, redirectToDashboard);
  server.on("/gen_204", HTTP_ANY, redirectToDashboard);
  server.on("/hotspot-detect.html", HTTP_ANY, redirectToDashboard);
  server.on("/library/test/success.html", HTTP_ANY, redirectToDashboard);
  server.on("/ncsi.txt", HTTP_ANY, redirectToDashboard);
  server.on("/connecttest.txt", HTTP_ANY, redirectToDashboard);
  server.onNotFound(redirectToDashboard);

  server.begin();
}

static void setupAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  const IPAddress ip(192, 168, 4, 1);
  const IPAddress gateway(192, 168, 4, 1);
  const IPAddress subnet(255, 255, 255, 0);

  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, false, AP_MAX_CLIENTS);
  dnsServer.start(53, "*", ip);
}

static void handlePhysicalControls() {
  const ButtonAction action = buttons.poll();
  if (action == ButtonAction::NONE) return;

  uint8_t selected = display.selected();
  RelayChannel& channel = settings.channels[selected];

  if (action == ButtonAction::TIMER) {
    display.nextChannel();
    beep();
    return;
  }

  if (action == ButtonAction::UP) channel.mode = RelayMode::FORCE_ON;
  if (action == ButtonAction::DOWN) channel.mode = RelayMode::FORCE_OFF;
  if (action == ButtonAction::CLOCK || action == ButtonAction::BACK) channel.mode = RelayMode::AUTO;

  Storage::save(settings);
  eventLog.add(timestampNow(), String(channel.name) + " mode → " + modeName(channel.mode) + " (panel)");
  updateRelays();
  beep();
}

void setup() {
  bootMs = millis();
  Serial.begin(115200);
  delay(100);

  for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
    pinMode(RELAY_PINS[i], OUTPUT);
    writeRelayPin(i, false); // Safe boot: all channels off before loading configuration.
  }

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  buttons.begin();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  display.begin();
  display.showBoot();

  rtcReady = rtc.begin();

  Storage::begin(settings);

  // Runtime state is never trusted from persisted bytes.
  for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
    settings.channels[i].outputState = false;
  }

  setupAccessPoint();
  setupWeb();

  eventLog.add(timestampNow(), "Controller started");
  if (!rtcReady) eventLog.add("boot", "DS3231 not detected");

  updateRelays();

  Serial.println();
  Serial.println(DEVICE_NAME);
  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("Dashboard: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  handlePhysicalControls();
  updateRelays();
  display.refresh(rtcReady, rtc, settings.channels);

  delay(2);
}
