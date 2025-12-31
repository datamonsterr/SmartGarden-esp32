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

#include "sensors/DhtSensor.h"
#include "sensors/PirSensor.h"
#include "sensors/AnalogSensor.h"

#include "actuators/RelayActuator.h"
#include "controllers/LightController.h"
#include "controllers/WateringController.h"

#include "app/Settings.h"
#include "app/RuntimeConfig.h"
#include "app/RemoteConfigManager.h"

#include "app/Telemetry.h"

namespace {

net::WiFiManager wifiManager;

WiFiClient wifiClient;
tb::ThingsBoardClient tbClient(wifiClient);

sensors::DhtSensor dht(config::kPinDht);
sensors::PirSensor pir(config::kPinPir);

// MQ-135 and soil moisture are treated as raw analog values.
sensors::AnalogSensor mq135(config::kPinMq135Analog);
sensors::AnalogSensor soil(config::kPinSoilMoistureAnalog);

actuators::RelayActuator lightRelay(config::kPinRelayLight, config::kRelayActiveLow);
actuators::RelayActuator valveRelay(config::kPinRelayValve, config::kRelayActiveLow);

controllers::LightController lightController(
  lightRelay,
  config::kTempLightHysteresisC);
controllers::WateringController wateringController(
    valveRelay,
    soil,
    config::kSoilDryThreshold,
    config::kSoilWetThreshold,
    config::kMinValveOnMs,
    config::kMinValveOffMs);

app::Telemetry telemetry;

app::RuntimeConfig runtimeConfig;

app::Settings settings;

app::RemoteConfigManager remoteConfig(runtimeConfig, settings, wateringController);

uint32_t lastSensorReadMs = 0;
uint32_t lastTelemetryMs = 0;

uint32_t lastAttrRequestMs = 0;
uint32_t attrRequestId = 1;
bool attrRequestedThisConnection = false;

sensors::DhtReading lastDhtReading;
bool lastMotionDetected = false;

void onTbRpc(const char* method, JsonVariantConst params) {
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

  if (strcmp(method, "setTempLimit") == 0) {
    const float limitC = params.as<float>();
    settings.setTempTooColdC(limitC);
    settings.setTempLimitEnabled(true);
    Serial.print("RPC setTempLimit: ");
    Serial.println(limitC);
    return;
  }

  if (strcmp(method, "setTempLimitEnabled") == 0) {
    const bool enabled = params.as<bool>();
    settings.setTempLimitEnabled(enabled);
    Serial.print("RPC setTempLimitEnabled: ");
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

  Serial.print("RPC unknown method: ");
  Serial.println(method);
}

void onTbAttributes(JsonVariantConst root) {
  if (remoteConfig.applyAttributes(root)) {
    Serial.println("Applied remote config (attributes)");
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);  // Wait longer for serial to initialize in Wokwi

  Serial.println();
  Serial.println("========================================");
  Serial.println("Smart Garden ESP32 starting...");
  Serial.println("========================================");

  lightRelay.begin();
  valveRelay.begin();

  dht.begin();
  delay(2000);  // Give DHT sensor time to stabilize (required for Wokwi simulation)
  
  pir.begin();
  mq135.begin();
  soil.begin();

  settings.setTempLimitEnabled(config::kTempLightEnabledByDefault);
  settings.setTempTooColdC(config::kTempTooColdCDefault);

  // Initialize runtime defaults from Config.h (fallback).
  runtimeConfig.telemetryIntervalMs = config::kTelemetryIntervalMs;
  runtimeConfig.sensorReadIntervalMs = config::kSensorReadIntervalMs;
  runtimeConfig.tempLightEnabled = config::kTempLightEnabledByDefault;
  runtimeConfig.tempTooColdC = config::kTempTooColdCDefault;
  runtimeConfig.soilWetThreshold = config::kSoilWetThreshold;
  runtimeConfig.soilDryThreshold = config::kSoilDryThreshold;
  runtimeConfig.minValveOnMs = config::kMinValveOnMs;
  runtimeConfig.minValveOffMs = config::kMinValveOffMs;

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

  Serial.println("Setup complete. Entering loop...");
}

void loop() {
  wifiManager.ensureConnected();

  // Only proceed with MQTT if WiFi is connected
  if (!wifiManager.isConnected()) {
    delay(100);
    return;
  }

  const uint32_t nowMs = millis();

  // Keep MQTT alive (non-blocking).
  tbClient.loop();

  const bool mqttConnected = tbClient.ensureConnected(config::kDeviceName);
  if (!mqttConnected) {
    // Force attribute re-request after reconnect.
    attrRequestedThisConnection = false;
  } else {
    if (!attrRequestedThisConnection && (nowMs - lastAttrRequestMs) >= 30000) {
      lastAttrRequestMs = nowMs;
      if (tbClient.requestSharedAttributes(attrRequestId++, app::RemoteConfigManager::sharedKeysCsv())) {
        Serial.println("Requested shared attributes");
        attrRequestedThisConnection = true;
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
      Serial.println("%  ");
    } else {
      Serial.println("DHT read failed (NaN). Check wiring/pin/type or read interval >= 2000ms");
    }
    lastMotionDetected = pir.readMotion();
    const int mq135Raw = mq135.readRaw();
    const int soilRaw = soil.readRaw();

    telemetry.updateSensors(lastDhtReading, lastMotionDetected, mq135Raw, soilRaw);

    wateringController.update(nowMs);
  }

  // Update light frequently so manual button / remote override takes effect immediately.
  lightController.update(lastDhtReading, settings);

  // Periodic telemetry send.
  if (nowMs - lastTelemetryMs >= runtimeConfig.telemetryIntervalMs) {
    lastTelemetryMs = nowMs;

    if (mqttConnected) {
      const auto payload = telemetry.buildTelemetryJson(lightController.state(), wateringController.state());
      Serial.print("Telemetry: ");
      Serial.println(payload);
      const bool ok = tbClient.sendTelemetryJson(payload.c_str());
      if (!ok) {
        Serial.println("Telemetry publish failed");
      }
    }
  }
}
