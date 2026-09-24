#pragma once

#include <Arduino.h>

// ---------- I2C ----------
static constexpr uint8_t I2C_SDA_PIN = 21;
static constexpr uint8_t I2C_SCL_PIN = 22;

// ---------- Relays ----------
static constexpr uint8_t RELAY_PINS[4] = {25, 26, 27, 33};

// Change to false if your relay board turns ON when GPIO is LOW.
static constexpr bool RELAY_ACTIVE_HIGH = true;

// ---------- Buttons ----------
static constexpr uint8_t BTN_CLOCK_PIN = 13;
static constexpr uint8_t BTN_UP_PIN = 14;
static constexpr uint8_t BTN_DOWN_PIN = 16;
static constexpr uint8_t BTN_TIMER_PIN = 17;
static constexpr uint8_t BTN_BACK_PIN = 18;

// ---------- Buzzer ----------
static constexpr uint8_t BUZZER_PIN = 19;

// ---------- Access Point ----------
static constexpr char AP_SSID[] = "Relay-Control";
static constexpr char AP_PASSWORD[] = "relay1234";

// ---------- Device ----------
static constexpr char DEVICE_NAME[] = "4-Relay Timer";
static constexpr char FIRMWARE_VERSION[] = "ESP32-v0.1.0";
