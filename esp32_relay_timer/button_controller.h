#pragma once
#include <Arduino.h>
#include "config.h"

enum class ButtonAction : uint8_t { NONE, CLOCK, UP, DOWN, TIMER, BACK };

class ButtonController {
public:
  void begin() {
    const uint8_t pins[] = {BTN_CLOCK_PIN, BTN_UP_PIN, BTN_DOWN_PIN, BTN_TIMER_PIN, BTN_BACK_PIN};
    for (uint8_t pin : pins) pinMode(pin, INPUT_PULLUP);
    for (auto& state : states_) state.lastStable = HIGH;
  }

  ButtonAction poll() {
    const uint8_t pins[5] = {BTN_CLOCK_PIN, BTN_UP_PIN, BTN_DOWN_PIN, BTN_TIMER_PIN, BTN_BACK_PIN};
    const ButtonAction actions[5] = {
      ButtonAction::CLOCK, ButtonAction::UP, ButtonAction::DOWN, ButtonAction::TIMER, ButtonAction::BACK
    };

    const unsigned long now = millis();

    for (uint8_t i = 0; i < 5; ++i) {
      const uint8_t reading = digitalRead(pins[i]);

      if (reading != states_[i].lastReading) {
        states_[i].changedAt = now;
        states_[i].lastReading = reading;
      }

      if ((now - states_[i].changedAt) >= BUTTON_DEBOUNCE_MS && reading != states_[i].lastStable) {
        states_[i].lastStable = reading;
        if (reading == LOW) return actions[i];
      }
    }

    return ButtonAction::NONE;
  }

private:
  struct State {
    uint8_t lastReading = HIGH;
    uint8_t lastStable = HIGH;
    unsigned long changedAt = 0;
  };

  State states_[5];
};
