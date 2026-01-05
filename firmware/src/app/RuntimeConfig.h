#pragma once

#include <Arduino.h>

namespace app {

// Runtime-configurable parameters.
// Defaults come from include/Config.h; values may be overridden from
// ThingsBoard.
struct RuntimeConfig {
  uint32_t telemetryIntervalMs = 10000;
  uint32_t sensorReadIntervalMs = 2000;

  // Temperature-light feature
  bool tempLightEnabled = false;
  float tempTooColdC = 18.0f;

  // Watering (timer-based, no soil sensor)
  uint32_t minValveOnMs = 30000;
  uint32_t minValveOffMs = 3600000;

  // Remote light control from ThingsBoard
  bool selfLightEnable = true; // Default: allow automatic light control

  // Remote valve control
  bool selfValveEnable = true; // Default: allow automatic watering
};

} // namespace app
