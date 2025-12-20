#pragma once

#include <Arduino.h>

namespace inputs {

class Button {
 public:
  explicit Button(uint8_t pin);

  void begin();

  // Call frequently from loop(). Returns true once per physical press.
  bool update(uint32_t nowMs);

 private:
  const uint8_t pin_;

  bool stableLevel_ = true;      // HIGH = not pressed (INPUT_PULLUP)
  bool lastRawLevel_ = true;
  uint32_t lastChangeMs_ = 0;

  static constexpr uint32_t kDebounceMs_ = 30;
};

}  // namespace inputs
