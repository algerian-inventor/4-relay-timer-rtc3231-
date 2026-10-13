#pragma once
#include <Arduino.h>
#include "config.h"

struct BellEvent {
  bool enabled = true;
  uint8_t daysMask = 0x1F; // bit0=Sun ... bit4=Thu; Fri/Sat disabled by default
  uint16_t minuteOfDay = 8 * 60;
  uint8_t durationSeconds = DEFAULT_BELL_DURATION_SECONDS;
};

struct PersistedSettings {
  uint32_t magic = SETTINGS_MAGIC;
  uint32_t schemaVersion = SETTINGS_SCHEMA_VERSION;
  bool automationEnabled = true;
  BellEvent events[MAX_BELL_EVENTS];
};

inline bool validMinuteOfDay(uint16_t value) {
  return value < 1440;
}

inline bool dayEnabled(uint8_t mask, uint8_t dayOfWeek) {
  return dayOfWeek < 7 && (mask & (1U << dayOfWeek));
}

inline bool eventMatches(const BellEvent& event, uint8_t dayOfWeek, uint16_t minuteOfDay) {
  return event.enabled &&
         validMinuteOfDay(event.minuteOfDay) &&
         event.minuteOfDay == minuteOfDay &&
         dayEnabled(event.daysMask, dayOfWeek);
}
