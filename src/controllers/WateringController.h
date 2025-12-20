#pragma once

#include <Arduino.h>

#include "actuators/RelayActuator.h"
#include "sensors/AnalogSensor.h"

namespace controllers {

struct WateringState {
  bool valveOn = false;
  int soilRaw = -1;
  bool isDry = false;
};

class WateringController {
 public:
  WateringController(
      actuators::RelayActuator& valveRelay,
      sensors::AnalogSensor& soilSensor,
      int dryThreshold,
      int wetThreshold,
      uint32_t minOnMs,
      uint32_t minOffMs);

  void update(uint32_t nowMs);
  WateringState state() const;

  void setThresholds(int dryThreshold, int wetThreshold);
  void setMinDurations(uint32_t minOnMs, uint32_t minOffMs);

 private:
  actuators::RelayActuator& valveRelay_;
  sensors::AnalogSensor& soilSensor_;

  int dryThreshold_;
  int wetThreshold_;
  uint32_t minOnMs_;
  uint32_t minOffMs_;

  WateringState state_;
  uint32_t lastSwitchMs_ = 0;
};

}  // namespace controllers
