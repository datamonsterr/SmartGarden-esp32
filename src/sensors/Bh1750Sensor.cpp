#include "sensors/Bh1750Sensor.h"

namespace sensors {

Bh1750Sensor::Bh1750Sensor() {}

void Bh1750Sensor::begin() {
  if (lightMeter_.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    initialized_ = true;
    Serial.println("BH1750 initialized");
  } else {
    Serial.println("Error initializing BH1750");
  }
}

float Bh1750Sensor::readLux() {
  if (!initialized_) {
    return -1.0f;
  }
  return lightMeter_.readLightLevel();
}

} // namespace sensors
