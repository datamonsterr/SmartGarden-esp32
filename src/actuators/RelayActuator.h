#pragma once

#include <Arduino.h>

namespace actuators {

class RelayActuator {
 public:
  RelayActuator(uint8_t pin, bool activeLow);

  void begin();
  void setOn(bool on);
  bool isOn() const;

 private:
  const uint8_t pin_;
  const bool activeLow_;
  bool on_ = false;
};

}  // namespace actuators
