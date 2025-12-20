#include "controllers/LightController.h"

namespace controllers {

LightController::LightController(actuators::RelayActuator& relay, uint32_t onAfterMotionMs, float tempHysteresisC)
    : relay_(relay), onAfterMotionMs_(onAfterMotionMs), tempHysteresisC_(tempHysteresisC) {}

void LightController::setOnAfterMotionMs(uint32_t onAfterMotionMs) {
  onAfterMotionMs_ = onAfterMotionMs;
}

void LightController::update(
    uint32_t nowMs,
    bool motionDetected,
    const sensors::DhtReading& dht,
    const app::Settings& settings) {
  state_.motionDetected = motionDetected;
  state_.manualOff = settings.manualOff();
  state_.remoteOverrideEnabled = settings.remoteOverrideEnabled();
  state_.tempLimitEnabled = settings.tempLimitEnabled();
  state_.tempTooColdC = settings.tempTooColdC();

  if (motionDetected) {
    state_.lastMotionMs = nowMs;
  }

  if (settings.tempLimitEnabled() && dht.ok) {
    const float onAtOrBelow = settings.tempTooColdC();
    const float offAtOrAbove = settings.tempTooColdC() + tempHysteresisC_;

    if (!tempRequestOn_ && dht.temperatureC <= onAtOrBelow) {
      tempRequestOn_ = true;
    } else if (tempRequestOn_ && dht.temperatureC >= offAtOrAbove) {
      tempRequestOn_ = false;
    }
  } else {
    tempRequestOn_ = false;
  }

  bool desiredOn = false;

  if (settings.manualOff()) {
    desiredOn = false;
  } else if (settings.remoteOverrideEnabled()) {
    desiredOn = settings.remoteLightOn();
  } else {
    const bool motionRequestOn = (nowMs - state_.lastMotionMs) <= onAfterMotionMs_;
    desiredOn = motionRequestOn || tempRequestOn_;
  }

  relay_.setOn(desiredOn);
  state_.lightOn = relay_.isOn();
}

LightState LightController::state() const {
  return state_;
}

}  // namespace controllers
