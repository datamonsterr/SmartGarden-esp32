#pragma once

#include <Arduino.h>

namespace app {

// Runtime-configurable parameters.
// Defaults come from include/Config.h; values may be overridden from ThingsBoard.
struct RuntimeConfig {
  uint32_t telemetryIntervalMs = 10000;
  uint32_t sensorReadIntervalMs = 2000;

  uint32_t lightOnAfterMotionMs = 60000;

  // Temperature-light feature
  bool tempLightEnabled = false;
  float tempTooColdC = 18.0f;

  // Watering (timer-based, no soil sensor)
  uint32_t minValveOnMs = 30000;
  uint32_t minValveOffMs = 3600000;
};

}  // namespace app
