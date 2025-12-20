#include "sensors/DhtSensor.h"

#include <DHT.h>

namespace sensors {

namespace {
static const uint8_t kDhtType = DHT22;
}

DhtSensor::DhtSensor(uint8_t pin) : pin_(pin) {}

void DhtSensor::begin() {
  // DHT library expects the object to live for program lifetime.
  dht_ = new DHT(pin_, kDhtType);
  static_cast<DHT*>(dht_)->begin();
}

DhtReading DhtSensor::read() {
  DhtReading out;
  if (dht_ == nullptr) {
    return out;
  }

  auto* dht = static_cast<DHT*>(dht_);
  const float humidity = dht->readHumidity();
  const float temperature = dht->readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    return out;
  }

  out.ok = true;
  out.humidityPct = humidity;
  out.temperatureC = temperature;
  return out;
}

}  // namespace sensors
