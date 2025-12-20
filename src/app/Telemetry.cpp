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

  // Temperature & humidity
  if (dht_.ok) {
    doc["temperature_c"] = dht_.temperatureC;
    doc["humidity_pct"] = dht_.humidityPct;
  } else {
    doc["temperature_c"] = nullptr;
    doc["humidity_pct"] = nullptr;
  }

  // Motion
  doc["motion"] = motionDetected_;

  // Air quality (raw ADC)
  doc["air_quality_raw"] = mq135Raw_;

  // Soil moisture
  doc["soil_raw"] = soilRaw_;
  doc["soil_is_dry"] = watering.isDry;

  // Light state
  doc["light_on"] = light.lightOn;
  doc["light_remote_override"] = light.remoteOverrideEnabled;
  doc["light_temp_limit_enabled"] = light.tempLimitEnabled;
  if (!isnan(light.tempTooColdC)) {
    doc["light_temp_too_cold_c"] = light.tempTooColdC;
  }

  // Valve state
  doc["valve_on"] = watering.valveOn;

  String out;
  serializeJson(doc, out);
  return out;
}

}  // namespace app
