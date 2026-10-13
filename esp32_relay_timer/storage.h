#pragma once
#include <Preferences.h>
#include "model.h"

namespace Storage {
  static Preferences prefs;

  inline void defaults(PersistedSettings& settings) {
    settings = PersistedSettings{};
    settings.automationEnabled = true;

    for (uint8_t i = 0; i < MAX_BELL_EVENTS; ++i) {
      settings.events[i] = BellEvent{};
      settings.events[i].enabled = false;
      settings.events[i].daysMask = SCHOOL_DAYS_MASK;
      settings.events[i].durationSec = DEFAULT_BELL_DURATION_SECONDS;
    }

    // Default timetable supplied for the school controller.
    static constexpr uint16_t defaultMinutes[] = {
      7 * 60 + 45,
      8 * 60,
      9 * 60,
      10 * 60,
      10 * 60 + 10,
      11 * 60,
      12 * 60,
      12 * 60 + 45,
      13 * 60,
      14 * 60,
      15 * 60,
      16 * 60
    };

    static_assert(
      sizeof(defaultMinutes) / sizeof(defaultMinutes[0]) <= MAX_BELL_EVENTS,
      "Default bell timetable exceeds MAX_BELL_EVENTS"
    );

    for (uint8_t i = 0; i < sizeof(defaultMinutes) / sizeof(defaultMinutes[0]); ++i) {
      settings.events[i].enabled = true;
      settings.events[i].daysMask = SCHOOL_DAYS_MASK;
      settings.events[i].minuteOfDay = defaultMinutes[i];
      settings.events[i].durationSec = DEFAULT_BELL_DURATION_SECONDS;
    }
  }

  inline bool validate(PersistedSettings& settings) {
    if (settings.magic != SETTINGS_MAGIC ||
        settings.schemaVersion != SETTINGS_SCHEMA_VERSION) {
      return false;
    }

    settings.automationEnabled = settings.automationEnabled ? true : false;

    for (uint8_t i = 0; i < MAX_BELL_EVENTS; ++i) {
      BellEvent& event = settings.events[i];
      event.daysMask &= 0x7F;

      if (!validMinuteOfDay(event.minuteOfDay)) {
        event.enabled = false;
        event.minuteOfDay = 8 * 60;
      }

      if (!validBellDuration(event.durationSec)) {
        event.durationSec = DEFAULT_BELL_DURATION_SECONDS;
      }
    }

    return true;
  }

  inline void begin(PersistedSettings& settings) {
    prefs.begin("relayctl", false);

    const size_t expected = sizeof(PersistedSettings);
    const size_t stored = prefs.getBytesLength("settings");

    if (stored != expected) {
      defaults(settings);
      prefs.putBytes("settings", &settings, expected);
      return;
    }

    prefs.getBytes("settings", &settings, expected);
    if (!validate(settings)) {
      defaults(settings);
      prefs.putBytes("settings", &settings, expected);
    }
  }

  inline bool save(const PersistedSettings& settings) {
    return prefs.putBytes("settings", &settings, sizeof(settings)) == sizeof(settings);
  }

  inline void factoryReset(PersistedSettings& settings) {
    prefs.clear();
    defaults(settings);
    save(settings);
  }
}
