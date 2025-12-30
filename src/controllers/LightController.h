#pragma once

#include <Arduino.h>

#include "actuators/RelayActuator.h"
#include "app/Settings.h"
#include "sensors/DhtSensor.h"

namespace controllers {

struct LightState {
  bool lightOn = false;
  bool motionDetected = false;  // For telemetry only

  // For telemetry/debugging
  bool manualOff = false;
  bool remoteOverrideEnabled = false;
  bool tempLimitEnabled = false;
  float tempTooColdC = NAN;
};

class LightController {
 public:
  LightController(actuators::RelayActuator& relay, float tempHysteresisC);

  void update(
      uint32_t nowMs,
      bool motionDetected,
      const sensors::DhtReading& dht,
      const app::Settings& settings);
  LightState state() const;

 private:
  actuators::RelayActuator& relay_;
  float tempHysteresisC_;

  LightState state_;
  bool tempRequestOn_ = false;
};

}  // namespace controllers
