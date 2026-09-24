#pragma once
#include <Arduino.h>
#include "config.h"

enum class RelayMode : uint8_t {
  AUTO = 0,
  FORCE_ON = 1,
  FORCE_OFF = 2
};

struct ScheduleRule {
  bool enabled = false;
  uint8_t daysMask = 0x7F; // bit0=Sun ... bit6=Sat
  uint16_t startMinute = 8 * 60;
  uint16_t endMinute = 9 * 60;
};

struct RelayChannel {
  char name[24] = {0};
  RelayMode mode = RelayMode::AUTO;
  ScheduleRule schedules[MAX_SCHEDULES];
  bool outputState = false; // runtime only
};

struct PersistedSettings {
  uint32_t magic = SETTINGS_MAGIC;
  uint32_t schemaVersion = SETTINGS_SCHEMA_VERSION;
  RelayChannel channels[CHANNEL_COUNT];
};

inline bool validMinuteOfDay(uint16_t value) {
  return value < 1440;
}

inline bool validMode(RelayMode mode) {
  return static_cast<uint8_t>(mode) <= static_cast<uint8_t>(RelayMode::FORCE_OFF);
}

inline bool dayEnabled(uint8_t mask, uint8_t dayOfWeek) {
  return dayOfWeek < 7 && (mask & (1U << dayOfWeek));
}

inline bool scheduleMatches(const ScheduleRule& rule, uint8_t dayOfWeek, uint16_t nowMinute) {
  if (!rule.enabled || !validMinuteOfDay(rule.startMinute) || !validMinuteOfDay(rule.endMinute)) return false;
  if (rule.startMinute == rule.endMinute) return false;

  if (rule.startMinute < rule.endMinute) {
    return dayEnabled(rule.daysMask, dayOfWeek) &&
           nowMinute >= rule.startMinute &&
           nowMinute < rule.endMinute;
  }

  // Cross-midnight: late portion belongs to today's rule;
  // early portion belongs to the previous day's rule.
  if (nowMinute >= rule.startMinute) {
    return dayEnabled(rule.daysMask, dayOfWeek);
  }

  const uint8_t previousDay = (dayOfWeek + 6) % 7;
  return nowMinute < rule.endMinute && dayEnabled(rule.daysMask, previousDay);
}

inline bool autoShouldRun(const RelayChannel& channel, uint8_t dayOfWeek, uint16_t nowMinute) {
  for (uint8_t i = 0; i < MAX_SCHEDULES; ++i) {
    if (scheduleMatches(channel.schedules[i], dayOfWeek, nowMinute)) return true;
  }
  return false;
}
