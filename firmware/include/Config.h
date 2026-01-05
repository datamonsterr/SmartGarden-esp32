#pragma once

#include <Arduino.h>

namespace config {

// ---- Device identity ----
// Used as part of MQTT clientId and for identifying logs.
constexpr const char *kDeviceName = "smart-garden-esp32";

// ---- Telemetry ----
constexpr uint32_t kTelemetryIntervalMs = 10000;
constexpr uint32_t kSensorReadIntervalMs = 5000;  // 5 seconds (easier to read logs)

// ---- Pins (change to match your wiring) ----
constexpr uint8_t kPinDht = 4;
constexpr uint8_t kPinPir = 27;

// ESP32 ADC pins (input-only is ok for sensors)
constexpr uint8_t kPinMq135Analog = 34;
// Soil moisture sensor removed - using timer-based watering instead

// Relay control pins (active LOW is common; configurable below)
constexpr uint8_t kPinRelayLight = 26;
constexpr uint8_t kPinRelayValve = 25;

// Manual override button (wired to GND, uses INPUT_PULLUP)
constexpr uint8_t kPinLightManualButton = 14;

// ---- I2C for RTC (DS1307) ----
constexpr uint8_t kPinI2cSda = 21;
constexpr uint8_t kPinI2cScl = 22;

// ---- Relay electrical convention ----
constexpr bool kRelayActiveLow = false;  // Try Active HIGH if LED doesn't light

// ---- Motion-triggered light ----
constexpr uint32_t kLightOnAfterMotionMs = 60000;

// Temperature-based light request: if too cold, turn on to warm plants.
constexpr bool kTempLightEnabledByDefault = false;
constexpr float kTempTooColdCDefault = 18.0f;
constexpr float kTempLightHysteresisC = 0.5f;

// ---- Auto watering ----
// Timer-based watering (no soil sensor)
constexpr uint32_t kMinValveOnMs = 30000;   // 30 seconds
constexpr uint32_t kMinValveOffMs = 60000;  // 1 minute

constexpr uint32_t kWateringIntervalMs = 60000; // 1 minute (for testing)
constexpr uint32_t kWateringDurationMs = 30000; // 30 seconds

} // namespace config
