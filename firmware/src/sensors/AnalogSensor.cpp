#include "sensors/AnalogSensor.h"

namespace sensors {

AnalogSensor::AnalogSensor(uint8_t pin) : pin_(pin) {}

void AnalogSensor::begin() {
  pinMode(pin_, INPUT);
}

int AnalogSensor::readRaw() const {
  return analogRead(pin_);
}

}  // namespace sensors
