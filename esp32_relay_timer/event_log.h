#pragma once
#include <Arduino.h>

struct EventRecord {
  String time;
  String message;
};

class EventLog {
public:
  static constexpr uint8_t CAPACITY = 24;

  void add(const String& time, const String& message) {
    records_[head_] = {time, message};
    head_ = (head_ + 1) % CAPACITY;
    if (count_ < CAPACITY) ++count_;
  }

  uint8_t count() const { return count_; }

  const EventRecord& newest(uint8_t offset) const {
    int index = static_cast<int>(head_) - 1 - offset;
    while (index < 0) index += CAPACITY;
    return records_[index % CAPACITY];
  }

  void clear() {
    head_ = 0;
    count_ = 0;
  }

private:
  EventRecord records_[CAPACITY];
  uint8_t head_ = 0;
  uint8_t count_ = 0;
};
