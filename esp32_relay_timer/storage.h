#pragma once
#include <Preferences.h>
#include <cstring>
#include "model.h"

namespace Storage {
  static Preferences prefs;

  inline void defaults(PersistedSettings& settings) {
    settings = PersistedSettings{};
    const char* names[CHANNEL_COUNT] = {"Channel 1", "Channel 2", "Channel 3", "Channel 4"};
    for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
      strncpy(settings.channels[i].name, names[i], sizeof(settings.channels[i].name) - 1);
      settings.channels[i].mode = RelayMode::AUTO;
      settings.channels[i].outputState = false;
      for (uint8_t s = 0; s < MAX_SCHEDULES; ++s) {
        settings.channels[i].schedules[s] = ScheduleRule{};
        settings.channels[i].schedules[s].enabled = false;
      }
    }
  }

  inline bool validate(PersistedSettings& settings) {
    if (settings.magic != SETTINGS_MAGIC || settings.schemaVersion != SETTINGS_SCHEMA_VERSION) return false;

    for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
      settings.channels[i].name[sizeof(settings.channels[i].name) - 1] = '\0';
      settings.channels[i].outputState = false;
      if (!validMode(settings.channels[i].mode)) settings.channels[i].mode = RelayMode::AUTO;

      for (uint8_t s = 0; s < MAX_SCHEDULES; ++s) {
        auto& rule = settings.channels[i].schedules[s];
        rule.daysMask &= 0x7F;
        if (!validMinuteOfDay(rule.startMinute)) rule.startMinute = 8 * 60;
        if (!validMinuteOfDay(rule.endMinute)) rule.endMinute = 9 * 60;
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
