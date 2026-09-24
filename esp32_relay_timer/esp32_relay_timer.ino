#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#include "config.h"
#include "model.h"
#include "web_ui.h"

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Preferences preferences;
WebServer server(80);
DNSServer dnsServer;

RelayChannel relays[4];
bool rtcReady = false;

static void setRelayHardware(uint8_t index, bool on) {
  if (index >= 4) return;
  const uint8_t level = (on == RELAY_ACTIVE_HIGH) ? HIGH : LOW;
  digitalWrite(RELAY_PINS[index], level);
  relays[index].outputState = on;
}

static const char* modeToString(RelayMode mode) {
  switch (mode) {
    case RelayMode::FORCE_ON: return "ON";
    case RelayMode::FORCE_OFF: return "OFF";
    default: return "AUTO";
  }
}

static void saveRelay(uint8_t index) {
  if (index >= 4) return;

  char key[20];
  snprintf(key, sizeof(key), "r%u_mode", index);
  preferences.putUChar(key, static_cast<uint8_t>(relays[index].mode));

  snprintf(key, sizeof(key), "r%u_en", index);
  preferences.putBool(key, relays[index].schedule.enabled);

  snprintf(key, sizeof(key), "r%u_sh", index);
  preferences.putUChar(key, relays[index].schedule.startHour);

  snprintf(key, sizeof(key), "r%u_sm", index);
  preferences.putUChar(key, relays[index].schedule.startMinute);

  snprintf(key, sizeof(key), "r%u_eh", index);
  preferences.putUChar(key, relays[index].schedule.endHour);

  snprintf(key, sizeof(key), "r%u_em", index);
  preferences.putUChar(key, relays[index].schedule.endMinute);
}

static void loadSettings() {
  preferences.begin("relay-timer", false);

  for (uint8_t i = 0; i < 4; i++) {
    relays[i].name = "Relay " + String(i + 1);

    char key[20];

    snprintf(key, sizeof(key), "r%u_mode", i);
    uint8_t storedMode = preferences.getUChar(key, 0);
    if (storedMode > 2) storedMode = 0;
    relays[i].mode = static_cast<RelayMode>(storedMode);

    snprintf(key, sizeof(key), "r%u_en", i);
    relays[i].schedule.enabled = preferences.getBool(key, false);

    snprintf(key, sizeof(key), "r%u_sh", i);
    relays[i].schedule.startHour = preferences.getUChar(key, 8);

    snprintf(key, sizeof(key), "r%u_sm", i);
    relays[i].schedule.startMinute = preferences.getUChar(key, 0);

    snprintf(key, sizeof(key), "r%u_eh", i);
    relays[i].schedule.endHour = preferences.getUChar(key, 9);

    snprintf(key, sizeof(key), "r%u_em", i);
    relays[i].schedule.endMinute = preferences.getUChar(key, 0);

    if (relays[i].schedule.startHour > 23) relays[i].schedule.startHour = 8;
    if (relays[i].schedule.startMinute > 59) relays[i].schedule.startMinute = 0;
    if (relays[i].schedule.endHour > 23) relays[i].schedule.endHour = 9;
    if (relays[i].schedule.endMinute > 59) relays[i].schedule.endMinute = 0;
  }
}

static void updateRelays() {
  uint8_t hour = 0;
  uint8_t minute = 0;

  if (rtcReady) {
    DateTime now = rtc.now();
    hour = now.hour();
    minute = now.minute();
  }

  for (uint8_t i = 0; i < 4; i++) {
    bool desired = false;

    switch (relays[i].mode) {
      case RelayMode::FORCE_ON:
        desired = true;
        break;

      case RelayMode::FORCE_OFF:
        desired = false;
        break;

      case RelayMode::AUTO:
      default:
        desired = rtcReady &&
          scheduleIsActive(hour, minute, relays[i].schedule);
        break;
    }

    if (desired != relays[i].outputState) {
      setRelayHardware(i, desired);
    }
  }
}

static String twoDigits(uint8_t value) {
  if (value < 10) return "0" + String(value);
  return String(value);
}

static void updateLcd() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 500) return;
  lastUpdate = millis();

  lcd.setCursor(0, 0);

  if (!rtcReady) {
    lcd.print("RTC NOT FOUND   ");
    lcd.setCursor(0, 1);
    lcd.print("AP: Relay-Control");
    return;
  }

  DateTime now = rtc.now();

  String top = twoDigits(now.hour()) + ":" +
               twoDigits(now.minute()) + ":" +
               twoDigits(now.second());

  lcd.print(top);
  for (int i = top.length(); i < 16; i++) lcd.print(' ');

  String states = "";
  for (uint8_t i = 0; i < 4; i++) {
    states += "R";
    states += String(i + 1);
    states += relays[i].outputState ? "+" : "-";
    if (i < 3) states += " ";
  }

  lcd.setCursor(0, 1);
  lcd.print(states.substring(0, 16));
  for (int i = states.length(); i < 16; i++) lcd.print(' ');
}

static String buildStatusJson() {
  StaticJsonDocument<1536> doc;

  doc["device"] = DEVICE_NAME;
  doc["firmware"] = FIRMWARE_VERSION;
  doc["ip"] = WiFi.softAPIP().toString();

  if (rtcReady) {
    DateTime now = rtc.now();

    String time = twoDigits(now.hour()) + ":" +
                  twoDigits(now.minute()) + ":" +
                  twoDigits(now.second());

    String date = twoDigits(now.day()) + "/" +
                  twoDigits(now.month()) + "/" +
                  String(now.year());

    doc["time"] = time;
    doc["date"] = date;
    doc["temperature"] = rtc.getTemperature();
  } else {
    doc["time"] = "--:--:--";
    doc["date"] = "RTC unavailable";
    doc["temperature"] = 0.0;
  }

  JsonArray relayArray = doc.createNestedArray("relays");

  for (uint8_t i = 0; i < 4; i++) {
    JsonObject item = relayArray.createNestedObject();
    item["name"] = relays[i].name;
    item["mode"] = static_cast<uint8_t>(relays[i].mode);
    item["modeName"] = modeToString(relays[i].mode);
    item["state"] = relays[i].outputState;

    JsonObject schedule = item.createNestedObject("schedule");
    schedule["enabled"] = relays[i].schedule.enabled;
    schedule["startHour"] = relays[i].schedule.startHour;
    schedule["startMinute"] = relays[i].schedule.startMinute;
    schedule["endHour"] = relays[i].schedule.endHour;
    schedule["endMinute"] = relays[i].schedule.endMinute;
  }

  String json;
  serializeJson(doc, json);
  return json;
}

static bool parseBody(StaticJsonDocument<384>& doc) {
  if (!server.hasArg("plain")) return false;

  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    server.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return false;
  }

  return true;
}

static void sendDashboard() {
  server.send_P(200, "text/html", INDEX_HTML);
}

static void redirectToDashboard() {
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
  server.send(302, "text/plain", "");
}

static void setupWebServer() {
  server.on("/", HTTP_GET, sendDashboard);

  server.on("/api/status", HTTP_GET, []() {
    server.send(200, "application/json", buildStatusJson());
  });

  server.on("/api/relay/mode", HTTP_POST, []() {
    StaticJsonDocument<384> doc;
    if (!parseBody(doc)) return;

    int relay = doc["relay"] | -1;
    int mode = doc["mode"] | -1;

    if (relay < 0 || relay > 3 || mode < 0 || mode > 2) {
      server.send(400, "application/json", "{\"error\":\"invalid_request\"}");
      return;
    }

    relays[relay].mode = static_cast<RelayMode>(mode);
    saveRelay(relay);
    updateRelays();

    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/relay/schedule", HTTP_POST, []() {
    StaticJsonDocument<384> doc;
    if (!parseBody(doc)) return;

    int relay = doc["relay"] | -1;
    int startHour = doc["startHour"] | -1;
    int startMinute = doc["startMinute"] | -1;
    int endHour = doc["endHour"] | -1;
    int endMinute = doc["endMinute"] | -1;
    bool enabled = doc["enabled"] | false;

    if (
      relay < 0 || relay > 3 ||
      startHour < 0 || startHour > 23 ||
      endHour < 0 || endHour > 23 ||
      startMinute < 0 || startMinute > 59 ||
      endMinute < 0 || endMinute > 59
    ) {
      server.send(400, "application/json", "{\"error\":\"invalid_schedule\"}");
      return;
    }

    RelaySchedule& s = relays[relay].schedule;
    s.enabled = enabled;
    s.startHour = startHour;
    s.startMinute = startMinute;
    s.endHour = endHour;
    s.endMinute = endMinute;

    saveRelay(relay);
    updateRelays();

    server.send(200, "application/json", "{\"ok\":true}");
  });

  // Common captive portal probes.
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

  IPAddress localIp(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.softAPConfig(localIp, gateway, subnet);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  dnsServer.start(53, "*", localIp);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  for (uint8_t i = 0; i < 4; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    relays[i].outputState = false;
    setRelayHardware(i, false);
  }

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(BTN_CLOCK_PIN, INPUT_PULLUP);
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_TIMER_PIN, INPUT_PULLUP);
  pinMode(BTN_BACK_PIN, INPUT_PULLUP);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("4-Relay ESP32");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  rtcReady = rtc.begin();

  loadSettings();
  setupAccessPoint();
  setupWebServer();
  updateRelays();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Ready");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.softAPIP());

  Serial.println();
  Serial.println("ESP32 relay controller ready");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("Dashboard: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  updateRelays();
  updateLcd();

  delay(2);
}
