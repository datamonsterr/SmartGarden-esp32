#include "controllers/WateringController.h"

namespace controllers {

WateringController::WateringController(
    actuators::RelayActuator& valveRelay,
    sensors::AnalogSensor& soilSensor,
    int dryThreshold,
    int wetThreshold,
    uint32_t minOnMs,
    uint32_t minOffMs)
    : valveRelay_(valveRelay),
      soilSensor_(soilSensor),
      dryThreshold_(dryThreshold),
      wetThreshold_(wetThreshold),
      minOnMs_(minOnMs),
      minOffMs_(minOffMs) {}

void WateringController::setThresholds(int dryThreshold, int wetThreshold) {
  dryThreshold_ = dryThreshold;
  wetThreshold_ = wetThreshold;
}

void WateringController::setMinDurations(uint32_t minOnMs, uint32_t minOffMs) {
  minOnMs_ = minOnMs;
  minOffMs_ = minOffMs;
}

void WateringController::update(uint32_t nowMs) {
  state_.soilRaw = soilSensor_.readRaw();

  // Dry/Wet evaluation (with hysteresis thresholds)
  if (state_.soilRaw <= dryThreshold_) {
    state_.isDry = true;
  }
  if (state_.soilRaw >= wetThreshold_) {
    state_.isDry = false;
  }

  const bool valveCurrentlyOn = valveRelay_.isOn();
  const uint32_t elapsedSinceSwitch = nowMs - lastSwitchMs_;

  // Decide desired valve state (respect min on/off durations)
  if (state_.isDry) {
    if (!valveCurrentlyOn && elapsedSinceSwitch >= minOffMs_) {
      valveRelay_.setOn(true);
      lastSwitchMs_ = nowMs;
    }
  } else {
    if (valveCurrentlyOn && elapsedSinceSwitch >= minOnMs_) {
      valveRelay_.setOn(false);
      lastSwitchMs_ = nowMs;
    }
  }

  state_.valveOn = valveRelay_.isOn();
}

WateringState WateringController::state() const {
  return state_;
}

}  // namespace controllers
