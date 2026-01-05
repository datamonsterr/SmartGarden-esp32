#include <Arduino.h>
#include <string.h>

#include "Config.h"

// Developer convenience: project can compile before you create `include/Secrets.h`.
// For real deployments, always create `include/Secrets.h` with real credentials.
#if __has_include("Secrets.h")
#include "Secrets.h"
#else
#include "Secrets.h.example"
#endif

#include "net/WiFiManager.h"
#include "thingsboard/ThingsBoardClient.h"

#include "sensors/AnalogSensor.h"
#include "sensors/DhtSensor.h"
#include "sensors/PirSensor.h"
#include "sensors/Bh1750Sensor.h"

#include "actuators/RelayActuator.h"
#include "controllers/LightController.h"
#include "controllers/WateringController.h"

#include "app/RemoteConfigManager.h"
#include "app/RuntimeConfig.h"
#include "app/Settings.h"
#include "inputs/Button.h"

#include "app/Telemetry.h"

#include <RTClib.h>
#include <Wire.h>

namespace {

net::WiFiManager wifiManager;

WiFiClient wifiClient;
tb::ThingsBoardClient tbClient(wifiClient);

sensors::DhtSensor dht(config::kPinDht);
sensors::PirSensor pir(config::kPinPir);
sensors::Bh1750Sensor bh1750;

// MQ-135 is treated as raw analog value
sensors::AnalogSensor mq135(config::kPinMq135Analog);

actuators::RelayActuator lightRelay(config::kPinRelayLight, config::kRelayActiveLow);
actuators::RelayActuator valveRelay(config::kPinRelayValve, config::kRelayActiveLow);

controllers::LightController lightController(
  lightRelay,
  config::kTempLightHysteresisC);
controllers::WateringController wateringController(valveRelay);

app::Telemetry telemetry;

app::RuntimeConfig runtimeConfig;

app::Settings settings;
inputs::Button lightManualButton(config::kPinLightManualButton);

app::RemoteConfigManager remoteConfig(runtimeConfig, settings, lightController,
                                      wateringController);

RTC_DS1307 rtc;

uint32_t lastSensorReadMs = 0;
uint32_t lastTelemetryMs = 0;

uint32_t lastAttrRequestMs = 0;
uint32_t attrRequestId = 1;
bool attrRequestedThisConnection = false;

sensors::DhtReading lastDhtReading;
bool lastMotionDetected = false;

void onTbRpc(const char* method, JsonVariantConst params) {
  // ========== DUMB DEVICE MODE ==========
  // ESP32 chủ yếu nhận lệnh từ Shared Attributes (self_light_enable).
  // RPC commands dưới đây là backup cho manual control/testing.
  // =======================================

  if (strcmp(method, "setLight") == 0) {
    const bool on = params.as<bool>();
    settings.setRemoteLightOverride(true, on);
    Serial.print("RPC setLight: ");
    Serial.println(on ? "ON" : "OFF");
    return;
  }

  if (strcmp(method, "clearLightOverride") == 0) {
    settings.setRemoteLightOverride(false, false);
    Serial.println("RPC clearLightOverride");
    return;
  }

  // Legacy RPCs for temp limit (not used in dumb device mode, kept for compatibility)
  if (strcmp(method, "setTempLimit") == 0) {
    const float limitC = params.as<float>();
    settings.setTempTooColdC(limitC);
    settings.setTempLimitEnabled(true);
    Serial.print("RPC setTempLimit (legacy): ");
    Serial.println(limitC);
    return;
  }

  if (strcmp(method, "setTempLimitEnabled") == 0) {
    const bool enabled = params.as<bool>();
    settings.setTempLimitEnabled(enabled);
    Serial.print("RPC setTempLimitEnabled (legacy): ");
    Serial.println(enabled ? "true" : "false");
    return;
  }

  if (strcmp(method, "setManualOff") == 0) {
    const bool off = params.as<bool>();
    settings.setManualOff(off);
    Serial.print("RPC setManualOff: ");
    Serial.println(off ? "true" : "false");
    return;
  }

  if (strcmp(method, "toggleManualOff") == 0) {
    settings.toggleManualOff();
    Serial.print("RPC toggleManualOff: ");
    Serial.println(settings.manualOff() ? "ON" : "OFF");
    return;
  }

  // Note: Watering is now controlled by Server via self_valve_enable attribute
  // (removed setWateringInterval RPC)

  Serial.print("RPC unknown method: ");
  Serial.println(method);
}

void onTbAttributes(JsonVariantConst root) {
  // ========== THINGSBOARD SHARED ATTRIBUTES HANDLER ==========
  // Callback này được gọi khi:
  // 1. ESP32 request attributes lúc khởi động (requestSharedAttributes)
  // 2. Server thay đổi Shared Attribute (real-time update)
  //
  // QUAN TRỌNG: Khi Server thay đổi self_light_enable, callback này
  // được trigger tự động => ESP32 nhận lệnh real-time
  // ===========================================================
  
  // 🔍 DEBUG: In ra toàn bộ JSON nhận được từ Server
  Serial.print("📥 Received attributes from ThingsBoard at ");
  Serial.print(millis());
  Serial.println(" ms:");
  String jsonDebug;
  serializeJsonPretty(root, jsonDebug);
  Serial.println(jsonDebug);
  
  if (remoteConfig.applyAttributes(root)) {
    Serial.println("✅ Applied remote config from ThingsBoard attributes");
    Serial.print("   └─ self_light_enable = ");
    Serial.println(settings.selfLightEnable() ? "TRUE" : "FALSE");
    Serial.print("   └─ self_valve_enable = ");
    Serial.println(settings.selfValveEnable() ? "TRUE" : "FALSE");
    Serial.print("   └─ Current temperature = ");
    if (lastDhtReading.ok) {
      Serial.print(lastDhtReading.temperatureC);
      Serial.println("°C");
    } else {
      Serial.println("N/A");
    }
  } else {
    Serial.println("⚠️  No changes applied (attribute format issue or no change)");
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(50);

  Serial.println();
  Serial.println("Smart Garden ESP32 starting...");
  
  // I2C for RTC
  Wire.begin(config::kPinI2cSda, config::kPinI2cScl);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC DS1307 on I2C");
  } else {
    Serial.println("RTC DS1307 found");
    if (!rtc.isrunning()) {
      Serial.println("RTC is NOT running, setting time to compile time!");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    } else {
      DateTime now = rtc.now();
      Serial.print("✅ RTC is running: ");
      Serial.print(now.year(), DEC);
      Serial.print('/');
      Serial.print(now.month(), DEC);
      Serial.print('/');
      Serial.print(now.day(), DEC);
      Serial.print(" ");
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.println(now.second(), DEC);
    }
  }

  lightRelay.begin();
  valveRelay.begin();

  dht.begin();
  pir.begin();
  mq135.begin();
  bh1750.begin();

  lightManualButton.begin();

  settings.setTempLimitEnabled(config::kTempLightEnabledByDefault);
  settings.setTempTooColdC(config::kTempTooColdCDefault);

  // Initialize runtime defaults from Config.h (fallback).
  runtimeConfig.telemetryIntervalMs = config::kTelemetryIntervalMs;
  runtimeConfig.sensorReadIntervalMs = config::kSensorReadIntervalMs;
  runtimeConfig.tempLightEnabled = config::kTempLightEnabledByDefault;
  runtimeConfig.tempTooColdC = config::kTempTooColdCDefault;
  runtimeConfig.minValveOnMs = config::kMinValveOnMs;
  runtimeConfig.minValveOffMs = config::kMinValveOffMs;
  runtimeConfig.selfLightEnable = true;  // Default: enabled

  // WateringController is now controlled via Server (no timer logic)
  // wateringController.setInterval() removed - Server controls via self_valve_enable

  remoteConfig.begin();

  Serial.print("Telemetry interval ms: ");
  Serial.println(runtimeConfig.telemetryIntervalMs);
  Serial.print("Sensor read interval ms: ");
  Serial.println(runtimeConfig.sensorReadIntervalMs);

  wifiManager.begin(secrets::kWifiSsid, secrets::kWifiPassword);
  wifiManager.ensureConnected();

  tbClient.begin(secrets::kThingsBoardHost, secrets::kThingsBoardPort, secrets::kThingsBoardAccessToken);
  tbClient.setRpcHandler(onTbRpc);
  tbClient.setAttributesHandler(onTbAttributes);
}

void loop() {
  wifiManager.ensureConnected();

  const uint32_t nowMs = millis();

  if (lightManualButton.update(nowMs)) {
    settings.toggleManualOff();
    Serial.print("Manual light OFF latch: ");
    Serial.println(settings.manualOff() ? "ON" : "OFF");
  }

  // Keep MQTT alive (non-blocking).
  tbClient.loop();

  const bool mqttConnected = tbClient.ensureConnected(config::kDeviceName);
  if (!mqttConnected) {
    // Force attribute re-request after reconnect.
    attrRequestedThisConnection = false;
  } else {
    // ========== REQUEST ATTRIBUTES ON RECONNECT ==========
    // Khi mới connect/reconnect, ESP32 cần hỏi Server về trạng thái hiện tại:
    // "Đèn nên ON hay OFF lúc này?"
    // 
    // Lý do: ESP32 có thể mất điện/reset giữa chừng, cần đồng bộ lại
    // với trạng thái self_light_enable từ Server
    // ======================================================
    if (!attrRequestedThisConnection && (nowMs - lastAttrRequestMs) >= 30000) {
      lastAttrRequestMs = nowMs;
      Serial.print("📡 Requesting shared attributes: ");
      Serial.println(app::RemoteConfigManager::sharedKeysCsv());
      if (tbClient.requestSharedAttributes(attrRequestId++, app::RemoteConfigManager::sharedKeysCsv())) {
        Serial.println("   └─ Request sent successfully");
        attrRequestedThisConnection = true;
      } else {
        Serial.println("   └─ ❌ Request failed!");
      }
    }
  }

  // Periodic sensor read.
  if (nowMs - lastSensorReadMs >= runtimeConfig.sensorReadIntervalMs) {
    lastSensorReadMs = nowMs;

    lastDhtReading = dht.read();
    if (lastDhtReading.ok) {
      Serial.print("DHT ok: T=");
      Serial.print(lastDhtReading.temperatureC);
      Serial.print("C H=");
      Serial.print(lastDhtReading.humidityPct);
      Serial.println("%");
    } else {
      Serial.println("DHT read failed (NaN). Check wiring/pin/type or read interval >= 2000ms");
    }
    
    lastMotionDetected = pir.readMotion();
    Serial.print("PIR motion: ");
    Serial.println(lastMotionDetected ? "DETECTED" : "none");
    
    const int mq135Raw = mq135.readRaw();
    Serial.print("MQ135 raw: ");
    Serial.println(mq135Raw);
    
    const float lightLux = bh1750.readLux();
    if (bh1750.isOk()) {
      Serial.print("BH1750 light: ");
      Serial.print(lightLux);
      Serial.println(" lux");
    } else {
      Serial.println("BH1750 not initialized");
    }

    telemetry.updateSensors(lastDhtReading, lastMotionDetected, mq135Raw, lightLux);

    wateringController.update(nowMs);
  }

  // Update light frequently so manual button / remote override takes effect immediately.
  lightController.update(nowMs, lastMotionDetected, lastDhtReading, settings);
  
  // Log light state changes
  static bool prevLightOn = false;
  const bool currentLightOn = lightController.state().lightOn;
  if (currentLightOn != prevLightOn) {
    Serial.print("💡 Light state changed: ");
    Serial.println(currentLightOn ? "ON" : "OFF");
    prevLightOn = currentLightOn;
  }

  // Periodic telemetry send.
  if (nowMs - lastTelemetryMs >= runtimeConfig.telemetryIntervalMs) {
    lastTelemetryMs = nowMs;

    // Log RTC time
    if (rtc.begin() && rtc.isrunning()) {
      DateTime now = rtc.now();
      Serial.print("🕐 RTC Time: ");
      Serial.print(now.year(), DEC);
      Serial.print('/');
      if (now.month() < 10) Serial.print('0');
      Serial.print(now.month(), DEC);
      Serial.print('/');
      if (now.day() < 10) Serial.print('0');
      Serial.print(now.day(), DEC);
      Serial.print(" ");
      if (now.hour() < 10) Serial.print('0');
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      if (now.minute() < 10) Serial.print('0');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      if (now.second() < 10) Serial.print('0');
      Serial.println(now.second(), DEC);
    }

    if (mqttConnected) {
      const auto payload = telemetry.buildTelemetryJson(lightController.state(), wateringController.state(), settings.selfLightEnable(), settings.selfValveEnable());
      Serial.println("========================================");
      Serial.print("📤 Sending Telemetry to ThingsBoard");
      Serial.println(payload);
      Serial.println("========================================");
      const bool ok = tbClient.sendTelemetryJson(payload.c_str());
      if (!ok) {
        Serial.println("❌ Telemetry publish failed");
      } else {
        Serial.println("✅ Telemetry published successfully");
      }
    }
  }
}
