#pragma once

#include <Arduino.h>

namespace sensors {

struct DhtReading {
  bool ok = false;
  float temperatureC = NAN;
  float humidityPct = NAN;
};

class DhtSensor {
 public:
  explicit DhtSensor(uint8_t pin);

  void begin();
  DhtReading read();

 private:
  const uint8_t pin_;

  // Opaque pointer to avoid leaking DHT include into all translation units.
  void* dht_ = nullptr;
};

}  // namespace sensors
