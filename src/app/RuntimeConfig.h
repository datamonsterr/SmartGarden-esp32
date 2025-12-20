#pragma once

#include <Arduino.h>

namespace app {

// Runtime-configurable parameters.
// Defaults come from include/Config.h; values may be overridden from ThingsBoard.
struct RuntimeConfig {
  uint32_t telemetryIntervalMs = 10000;
  uint32_t sensorReadIntervalMs = 2000;

  // Temperature-light feature
  bool tempLightEnabled = false;
  float tempTooColdC = 18.0f;

  // Watering
  int soilWetThreshold = 2400;
  int soilDryThreshold = 1800;
  uint32_t minValveOnMs = 10000;
  uint32_t minValveOffMs = 30000;
};

}  // namespace app
