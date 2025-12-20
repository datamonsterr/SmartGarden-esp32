#include "sensors/PirSensor.h"

namespace sensors {

PirSensor::PirSensor(uint8_t pin) : pin_(pin) {}

void PirSensor::begin() {
  pinMode(pin_, INPUT);
}

bool PirSensor::readMotion() const {
  return digitalRead(pin_) == HIGH;
}

}  // namespace sensors
