#include "controllers/LightController.h"

namespace controllers {

LightController::LightController(actuators::RelayActuator& relay, float tempHysteresisC)
    : relay_(relay), tempHysteresisC_(tempHysteresisC) {}

void LightController::update(
    const sensors::DhtReading& dht,
    const app::Settings& settings) {
  state_.manualOff = settings.manualOff();
  state_.remoteOverrideEnabled = settings.remoteOverrideEnabled();
  state_.tempLimitEnabled = settings.tempLimitEnabled();
  state_.tempTooColdC = settings.tempTooColdC();

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
    // Auto mode: turn on if temperature is too cold for plants
    desiredOn = tempRequestOn_;
  }

  relay_.setOn(desiredOn);
  state_.lightOn = relay_.isOn();
}

LightState LightController::state() const {
  return state_;
}

}  // namespace controllers
