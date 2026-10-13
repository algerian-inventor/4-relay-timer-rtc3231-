#pragma once
#include <Arduino.h>

// Minimal product hardware: ESP32 + DS3231 + one relay module.
static constexpr uint8_t MAX_BELL_EVENTS = 12;

// DS3231 I2C
static constexpr uint8_t I2C_SDA_PIN = 21;
static constexpr uint8_t I2C_SCL_PIN = 22;

// Bell relay
static constexpr uint8_t RELAY_PIN = 25;

// Common 5 V relay modules are usually active-LOW.
// OFF = HIGH, ON = LOW. Verify the exact relay module before installation.
static constexpr bool RELAY_ACTIVE_HIGH = false;

// Bell behavior
static constexpr uint8_t DEFAULT_BELL_DURATION_SECONDS = 3;
static constexpr uint8_t MAX_BELL_DURATION_SECONDS = 30;
// Automatic events only start near second 00. This prevents a reboot at
// e.g. 10:00:45 from unexpectedly ringing the 10:00 bell.
static constexpr uint8_t TRIGGER_WINDOW_SECONDS = 5;

// Local access point
static constexpr char AP_SSID[] = "SchoolBell-Setup";
static constexpr char AP_PASSWORD[] = "schoolbell";
static constexpr uint8_t AP_CHANNEL = 6;
static constexpr uint8_t AP_MAX_CLIENTS = 4;

// Product
static constexpr char DEVICE_NAME[] = "School Bell Controller";
static constexpr char FIRMWARE_VERSION[] = "1.0.0-alpha.4";
static constexpr uint32_t SETTINGS_SCHEMA_VERSION = 3;
static constexpr uint32_t SETTINGS_MAGIC = 0x53424331; // SBC1
