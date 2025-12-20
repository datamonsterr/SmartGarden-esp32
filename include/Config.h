#pragma once

#include <Arduino.h>

namespace config {

// ---- Device identity ----
// Used as part of MQTT clientId and for identifying logs.
constexpr const char* kDeviceName = "smart-garden-esp32";

// ---- Telemetry ----
constexpr uint32_t kTelemetryIntervalMs = 10000;
constexpr uint32_t kSensorReadIntervalMs = 2000;

// ---- Pins (change to match your wiring) ----
constexpr uint8_t kPinDht = 4;
constexpr uint8_t kPinPir = 27;

// ESP32 ADC pins (input-only is ok for sensors)
constexpr uint8_t kPinMq135Analog = 34;
constexpr uint8_t kPinSoilMoistureAnalog = 35;

// Relay control pins (active LOW is common; configurable below)
constexpr uint8_t kPinRelayLight = 26;
constexpr uint8_t kPinRelayValve = 25;

// Manual override button (wired to GND, uses INPUT_PULLUP)
constexpr uint8_t kPinLightManualButton = 14;

// ---- Relay electrical convention ----
constexpr bool kRelayActiveLow = true;

// ---- Motion-triggered light ----
constexpr uint32_t kLightOnAfterMotionMs = 60000;

// Temperature-based light request: if too cold, turn on to warm plants.
constexpr bool kTempLightEnabledByDefault = false;
constexpr float kTempTooColdCDefault = 18.0f;
constexpr float kTempLightHysteresisC = 0.5f;

// ---- Auto watering ----
// Soil moisture analog readings vary by sensor and soil. Calibrate and adjust.
// Convention in this project: higher value = wetter.
constexpr int kSoilWetThreshold = 2400;   // stop watering when >=
constexpr int kSoilDryThreshold = 1800;   // start watering when <=

constexpr uint32_t kMinValveOnMs = 10000;
constexpr uint32_t kMinValveOffMs = 30000;

}  // namespace config
