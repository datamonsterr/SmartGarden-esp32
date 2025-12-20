#include "sensors/DhtSensor.h"

#include <DHTesp.h>

namespace sensors {

// Static DHTesp instance to ensure it persists
static DHTesp dhtSensor;

DhtSensor::DhtSensor(uint8_t pin) : pin_(pin) {}

void DhtSensor::begin() {
  dhtSensor.setup(pin_, DHTesp::DHT22);
  dht_ = &dhtSensor;
}

DhtReading DhtSensor::read() {
  DhtReading out;
  if (dht_ == nullptr) {
    return out;
  }

  DHTesp* dht = static_cast<DHTesp*>(dht_);
  
  float humidity = dht->getHumidity();
  float temperature = dht->getTemperature();

  if (dht->getStatus() != DHTesp::ERROR_NONE) {
    return out;
  }

  if (isnan(humidity) || isnan(temperature)) {
    return out;
  }

  out.ok = true;
  out.humidityPct = humidity;
  out.temperatureC = temperature;
  return out;
}

}  // namespace sensors
