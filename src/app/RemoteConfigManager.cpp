#include "app/RemoteConfigManager.h"

#include <Preferences.h>

namespace app {

namespace {

static const char* kPrefsNamespace = "sg_cfg";

}  // namespace

RemoteConfigManager::RemoteConfigManager(
    RuntimeConfig& runtimeConfig,
    Settings& settings,
    controllers::LightController& light,
    controllers::WateringController& watering)
    : config_(runtimeConfig), settings_(settings), light_(light), watering_(watering) {}

void RemoteConfigManager::begin() {
  // Load last known config (if any). If not present, runtime config stays at defaults.
  loadFromNvs_();
  applyToControllers_();

  // Keep Settings in sync with runtime config defaults.
  settings_.setTempLimitEnabled(config_.tempLightEnabled);
  settings_.setTempTooColdC(config_.tempTooColdC);
}

const char* RemoteConfigManager::sharedKeysCsv() {
  // Keep this stable so dashboards / attributes are easy to manage.
  return "telemetryIntervalMs,sensorReadIntervalMs,lightOnAfterMotionMs,tempLightEnabled,tempTooColdC,minValveOnMs,minValveOffMs";
}

bool RemoteConfigManager::applyAttributes(JsonVariantConst root) {
  changed_ = false;

  // TB attribute payload can be:
  // - {"shared":{...}}
  // - {"client":{...}}
  // - or directly {"key":value,...} for update messages
  JsonVariantConst obj = root;
  if (root.is<JsonObjectConst>()) {
    const JsonObjectConst rootObj = root.as<JsonObjectConst>();
    if (rootObj.containsKey("shared")) {
      obj = rootObj["shared"];
    } else if (rootObj.containsKey("client")) {
      obj = rootObj["client"];
    }
  }

  if (!obj.is<JsonObjectConst>()) {
    return false;
  }

  const JsonObjectConst cfg = obj.as<JsonObjectConst>();

  maybeSetU32_(cfg, "telemetryIntervalMs", config_.telemetryIntervalMs);
  maybeSetU32_(cfg, "sensorReadIntervalMs", config_.sensorReadIntervalMs);
  maybeSetU32_(cfg, "lightOnAfterMotionMs", config_.lightOnAfterMotionMs);

  maybeSetBool_(cfg, "tempLightEnabled", config_.tempLightEnabled);
  maybeSetFloat_(cfg, "tempTooColdC", config_.tempTooColdC);

  maybeSetU32_(cfg, "minValveOnMs", config_.minValveOnMs);
  maybeSetU32_(cfg, "minValveOffMs", config_.minValveOffMs);

  // Safety clamps (avoid breaking sensors/logic via bad server values)
  if (config_.sensorReadIntervalMs < 2000) {
    config_.sensorReadIntervalMs = 2000;
    changed_ = true;
  }
  if (config_.telemetryIntervalMs < 1000) {
    config_.telemetryIntervalMs = 1000;
    changed_ = true;
  }

  if (changed_) {
    applyToControllers_();

    settings_.setTempLimitEnabled(config_.tempLightEnabled);
    settings_.setTempTooColdC(config_.tempTooColdC);

    saveToNvs_();
  }

  return changed_;
}

void RemoteConfigManager::applyToControllers_() {
  light_.setOnAfterMotionMs(config_.lightOnAfterMotionMs);
  // Watering controller now only needs interval/duration, not thresholds
}

void RemoteConfigManager::maybeSetU32_(JsonVariantConst obj, const char* key, uint32_t& dst) {
  if (!obj.containsKey(key)) {
    return;
  }
  const uint32_t v = obj[key].as<uint32_t>();
  if (v != dst) {
    dst = v;
    changed_ = true;
  }
}

void RemoteConfigManager::maybeSetI32_(JsonVariantConst obj, const char* key, int& dst) {
  if (!obj.containsKey(key)) {
    return;
  }
  const int v = obj[key].as<int>();
  if (v != dst) {
    dst = v;
    changed_ = true;
  }
}

void RemoteConfigManager::maybeSetBool_(JsonVariantConst obj, const char* key, bool& dst) {
  if (!obj.containsKey(key)) {
    return;
  }
  const bool v = obj[key].as<bool>();
  if (v != dst) {
    dst = v;
    changed_ = true;
  }
}

void RemoteConfigManager::maybeSetFloat_(JsonVariantConst obj, const char* key, float& dst) {
  if (!obj.containsKey(key)) {
    return;
  }
  const float v = obj[key].as<float>();
  if (isnan(dst) || fabsf(v - dst) > 0.0001f) {
    dst = v;
    changed_ = true;
  }
}

bool RemoteConfigManager::loadFromNvs_() {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, true)) {
    return false;
  }

  const bool has = prefs.getBool("has", false);
  if (!has) {
    prefs.end();
    return false;
  }

  config_.telemetryIntervalMs = prefs.getUInt("tel_ms", config_.telemetryIntervalMs);
  config_.sensorReadIntervalMs = prefs.getUInt("sen_ms", config_.sensorReadIntervalMs);
  config_.lightOnAfterMotionMs = prefs.getUInt("lgt_ms", config_.lightOnAfterMotionMs);

  config_.tempLightEnabled = prefs.getBool("tmp_en", config_.tempLightEnabled);
  config_.tempTooColdC = prefs.getFloat("tmp_c", config_.tempTooColdC);

  // Soil sensor removed - no longer load thresholds
  config_.minValveOnMs = prefs.getUInt("v_on", config_.minValveOnMs);
  config_.minValveOffMs = prefs.getUInt("v_off", config_.minValveOffMs);

  prefs.end();
  return true;
}

void RemoteConfigManager::saveToNvs_() {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, false)) {
    return;
  }

  prefs.putBool("has", true);

  prefs.putUInt("tel_ms", config_.telemetryIntervalMs);
  prefs.putUInt("sen_ms", config_.sensorReadIntervalMs);
  prefs.putUInt("lgt_ms", config_.lightOnAfterMotionMs);

  prefs.putBool("tmp_en", config_.tempLightEnabled);
  prefs.putFloat("tmp_c", config_.tempTooColdC);

  // Soil sensor removed - no longer save thresholds
  prefs.putUInt("v_on", config_.minValveOnMs);
  prefs.putUInt("v_off", config_.minValveOffMs);

  prefs.end();
}

}  // namespace app
