#pragma once

#include <Arduino.h>

#include "actuators/RelayActuator.h"
#include "app/Settings.h"
#include "sensors/DhtSensor.h"

namespace controllers {

struct LightState {
  bool lightOn = false;
  bool motionDetected = false;
  uint32_t lastMotionMs = 0;

  // For telemetry/debugging
  bool manualOff = false;
  bool remoteOverrideEnabled = false;
  bool tempLimitEnabled = false;
  float tempTooColdC = NAN;
};

class LightController {
 public:
  LightController(actuators::RelayActuator& relay, uint32_t onAfterMotionMs, float tempHysteresisC);

  void update(
      uint32_t nowMs,
      bool motionDetected,
      const sensors::DhtReading& dht,
      const app::Settings& settings);
  LightState state() const;

  void setOnAfterMotionMs(uint32_t onAfterMotionMs);

 private:
  actuators::RelayActuator& relay_;
  uint32_t onAfterMotionMs_;
  float tempHysteresisC_;

  LightState state_;
  bool tempRequestOn_ = false;
};

}  // namespace controllers
