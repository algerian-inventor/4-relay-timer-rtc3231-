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

bool rtcDetected = false;
bool rtcValid = false;
bool apStarted = false;

bool relayOn = false;
uint32_t relayOffDeadlineMs = 0;

bool schedulerPrimed = false;
uint32_t observedMinuteToken = 0;
uint32_t lastTriggeredMinuteToken = 0;
uint32_t lastRtcHealthCheckMs = 0;

static String twoDigits(uint8_t value) {
  return value < 10 ? "0" + String(value) : String(value);
}

static String minuteText(uint16_t minuteOfDay) {
  return twoDigits(minuteOfDay / 60) + ":" + twoDigits(minuteOfDay % 60);
}

static bool isLeapYear(uint16_t year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static uint8_t daysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month < 1 || month > 12) return 0;
  if (month == 2 && isLeapYear(year)) return 29;
  return days[month - 1];
}

static bool validDateTimeFields(
  int year, int month, int day, int hour, int minute, int second
) {
  if (year < 2024 || year > 2099) return false;
  if (month < 1 || month > 12) return false;
  if (day < 1 || day > daysInMonth(year, month)) return false;
  if (hour < 0 || hour > 23) return false;
  if (minute < 0 || minute > 59) return false;
  if (second < 0 || second > 59) return false;
  return true;
}

static String timestampNow() {
  if (!rtcValid) return "RTC invalid";
  const DateTime now = rtc.now();
  return twoDigits(now.hour()) + ":" +
         twoDigits(now.minute()) + ":" +
         twoDigits(now.second());
}

static uint32_t minuteToken(const DateTime& now) {
  const uint32_t minuteOfDay = now.hour() * 60U + now.minute();
  return ((((uint32_t)now.year() * 13U + now.month()) * 32U + now.day()) * 1440U)
         + minuteOfDay;
}

static void writeRelay(bool on) {
  const uint8_t level = (on == RELAY_ACTIVE_HIGH) ? HIGH : LOW;
  digitalWrite(RELAY_PIN, level);
}

static void stopBell(const char* reason) {
  if (!relayOn) {
    writeRelay(false);
    return;
  }

  writeRelay(false);
  relayOn = false;

  String message = "Bell OFF";
  if (reason && strlen(reason)) message += " • " + String(reason);
  eventLog.add(timestampNow(), message);
}

static void startBellPulse(uint8_t durationSec, const char* reason) {
  if (!validBellDuration(durationSec)) {
    durationSec = DEFAULT_BELL_DURATION_SECONDS;
  }

  writeRelay(true);

  if (!relayOn) {
    relayOn = true;
    String message = "Bell ON";
    if (reason && strlen(reason)) message += " • " + String(reason);
    message += " • " + String(durationSec) + "s";
    eventLog.add(timestampNow(), message);
  }

  // Non-blocking deadline. A new pulse safely replaces/extends the current one.
  relayOffDeadlineMs = millis() + (uint32_t)durationSec * 1000UL;
}

static void serviceRelayTimeout() {
  if (!relayOn) return;

  if ((int32_t)(millis() - relayOffDeadlineMs) >= 0) {
    stopBell("pulse complete");
  }
}

static void resetSchedulerBaseline() {
  schedulerPrimed = false;
  observedMinuteToken = 0;
}

static void refreshRtcHealth() {
  if (!rtcDetected) {
    rtcValid = false;
    return;
  }

  if (millis() - lastRtcHealthCheckMs < 1000UL) return;
  lastRtcHealthCheckMs = millis();

  bool healthy = !rtc.lostPower();

  if (healthy) {
    const DateTime now = rtc.now();
    healthy = now.year() >= 2024 && now.year() <= 2099;
  }

  if (rtcValid && !healthy) {
    stopBell("RTC fault");
    resetSchedulerBaseline();
    eventLog.add("RTC", "Automatic schedule paused because RTC time is invalid");
  }

  rtcValid = healthy;
}

static void serviceSchedule() {
  if (!rtcValid || !settings.automationEnabled) {
    resetSchedulerBaseline();
    return;
  }

  const DateTime now = rtc.now();
  const uint32_t token = minuteToken(now);

  // On boot or after a clock/mode change, observe the current minute first.
  // This intentionally avoids an accidental repeat ring if power returns
  // halfway through a scheduled minute.
  if (!schedulerPrimed) {
    observedMinuteToken = token;
    schedulerPrimed = true;
    return;
  }

  if (token == observedMinuteToken) return;
  observedMinuteToken = token;

  if (token == lastTriggeredMinuteToken) return;

  const uint8_t day = now.dayOfTheWeek();
  const uint16_t minuteOfDay = now.hour() * 60U + now.minute();

  uint8_t matched = 0;
  uint8_t durationSec = 0;

  for (uint8_t i = 0; i < MAX_BELL_EVENTS; ++i) {
    const BellEvent& event = settings.events[i];
    if (!eventMatches(event, day, minuteOfDay)) continue;

    ++matched;
    if (event.durationSec > durationSec) durationSec = event.durationSec;
  }

  if (matched == 0) return;

  lastTriggeredMinuteToken = token;
  startBellPulse(durationSec, "scheduled");

  if (matched > 1) {
    eventLog.add(timestampNow(), String(matched) + " bell events shared this minute; one pulse used");
  }
}

static String statusJson() {
  JsonDocument doc;

  doc["device"] = DEVICE_NAME;
  doc["firmware"] = FIRMWARE_VERSION;
  doc["schema"] = SETTINGS_SCHEMA_VERSION;
  doc["ssid"] = AP_SSID;
  doc["ip"] = WiFi.softAPIP().toString();
  doc["clients"] = WiFi.softAPgetStationNum();
  doc["apStarted"] = apStarted;
  doc["uptimeSec"] = millis() / 1000UL;

  doc["rtcDetected"] = rtcDetected;
  doc["rtcValid"] = rtcValid;
  doc["relayOn"] = relayOn;
  doc["automationEnabled"] = settings.automationEnabled;

  if (rtcValid) {
    const DateTime now = rtc.now();
    doc["time"] = twoDigits(now.hour()) + ":" +
                  twoDigits(now.minute()) + ":" +
                  twoDigits(now.second());
    doc["date"] = twoDigits(now.day()) + "/" +
                  twoDigits(now.month()) + "/" +
                  String(now.year());
    doc["dayOfWeek"] = now.dayOfTheWeek();
    doc["minuteOfDay"] = now.hour() * 60U + now.minute();
    doc["temperature"] = rtc.getTemperature();
  } else {
    doc["time"] = "--:--:--";
    doc["date"] = rtcDetected ? "RTC time must be synchronized" : "DS3231 not detected";
    doc["dayOfWeek"] = 0;
    doc["minuteOfDay"] = 0;
    doc["temperature"] = 0;
  }

  JsonArray events = doc["events"].to<JsonArray>();
  for (uint8_t i = 0; i < MAX_BELL_EVENTS; ++i) {
    const BellEvent& event = settings.events[i];
    JsonObject row = events.add<JsonObject>();
    row["slot"] = i;
    row["enabled"] = event.enabled;
    row["daysMask"] = event.daysMask;
    row["minuteOfDay"] = event.minuteOfDay;
    row["time"] = minuteText(event.minuteOfDay);
    row["durationSec"] = event.durationSec;
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

  const String body = server.arg("plain");
  if (body.length() > 2048) {
    server.send(413, "application/json", "{\"error\":\"body_too_large\"}");
    return false;
  }

  const DeserializationError error = deserializeJson(doc, body);
  if (error) {
    server.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return false;
  }

  return true;
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

  server.on("/api/ring", HTTP_POST, []() {
    JsonDocument doc;
    if (!readJson(doc)) return;

    int duration = doc["durationSec"] | DEFAULT_BELL_DURATION_SECONDS;
    if (duration < 1 || duration > MAX_BELL_DURATION_SECONDS) {
      server.send(400, "application/json", "{\"error\":\"invalid_duration\"}");
      return;
    }

    startBellPulse((uint8_t)duration, "manual web control");
    sendOk();
  });

  server.on("/api/automation", HTTP_POST, []() {
    JsonDocument doc;
    if (!readJson(doc)) return;

    if (!doc["enabled"].is<bool>()) {
      server.send(400, "application/json", "{\"error\":\"invalid_request\"}");
      return;
    }

    settings.automationEnabled = doc["enabled"].as<bool>();
    if (!Storage::save(settings)) {
      server.send(500, "application/json", "{\"error\":\"save_failed\"}");
      return;
    }

    resetSchedulerBaseline();
    eventLog.add(timestampNow(), settings.automationEnabled ? "Automatic schedule enabled" : "Automatic schedule disabled");
    sendOk();
  });

  server.on("/api/event", HTTP_POST, []() {
    JsonDocument doc;
    if (!readJson(doc)) return;

    const int slot = doc["slot"] | -1;
    const int minuteOfDay = doc["minuteOfDay"] | -1;
    const int daysMask = doc["daysMask"] | -1;
    const int durationSec = doc["durationSec"] | -1;

    if (slot < 0 || slot >= MAX_BELL_EVENTS ||
        minuteOfDay < 0 || minuteOfDay >= 1440 ||
        daysMask < 0 || daysMask > 127 ||
        durationSec < 1 || durationSec > MAX_BELL_DURATION_SECONDS ||
        !doc["enabled"].is<bool>()) {
      server.send(400, "application/json", "{\"error\":\"invalid_event\"}");
      return;
    }

    BellEvent& event = settings.events[slot];
    event.enabled = doc["enabled"].as<bool>();
    event.daysMask = (uint8_t)daysMask;
    event.minuteOfDay = (uint16_t)minuteOfDay;
    event.durationSec = (uint8_t)durationSec;

    if (!Storage::save(settings)) {
      server.send(500, "application/json", "{\"error\":\"save_failed\"}");
      return;
    }

    resetSchedulerBaseline();
    eventLog.add(timestampNow(), "Bell event " + String(slot + 1) + " updated");
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

    if (!rtcDetected ||
        !validDateTimeFields(year, month, day, hour, minute, second)) {
      server.send(400, "application/json", "{\"error\":\"invalid_time\"}");
      return;
    }

    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    rtcValid = true;
    resetSchedulerBaseline();
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

    stopBell("factory reset");
    Storage::factoryReset(settings);
    resetSchedulerBaseline();
    eventLog.clear();
    eventLog.add(timestampNow(), "Factory timetable restored");
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
  apStarted = WiFi.softAP(
    AP_SSID,
    AP_PASSWORD,
    AP_CHANNEL,
    false,
    AP_MAX_CLIENTS
  );

  if (apStarted) {
    dnsServer.start(53, "*", ip);
  }
}

void setup() {
  Serial.begin(115200);
  delay(50);

  // Fail-safe startup: establish OFF before enabling the relay GPIO.
  const uint8_t offLevel = RELAY_ACTIVE_HIGH ? LOW : HIGH;
  digitalWrite(RELAY_PIN, offLevel);
  pinMode(RELAY_PIN, OUTPUT);
  writeRelay(false);
  relayOn = false;

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  rtcDetected = rtc.begin();

  if (rtcDetected) {
    rtcValid = !rtc.lostPower();
    if (rtcValid) {
      const DateTime now = rtc.now();
      rtcValid = now.year() >= 2024 && now.year() <= 2099;
    }
  }

  Storage::begin(settings);

  setupAccessPoint();
  setupWeb();

  eventLog.add(rtcValid ? timestampNow() : "boot", "Controller started");

  if (!rtcDetected) {
    eventLog.add("boot", "DS3231 not detected; automatic ringing disabled");
  } else if (!rtcValid) {
    eventLog.add("boot", "RTC time invalid; synchronize time before automatic ringing");
  }

  if (!apStarted) {
    eventLog.add("boot", "Wi-Fi access point failed to start");
  }

  Serial.println();
  Serial.println(DEVICE_NAME);
  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("Dashboard: http://");
  Serial.println(WiFi.softAPIP());
  Serial.print("RTC: ");
  Serial.println(rtcValid ? "ready" : "not ready");
}

void loop() {
  if (apStarted) dnsServer.processNextRequest();
  server.handleClient();

  refreshRtcHealth();
  serviceRelayTimeout();
  serviceSchedule();

  delay(2);
}
