#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <RTClib.h>
#include <ArduinoJson.h>

#include "config.h"
#include "model.h"
#include "storage.h"
#include "event_log.h"
#include "web_ui.h"

RTC_DS3231 rtc;
WebServer server(80);
DNSServer dnsServer;

PersistedSettings settings;
EventLog eventLog;

bool rtcReady = false;
bool rtcValid = false;
bool apReady = false;
bool bellRinging = false;

unsigned long bellOffAtMs = 0;
uint32_t lastTriggerStamp = 0;
uint8_t lastTriggerSlot = 0xFF;

static String twoDigits(uint8_t value) {
  return value < 10 ? "0" + String(value) : String(value);
}

static String minuteText(uint16_t minuteOfDay) {
  return twoDigits(minuteOfDay / 60) + ":" + twoDigits(minuteOfDay % 60);
}

static String timestampNow() {
  if (!rtcReady) return "RTC unavailable";
  if (!rtcValid) return "RTC not set";

  const DateTime now = rtc.now();
  return twoDigits(now.hour()) + ":" + twoDigits(now.minute()) + ":" + twoDigits(now.second());
}

static uint32_t makeTriggerStamp(const DateTime& now) {
  const uint32_t dateKey =
      ((static_cast<uint32_t>(now.year()) * 13UL + now.month()) * 32UL + now.day());
  return dateKey * 1440UL + static_cast<uint32_t>(now.hour()) * 60UL + now.minute();
}

static bool isLeapYear(uint16_t year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static uint8_t daysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t DAYS[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 0;
  if (month == 2 && isLeapYear(year)) return 29;
  return DAYS[month - 1];
}

static void writeRelay(bool on) {
  const uint8_t level = (on == RELAY_ACTIVE_HIGH) ? HIGH : LOW;
  digitalWrite(RELAY_PIN, level);
}

static void stopBell(const char* reason) {
  if (!bellRinging) return;

  writeRelay(false);
  bellRinging = false;

  String message = "Bell OFF";
  if (reason && strlen(reason)) message += " • " + String(reason);
  eventLog.add(timestampNow(), message);
}

static bool startBell(uint8_t durationSeconds, const char* reason) {
  if (durationSeconds < 1 || durationSeconds > MAX_BELL_DURATION_SECONDS) return false;

  if (!bellRinging) {
    writeRelay(true);
    bellRinging = true;

    String message = "Bell ON";
    if (reason && strlen(reason)) message += " • " + String(reason);
    eventLog.add(timestampNow(), message);
  }

  // Non-blocking pulse. A new ring request restarts the timeout safely.
  bellOffAtMs = millis() + static_cast<unsigned long>(durationSeconds) * 1000UL;
  return true;
}

static void serviceBellPulse() {
  if (!bellRinging) return;

  if (static_cast<int32_t>(millis() - bellOffAtMs) >= 0) {
    stopBell("pulse complete");
  }
}

static void markCurrentMinuteTriggered(uint8_t slot = 0xFF) {
  if (!rtcValid) {
    lastTriggerStamp = 0;
    lastTriggerSlot = 0xFF;
    Storage::clearLastTrigger();
    return;
  }

  const DateTime now = rtc.now();
  lastTriggerStamp = makeTriggerStamp(now);
  lastTriggerSlot = slot;
  Storage::saveLastTrigger(lastTriggerStamp, lastTriggerSlot);
}

static void serviceScheduler() {
  if (!settings.automationEnabled || !rtcValid) return;

  const DateTime now = rtc.now();

  // Only trigger close to xx:xx:00. This avoids ringing late after a reboot.
  if (now.second() > TRIGGER_WINDOW_SECONDS) return;

  const uint16_t minuteOfDay = static_cast<uint16_t>(now.hour()) * 60U + now.minute();
  const uint32_t stamp = makeTriggerStamp(now);

  // One automatic ring maximum per calendar minute, persisted across resets.
  if (stamp == lastTriggerStamp) return;

  for (uint8_t i = 0; i < MAX_BELL_EVENTS; ++i) {
    const BellEvent& event = settings.events[i];
    if (!eventMatches(event, now.dayOfTheWeek(), minuteOfDay)) continue;

    lastTriggerStamp = stamp;
    lastTriggerSlot = i;
    Storage::saveLastTrigger(lastTriggerStamp, lastTriggerSlot);

    const String reason = "schedule " + minuteText(event.minuteOfDay);
    startBell(event.durationSeconds, reason.c_str());
    return;
  }
}

static String statusJson() {
  JsonDocument doc;

  doc["device"] = DEVICE_NAME;
  doc["firmware"] = FIRMWARE_VERSION;
  doc["schema"] = SETTINGS_SCHEMA_VERSION;
  doc["ssid"] = AP_SSID;
  doc["ip"] = apReady ? WiFi.softAPIP().toString() : "AP unavailable";
  doc["clients"] = apReady ? WiFi.softAPgetStationNum() : 0;
  doc["uptimeSec"] = millis() / 1000UL;
  doc["apReady"] = apReady;
  doc["rtcReady"] = rtcReady;
  doc["rtcValid"] = rtcValid;
  doc["automationEnabled"] = settings.automationEnabled;
  doc["bellRinging"] = bellRinging;

  if (rtcReady) {
    doc["temperature"] = rtc.getTemperature();
  } else {
    doc["temperature"] = 0;
  }

  if (rtcValid) {
    const DateTime now = rtc.now();
    doc["time"] = twoDigits(now.hour()) + ":" + twoDigits(now.minute()) + ":" + twoDigits(now.second());
    doc["date"] = twoDigits(now.day()) + "/" + twoDigits(now.month()) + "/" + String(now.year());
    doc["dayOfWeek"] = now.dayOfTheWeek();
  } else {
    doc["time"] = "--:--:--";
    doc["date"] = rtcReady ? "RTC needs synchronization" : "DS3231 unavailable";
    doc["dayOfWeek"] = 0;
  }

  JsonArray rows = doc["bellEvents"].to<JsonArray>();
  for (uint8_t i = 0; i < MAX_BELL_EVENTS; ++i) {
    const BellEvent& event = settings.events[i];
    JsonObject row = rows.add<JsonObject>();
    row["slot"] = i;
    row["enabled"] = event.enabled;
    row["daysMask"] = event.daysMask;
    row["minuteOfDay"] = event.minuteOfDay;
    row["time"] = minuteText(event.minuteOfDay);
    row["durationSeconds"] = event.durationSeconds;
  }

  String out;
  serializeJson(doc, out);
  return out;
}

static String historyJson() {
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

static void sendOk() {
  server.send(200, "application/json", "{\"ok\":true}");
}

static bool hasScheduleConflict(uint8_t editingSlot, uint16_t minuteOfDay, uint8_t daysMask, bool enabled) {
  if (!enabled) return false;

  for (uint8_t i = 0; i < MAX_BELL_EVENTS; ++i) {
    if (i == editingSlot) continue;

    const BellEvent& other = settings.events[i];
    if (!other.enabled) continue;

    if (other.minuteOfDay == minuteOfDay && (other.daysMask & daysMask) != 0) {
      return true;
    }
  }

  return false;
}

static void setupApi() {
  server.on("/api/status", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", statusJson());
  });

  server.on("/api/history", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", historyJson());
  });

  server.on("/api/ring", HTTP_POST, []() {
    JsonDocument doc;
    if (!readJson(doc)) return;

    const int duration = doc["durationSeconds"] | DEFAULT_BELL_DURATION_SECONDS;
    if (duration < 1 || duration > MAX_BELL_DURATION_SECONDS) {
      server.send(400, "application/json", "{\"error\":\"invalid_duration\"}");
      return;
    }

    startBell(static_cast<uint8_t>(duration), "manual web");
    sendOk();
  });

  server.on("/api/automation", HTTP_POST, []() {
    JsonDocument doc;
    if (!readJson(doc)) return;

    if (!doc["enabled"].is<bool>()) {
      server.send(400, "application/json", "{\"error\":\"missing_enabled\"}");
      return;
    }

    const bool enabled = doc["enabled"].as<bool>();
    settings.automationEnabled = enabled;

    if (!Storage::save(settings)) {
      server.send(500, "application/json", "{\"error\":\"storage_failed\"}");
      return;
    }

    if (enabled) {
      // Do not unexpectedly ring an event in the current minute when automation is resumed.
      markCurrentMinuteTriggered();
    }

    eventLog.add(timestampNow(), enabled ? "Automatic schedule enabled" : "Automatic schedule paused");
    sendOk();
  });

  server.on("/api/bell-event", HTTP_POST, []() {
    JsonDocument doc;
    if (!readJson(doc)) return;

    const int slot = doc["slot"] | -1;
    const int minuteOfDay = doc["minuteOfDay"] | -1;
    const int daysMask = doc["daysMask"] | -1;
    const int duration = doc["durationSeconds"] | -1;
    const bool enabled = doc["enabled"] | false;

    if (slot < 0 || slot >= MAX_BELL_EVENTS ||
        minuteOfDay < 0 || minuteOfDay >= 1440 ||
        daysMask < 0 || daysMask > 127 ||
        duration < 1 || duration > MAX_BELL_DURATION_SECONDS) {
      server.send(400, "application/json", "{\"error\":\"invalid_bell_event\"}");
      return;
    }

    if (hasScheduleConflict(
          static_cast<uint8_t>(slot),
          static_cast<uint16_t>(minuteOfDay),
          static_cast<uint8_t>(daysMask),
          enabled)) {
      server.send(409, "application/json", "{\"error\":\"duplicate_time_for_selected_days\"}");
      return;
    }

    BellEvent& event = settings.events[slot];
    event.enabled = enabled;
    event.minuteOfDay = static_cast<uint16_t>(minuteOfDay);
    event.daysMask = static_cast<uint8_t>(daysMask);
    event.durationSeconds = static_cast<uint8_t>(duration);

    if (!Storage::save(settings)) {
      server.send(500, "application/json", "{\"error\":\"storage_failed\"}");
      return;
    }

    eventLog.add(timestampNow(), "Bell event " + String(slot + 1) + " updated to " + minuteText(event.minuteOfDay));
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

    const uint8_t maxDay =
        (year >= 2024 && year <= 2099 && month >= 1 && month <= 12)
          ? daysInMonth(static_cast<uint16_t>(year), static_cast<uint8_t>(month))
          : 0;

    if (!rtcReady ||
        year < 2024 || year > 2099 ||
        month < 1 || month > 12 ||
        day < 1 || day > maxDay ||
        hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 ||
        second < 0 || second > 59) {
      server.send(400, "application/json", "{\"error\":\"invalid_time\"}");
      return;
    }

    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    rtcValid = true;

    // Synchronizing the clock must not immediately fire an event in this minute.
    markCurrentMinuteTriggered();

    eventLog.add(timestampNow(), "RTC synchronized from connected device");
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

    stopBell("factory reset");
    writeRelay(false);

    Storage::factoryReset(settings);
    eventLog.clear();

    // Avoid a surprise bell if reset is performed during a default event minute.
    markCurrentMinuteTriggered();

    eventLog.add(timestampNow(), "Factory defaults restored");
    sendOk();
  });
}

static void redirectToDashboard() {
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
  server.send(302, "text/plain", "");
}

static void setupWeb() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html", INDEX_HTML);
  });

  setupApi();

  server.on("/generate_204", HTTP_ANY, redirectToDashboard);
  server.on("/gen_204", HTTP_ANY, redirectToDashboard);
  server.on("/hotspot-detect.html", HTTP_ANY, redirectToDashboard);
  server.on("/library/test/success.html", HTTP_ANY, redirectToDashboard);
  server.on("/ncsi.txt", HTTP_ANY, redirectToDashboard);
  server.on("/connecttest.txt", HTTP_ANY, redirectToDashboard);

  server.onNotFound([]() {
    if (server.uri().startsWith("/api/")) {
      server.send(404, "application/json", "{\"error\":\"not_found\"}");
      return;
    }
    redirectToDashboard();
  });

  server.begin();
}

static void setupAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  const IPAddress ip(192, 168, 4, 1);
  const IPAddress gateway(192, 168, 4, 1);
  const IPAddress subnet(255, 255, 255, 0);

  const bool configured = WiFi.softAPConfig(ip, gateway, subnet);
  const bool started = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, false, AP_MAX_CLIENTS);
  apReady = configured && started;

  if (apReady) {
    dnsServer.start(53, "*", ip);
  }
}

void setup() {
  Serial.begin(115200);

  // Fail-safe boot: preload OFF before making the pin an output.
  const uint8_t offLevel = RELAY_ACTIVE_HIGH ? LOW : HIGH;
  digitalWrite(RELAY_PIN, offLevel);
  pinMode(RELAY_PIN, OUTPUT);
  writeRelay(false);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  rtcReady = rtc.begin();
  rtcValid = rtcReady && !rtc.lostPower();

  Storage::begin(settings);
  Storage::loadLastTrigger(lastTriggerStamp, lastTriggerSlot);

  setupAccessPoint();
  setupWeb();

  eventLog.add(timestampNow(), "Controller started");
  if (!rtcReady) {
    eventLog.add("boot", "DS3231 not detected; automatic ringing disabled");
  } else if (!rtcValid) {
    eventLog.add("boot", "RTC lost power; synchronize time before automatic ringing");
  }

  if (!apReady) {
    eventLog.add(timestampNow(), "Wi-Fi access point failed to start");
  }

  Serial.println();
  Serial.println(DEVICE_NAME);
  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("Dashboard: http://");
  Serial.println(WiFi.softAPIP());
  Serial.print("RTC valid: ");
  Serial.println(rtcValid ? "yes" : "no");
}

void loop() {
  if (apReady) {
    dnsServer.processNextRequest();
  }

  server.handleClient();

  serviceBellPulse();
  serviceScheduler();

  delay(2);
}
