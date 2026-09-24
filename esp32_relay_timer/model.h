#pragma once

#include <Arduino.h>

enum class RelayMode : uint8_t {
  AUTO = 0,
  FORCE_ON = 1,
  FORCE_OFF = 2
};

struct RelaySchedule {
  bool enabled = false;
  uint8_t startHour = 8;
  uint8_t startMinute = 0;
  uint8_t endHour = 9;
  uint8_t endMinute = 0;
};

struct RelayChannel {
  String name;
  RelayMode mode = RelayMode::AUTO;
  RelaySchedule schedule;
  bool outputState = false;
};

inline bool scheduleIsActive(
  uint8_t nowHour,
  uint8_t nowMinute,
  const RelaySchedule& schedule
) {
  if (!schedule.enabled) return false;

  const int nowMinutes = nowHour * 60 + nowMinute;
  const int startMinutes = schedule.startHour * 60 + schedule.startMinute;
  const int endMinutes = schedule.endHour * 60 + schedule.endMinute;

  if (startMinutes == endMinutes) return false;

  if (startMinutes < endMinutes) {
    return nowMinutes >= startMinutes && nowMinutes < endMinutes;
  }

  // Cross-midnight schedule, e.g. 22:00 -> 06:00.
  return nowMinutes >= startMinutes || nowMinutes < endMinutes;
}
