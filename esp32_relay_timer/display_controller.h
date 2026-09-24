#pragma once
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include "model.h"

class DisplayController {
public:
  DisplayController(LiquidCrystal_I2C& lcd) : lcd_(lcd) {}

  void begin() {
    lcd_.init();
    lcd_.backlight();
    lcd_.clear();
  }

  void showBoot() {
    lcd_.clear();
    lcd_.setCursor(0, 0);
    lcd_.print("Relay Control");
    lcd_.setCursor(0, 1);
    lcd_.print("ESP32 V1 boot...");
  }

  void setSelected(uint8_t index) { selected_ = index % CHANNEL_COUNT; }
  uint8_t selected() const { return selected_; }

  void nextChannel() { selected_ = (selected_ + 1) % CHANNEL_COUNT; }

  void refresh(bool rtcReady, RTC_DS3231& rtc, RelayChannel* channels) {
    if (millis() - lastRefresh_ < LCD_REFRESH_MS) return;
    lastRefresh_ = millis();

    if (!rtcReady) {
      line(0, "RTC NOT FOUND");
      line(1, "WiFi: Relay-Control");
      return;
    }

    DateTime now = rtc.now();
    char first[17];
    snprintf(first, sizeof(first), "%02d:%02d:%02d  C%d", now.hour(), now.minute(), now.second(), selected_ + 1);

    const RelayChannel& channel = channels[selected_];
    const char* mode = channel.mode == RelayMode::AUTO ? "AUTO" :
                       channel.mode == RelayMode::FORCE_ON ? "ON" : "OFF";

    char second[17];
    snprintf(second, sizeof(second), "%-6.6s %s %s", channel.name, channel.outputState ? "RUN" : "STOP", mode);

    line(0, first);
    line(1, second);
  }

private:
  LiquidCrystal_I2C& lcd_;
  uint8_t selected_ = 0;
  unsigned long lastRefresh_ = 0;

  void line(uint8_t row, const String& text) {
    lcd_.setCursor(0, row);
    String value = text.substring(0, 16);
    lcd_.print(value);
    for (uint8_t i = value.length(); i < 16; ++i) lcd_.print(' ');
  }
};
