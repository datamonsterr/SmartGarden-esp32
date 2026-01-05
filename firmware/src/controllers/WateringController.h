#pragma once

#include <Arduino.h>

#include "actuators/RelayActuator.h"

namespace controllers {

struct WateringState {
  bool valveOn = false;
};

class WateringController {
public:
  WateringController(actuators::RelayActuator &valveRelay);

  void update(uint32_t nowMs);
  WateringState state() const;

  // Server control via Shared Attribute
  void setSelfValveEnable(bool enabled);

private:
  actuators::RelayActuator &valveRelay_;
  bool selfValveEnable_ = false;
  WateringState state_;
};

} // namespace controllers
