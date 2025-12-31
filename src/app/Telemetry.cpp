#include "app/Telemetry.h"

namespace app {

void Telemetry::updateSensors(
    const sensors::DhtReading& dht,
    bool motionDetected,
    int mq135Raw,
    float lightLux) {
  dht_ = dht;
  motionDetected_ = motionDetected;
  mq135Raw_ = mq135Raw;
  lightLux_ = lightLux;
}

String Telemetry::buildTelemetryJson(const controllers::LightState& light, const controllers::WateringState& watering, bool selfLightEnable, bool selfValveEnable) const {
  JsonDocument doc;

  // ========== REQUIRED BY THINGSBOARD RULE CHAIN ==========
  // Server's Rule Chain filters on "temperature_c" to trigger automation.
  // This field MUST be present and use exact key name "temperature_c".
  // ========================================================
  
  // DHT22 sensor data
  if (dht_.ok) {
    doc["temperature_c"] = dht_.temperatureC;  // ⚠️ CRITICAL: Server depends on this key
    doc["humidity_pct"] = dht_.humidityPct;
  } else {
    doc["temperature_c"] = nullptr;
    doc["humidity_pct"] = nullptr;
  }

  // Motion sensor (for monitoring/telemetry only, not used in automation)
  doc["motion"] = motionDetected_;

  // Air quality (MQ135)
  doc["air_quality_raw"] = mq135Raw_;

  // Light intensity (BH1750)
  if (lightLux_ >= 0) {
    doc["light_lux"] = lightLux_;
  } else {
    doc["light_lux"] = nullptr;
  }

  // Light controller state
  doc["light_on"] = light.lightOn;
  doc["manual_off"] = light.manualOff;
  doc["self_light_enable"] = selfLightEnable;

  // Watering controller state
  doc["valve_on"] = watering.valveOn;
  doc["self_valve_enable"] = selfValveEnable;

  String out;
  serializeJson(doc, out);
  return out;
}

}  // namespace app
