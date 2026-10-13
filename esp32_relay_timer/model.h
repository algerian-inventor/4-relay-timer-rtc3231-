#pragma once
#include <Arduino.h>
#include "config.h"

struct BellEvent {
  bool enabled = false;
  uint8_t daysMask = SCHOOL_DAYS_MASK;
  uint16_t minuteOfDay = 8 * 60;
  uint8_t durationSec = DEFAULT_BELL_DURATION_SECONDS;
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

inline bool validBellDuration(uint8_t value) {
  return value >= 1 && value <= MAX_BELL_DURATION_SECONDS;
}

inline bool dayEnabled(uint8_t mask, uint8_t dayOfWeek) {
  return dayOfWeek < 7 && (mask & (1U << dayOfWeek));
}

inline bool eventMatches(const BellEvent& event, uint8_t dayOfWeek, uint16_t minuteOfDay) {
  return event.enabled &&
         validMinuteOfDay(event.minuteOfDay) &&
         dayEnabled(event.daysMask, dayOfWeek) &&
         event.minuteOfDay == minuteOfDay;
}
