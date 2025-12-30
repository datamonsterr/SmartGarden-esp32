#pragma once

#include <Arduino.h>

#include <ArduinoJson.h>

#include "controllers/LightController.h"
#include "controllers/WateringController.h"
#include "sensors/DhtSensor.h"

namespace app {

class Telemetry {
 public:
  void updateSensors(
      const sensors::DhtReading& dht,
      bool motionDetected,
      int mq135Raw,
      float lightLux);

  // Returns a compact JSON object string (fits in PubSubClient buffer).
  String buildTelemetryJson(const controllers::LightState& light, const controllers::WateringState& watering, bool selfLightEnable) const;

 private:
  sensors::DhtReading dht_;
  bool motionDetected_ = false;
  int mq135Raw_ = -1;
  float lightLux_ = -1.0f;
};

}  // namespace app
