#pragma once

#include <Arduino.h>

#include "actuators/RelayActuator.h"
#include "sensors/AnalogSensor.h"

namespace controllers {

struct WateringState {
  bool valveOn = false;
  uint32_t lastWateringMs = 0;
  uint32_t nextWateringMs = 0;
};

class WateringController {
 public:
  WateringController(
      actuators::RelayActuator& valveRelay,
      uint32_t intervalMs,
      uint32_t durationMs);

  void update(uint32_t nowMs);
  WateringState state() const;

  void setInterval(uint32_t intervalMs, uint32_t durationMs);

 private:
  actuators::RelayActuator& valveRelay_;

  uint32_t intervalMs_;
  uint32_t durationMs_;
  uint32_t lastWateringStartMs_ = 0;
  bool isWatering_ = false;

  WateringState state_;
};

} // namespace controllers
