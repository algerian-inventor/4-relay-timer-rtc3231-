#pragma once
#include <Arduino.h>

static constexpr uint8_t CHANNEL_COUNT = 4;
static constexpr uint8_t MAX_SCHEDULES = 6;

// I2C
static constexpr uint8_t I2C_SDA_PIN = 21;
static constexpr uint8_t I2C_SCL_PIN = 22;
static constexpr uint8_t LCD_ADDRESS = 0x27;

// Relays
static constexpr uint8_t RELAY_PINS[CHANNEL_COUNT] = {25, 26, 27, 33};
// Set false for the common active-LOW relay modules.
static constexpr bool RELAY_ACTIVE_HIGH = true;

// Buttons (INPUT_PULLUP)
static constexpr uint8_t BTN_CLOCK_PIN = 13;
static constexpr uint8_t BTN_UP_PIN = 14;
static constexpr uint8_t BTN_DOWN_PIN = 16;
static constexpr uint8_t BTN_TIMER_PIN = 17;
static constexpr uint8_t BTN_BACK_PIN = 18;
static constexpr uint8_t BUZZER_PIN = 19;

// Local network
static constexpr char AP_SSID[] = "Relay-Control";
static constexpr char AP_PASSWORD[] = "relay1234";
static constexpr uint8_t AP_CHANNEL = 6;
static constexpr uint8_t AP_MAX_CLIENTS = 4;

// Product
static constexpr char DEVICE_NAME[] = "Relay Automation Controller";
static constexpr char FIRMWARE_VERSION[] = "1.0.0-alpha.1";
static constexpr uint32_t SETTINGS_SCHEMA_VERSION = 1;
static constexpr uint32_t SETTINGS_MAGIC = 0x524C5931; // RLY1

static constexpr unsigned long LCD_REFRESH_MS = 300;
static constexpr unsigned long BUTTON_DEBOUNCE_MS = 35;
