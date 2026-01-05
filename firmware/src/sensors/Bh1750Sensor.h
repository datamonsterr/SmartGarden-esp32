#pragma once

#include <Arduino.h>
#include <BH1750.h>

namespace sensors {

class Bh1750Sensor {
 public:
  Bh1750Sensor();

  void begin();
  float readLux();
  bool isOk() const { return initialized_; }

 private:
  BH1750 lightMeter_;
  bool initialized_ = false;
};

} // namespace sensors
