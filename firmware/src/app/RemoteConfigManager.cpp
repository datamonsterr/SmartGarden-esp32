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
  settings_.setSelfLightEnable(config_.selfLightEnable);
}

const char* RemoteConfigManager::sharedKeysCsv() {
  // Keep this stable so dashboards / attributes are easy to manage.
  return "telemetryIntervalMs,sensorReadIntervalMs,tempLightEnabled,tempTooColdC,minValveOnMs,minValveOffMs,self_light_enable,self_valve_enable";
}

bool RemoteConfigManager::applyAttributes(JsonVariantConst root) {
  changed_ = false;

  // ========== THINGSBOARD SHARED ATTRIBUTES ==========
  // ESP32 hoạt động như "Dumb Device":
  // - Gửi temperature_c lên Server
  // - Server phân tích và set self_light_enable = true/false
  // - ESP32 nhận và bật/tắt đèn theo lệnh
  //
  // TB attribute payload formats:
  // - {"shared":{...}}        (when requesting attributes)
  // - {"client":{...}}        (client-side attributes)
  // - {"key":value,...}       (direct update notification)
  // ===================================================

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

  maybeSetBool_(cfg, "tempLightEnabled", config_.tempLightEnabled);
  maybeSetFloat_(cfg, "tempTooColdC", config_.tempTooColdC);

  maybeSetU32_(cfg, "minValveOnMs", config_.minValveOnMs);
  maybeSetU32_(cfg, "minValveOffMs", config_.minValveOffMs);

  // ⚠️ CRITICAL: self_light_enable từ ThingsBoard Rule Chain
  // Server tự động set attribute này dựa trên nhiệt độ
  Serial.print("🔍 Checking self_light_enable in attributes... ");
  if (cfg.containsKey("self_light_enable")) {
    const bool newVal = cfg["self_light_enable"].as<bool>();
    Serial.print("Found: ");
    Serial.print(newVal ? "TRUE" : "FALSE");
    Serial.print(" (current: ");
    Serial.print(config_.selfLightEnable ? "TRUE" : "FALSE");
    Serial.println(")");
  } else {
    Serial.println("NOT FOUND in attributes payload!");
  }
  maybeSetBool_(cfg, "self_light_enable", config_.selfLightEnable);

  // self_valve_enable: Server controls watering valve
  Serial.print("🔍 Checking self_valve_enable in attributes... ");
  if (cfg.containsKey("self_valve_enable")) {
    const bool newVal = cfg["self_valve_enable"].as<bool>();
    Serial.print("Found: ");
    Serial.print(newVal ? "TRUE" : "FALSE");
    Serial.print(" (current: ");
    Serial.print(config_.selfValveEnable ? "TRUE" : "FALSE");
    Serial.println(")");
  } else {
    Serial.println("NOT FOUND in attributes payload!");
  }
  maybeSetBool_(cfg, "self_valve_enable", config_.selfValveEnable);

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
    
    // Sync self_light_enable to Settings immediately
    settings_.setSelfLightEnable(config_.selfLightEnable);
    
    // Sync self_valve_enable to Settings and WateringController
    settings_.setSelfValveEnable(config_.selfValveEnable);
    watering_.setSelfValveEnable(config_.selfValveEnable);

    saveToNvs_();
  }

  return changed_;
}

void RemoteConfigManager::applyToControllers_() {
  // No motion-based control anymore
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

  config_.tempLightEnabled = prefs.getBool("tmp_en", config_.tempLightEnabled);
  config_.tempTooColdC = prefs.getFloat("tmp_c", config_.tempTooColdC);

  // Soil sensor removed - no longer load thresholds
  config_.minValveOnMs = prefs.getUInt("v_on", config_.minValveOnMs);
  config_.minValveOffMs = prefs.getUInt("v_off", config_.minValveOffMs);

  config_.selfLightEnable = prefs.getBool("slf_lgt", config_.selfLightEnable);
  config_.selfValveEnable = prefs.getBool("slf_vlv", config_.selfValveEnable);

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

  prefs.putBool("tmp_en", config_.tempLightEnabled);
  prefs.putFloat("tmp_c", config_.tempTooColdC);

  // Soil sensor removed - no longer save thresholds
  prefs.putUInt("v_on", config_.minValveOnMs);
  prefs.putUInt("v_off", config_.minValveOffMs);

  prefs.putBool("slf_lgt", config_.selfLightEnable);
  prefs.putBool("slf_vlv", config_.selfValveEnable);

  prefs.end();
}

}  // namespace app
