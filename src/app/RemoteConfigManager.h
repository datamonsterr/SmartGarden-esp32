#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "app/RuntimeConfig.h"
#include "app/Settings.h"
#include "controllers/LightController.h"
#include "controllers/WateringController.h"

namespace app {

class RemoteConfigManager {
 public:
  RemoteConfigManager(
      RuntimeConfig& runtimeConfig,
      Settings& settings,
      controllers::LightController& light,
      controllers::WateringController& watering);

  void begin();

  // Apply attributes payload from ThingsBoard (response or update).
  // Returns true if any setting was changed.
  bool applyAttributes(JsonVariantConst root);

  // Create a comma-separated list of shared-attribute keys to request.
  static const char* sharedKeysCsv();

 private:
  RuntimeConfig& config_;
  Settings& settings_;
  controllers::LightController& light_;
  controllers::WateringController& watering_;

  bool loadFromNvs_();
  void saveToNvs_();

  bool changed_ = false;

  void applyToControllers_();

  // Helpers
  void maybeSetU32_(JsonVariantConst obj, const char* key, uint32_t& dst);
  void maybeSetI32_(JsonVariantConst obj, const char* key, int& dst);
  void maybeSetBool_(JsonVariantConst obj, const char* key, bool& dst);
  void maybeSetFloat_(JsonVariantConst obj, const char* key, float& dst);
};

}  // namespace app
