#pragma once

#include <Arduino.h>

namespace sensors {

// Simple wrapper for ESP32 ADC reads.
class AnalogSensor {
 public:
  explicit AnalogSensor(uint8_t pin);

  void begin();
  int readRaw() const;

 private:
  const uint8_t pin_;
};

}  // namespace sensors
