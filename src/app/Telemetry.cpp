#include "app/Telemetry.h"

namespace app {

void Telemetry::updateSensors(
    const sensors::DhtReading& dht,
    bool motionDetected,
    int mq135Raw,
    int soilRaw) {
  dht_ = dht;
  motionDetected_ = motionDetected;
  mq135Raw_ = mq135Raw;
  soilRaw_ = soilRaw;
}

String Telemetry::buildTelemetryJson(const controllers::LightState& light, const controllers::WateringState& watering) const {
  JsonDocument doc;

  if (dht_.ok) {
    doc["temperature_c"] = dht_.temperatureC;
    doc["humidity_pct"] = dht_.humidityPct;
  } else {
    doc["temperature_c"] = nullptr;
    doc["humidity_pct"] = nullptr;
  }

  String out;
  serializeJson(doc, out);
  return out;
}

}  // namespace app
