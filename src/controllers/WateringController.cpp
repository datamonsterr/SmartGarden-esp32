#include "controllers/WateringController.h"

namespace controllers {

WateringController::WateringController(actuators::RelayActuator &valveRelay,
                                       uint32_t intervalMs, uint32_t durationMs)
    : valveRelay_(valveRelay), intervalMs_(intervalMs),
      durationMs_(durationMs) {}

void WateringController::setInterval(uint32_t intervalMs, uint32_t durationMs) {
  intervalMs_ = intervalMs;
  durationMs_ = durationMs;
}

void WateringController::update(uint32_t nowMs) {
  // Feedback logic: Force ON for 2s
  if (nowMs - feedbackStartMs_ < 2000) {
    valveRelay_.setOn(true);
    state_.valveOn = true;
    // Don't mess with timer state during feedback, just force output
  } else {
    // Determine if timer should be running logic
    bool timerAllows = false;

    // Normal Timer-based watering logic
    if (intervalMs_ > 0 && durationMs_ > 0) {
      if (!isWatering_) {
        // Check if it's time to start watering
        if (lastWateringStartMs_ == 0 ||
            (nowMs - lastWateringStartMs_) >= intervalMs_) {
          isWatering_ = true;
          lastWateringStartMs_ = nowMs;
          Serial.println("Watering: Timer START");
        }
      } else {
        // Check if watering duration has elapsed
        if ((nowMs - lastWateringStartMs_) >= durationMs_) {
          isWatering_ = false;
          Serial.println("Watering: Timer STOP");
        }
      }
      // If isWatering_ is true, we want the valve ON
      timerAllows = isWatering_;
    } else {
      // Invalid config, ensure we aren't "watering"
      isWatering_ = false;
    }

    // Apply to relay
    // Note: If enabled_ is false, we still run the *timer logic* (updating
    // states/logs) AND we still actuate the relay as per user request "trigger
    // 30s ON".
    valveRelay_.setOn(timerAllows);
  }

  // Update state (read back from relay)
  state_.valveOn = valveRelay_.isOn();
  state_.lastWateringMs = lastWateringStartMs_;
  state_.nextWateringMs = lastWateringStartMs_ + intervalMs_;
}

WateringState WateringController::state() const { return state_; }

void WateringController::setSelfValveEnable(bool enabled) {
  enabled_ = enabled;
}

void WateringController::triggerFeedback() { feedbackStartMs_ = millis(); }

} // namespace controllers
