#include "inputs/Button.h"

namespace inputs {

Button::Button(uint8_t pin) : pin_(pin) {}

void Button::begin() {
  pinMode(pin_, INPUT_PULLUP);
  stableLevel_ = digitalRead(pin_) == HIGH;
  lastRawLevel_ = stableLevel_;
  lastChangeMs_ = millis();
}

bool Button::update(uint32_t nowMs) {
  const bool rawLevel = digitalRead(pin_) == HIGH;

  if (rawLevel != lastRawLevel_) {
    lastRawLevel_ = rawLevel;
    lastChangeMs_ = nowMs;
  }

  if (rawLevel != stableLevel_ && (nowMs - lastChangeMs_) >= kDebounceMs_) {
    const bool previousStable = stableLevel_;
    stableLevel_ = rawLevel;

    // Detect press edge: HIGH -> LOW
    if (previousStable == true && stableLevel_ == false) {
      return true;
    }
  }

  return false;
}

}  // namespace inputs
