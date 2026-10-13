#pragma once
#include <Preferences.h>
#include <cstring>
#include "model.h"

namespace Storage {
  static Preferences prefs;

  inline void defaults(PersistedSettings& settings) {
    settings = PersistedSettings{};
    settings.automationEnabled = true;

    static constexpr uint16_t DEFAULT_TIMES[MAX_BELL_EVENTS] = {
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

    for (uint8_t i = 0; i < MAX_BELL_EVENTS; ++i) {
      settings.events[i].enabled = true;
      settings.events[i].daysMask = 0x1F; // Sunday through Thursday
      settings.events[i].minuteOfDay = DEFAULT_TIMES[i];
      settings.events[i].durationSeconds = DEFAULT_BELL_DURATION_SECONDS;
    }
  }

  inline bool validate(PersistedSettings& settings) {
    if (settings.magic != SETTINGS_MAGIC || settings.schemaVersion != SETTINGS_SCHEMA_VERSION) {
      return false;
    }

    for (uint8_t i = 0; i < MAX_BELL_EVENTS; ++i) {
      BellEvent& event = settings.events[i];
      event.daysMask &= 0x7F;

      if (!validMinuteOfDay(event.minuteOfDay)) {
        return false;
      }

      if (event.durationSeconds < 1 || event.durationSeconds > MAX_BELL_DURATION_SECONDS) {
        event.durationSeconds = DEFAULT_BELL_DURATION_SECONDS;
      }
    }

    return true;
  }

  inline void begin(PersistedSettings& settings) {
    prefs.begin("schoolbell", false);

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

  inline void loadLastTrigger(uint32_t& stamp, uint8_t& slot) {
    stamp = prefs.getUInt("laststamp", 0);
    slot = prefs.getUChar("lastslot", 0xFF);
  }

  inline void saveLastTrigger(uint32_t stamp, uint8_t slot) {
    prefs.putUInt("laststamp", stamp);
    prefs.putUChar("lastslot", slot);
  }

  inline void clearLastTrigger() {
    prefs.remove("laststamp");
    prefs.remove("lastslot");
  }
}
