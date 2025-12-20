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
      int soilRaw);

  // Returns a compact JSON object string (fits in PubSubClient buffer).
  String buildTelemetryJson(const controllers::LightState& light, const controllers::WateringState& watering) const;

 private:
  sensors::DhtReading dht_;
  bool motionDetected_ = false;
  int mq135Raw_ = -1;
  int soilRaw_ = -1;
};

}  // namespace app
