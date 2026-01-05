#pragma once

#include <Arduino.h>

namespace sensors {

class PirSensor {
 public:
  explicit PirSensor(uint8_t pin);

  void begin();
  bool readMotion() const;

 private:
  const uint8_t pin_;
};

}  // namespace sensors
