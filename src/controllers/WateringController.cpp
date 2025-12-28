#include "controllers/WateringController.h"

namespace controllers {

WateringController::WateringController(
    actuators::RelayActuator& valveRelay,
    uint32_t intervalMs,
    uint32_t durationMs)
    : valveRelay_(valveRelay),
      intervalMs_(intervalMs),
      durationMs_(durationMs) {}

void WateringController::setInterval(uint32_t intervalMs, uint32_t durationMs) {
  intervalMs_ = intervalMs;
  durationMs_ = durationMs;
}

void WateringController::update(uint32_t nowMs) {
  // Timer-based watering logic
  if (intervalMs_ > 0 && durationMs_ > 0) {
    if (!isWatering_) {
      // Check if it's time to start watering
      if (lastWateringStartMs_ == 0 || (nowMs - lastWateringStartMs_) >= intervalMs_) {
        isWatering_ = true;
        lastWateringStartMs_ = nowMs;
        valveRelay_.setOn(true);
        Serial.println("Watering: Timer START");
      }
    } else {
      // Check if watering duration has elapsed
      if ((nowMs - lastWateringStartMs_) >= durationMs_) {
        isWatering_ = false;
        valveRelay_.setOn(false);
        Serial.println("Watering: Timer STOP");
      }
    }
  } else {
    // Invalid config, turn off valve
    if (isWatering_) {
      isWatering_ = false;
      valveRelay_.setOn(false);
    }
  }

  // Update state
  state_.valveOn = valveRelay_.isOn();
  state_.lastWateringMs = lastWateringStartMs_;
  state_.nextWateringMs = lastWateringStartMs_ + intervalMs_;
}

WateringState WateringController::state() const {
  return state_;
}

} // namespace controllers

