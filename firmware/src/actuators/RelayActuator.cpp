#include "actuators/RelayActuator.h"

namespace actuators {

RelayActuator::RelayActuator(uint8_t pin, bool activeLow) : pin_(pin), activeLow_(activeLow) {}

void RelayActuator::begin() {
  pinMode(pin_, OUTPUT);
  setOn(false);
}

void RelayActuator::setOn(bool on) {
  on_ = on;
  const bool level = activeLow_ ? !on : on;
  digitalWrite(pin_, level ? HIGH : LOW);
}

bool RelayActuator::isOn() const {
  return on_;
}

}  // namespace actuators
