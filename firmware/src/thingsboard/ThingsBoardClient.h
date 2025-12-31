#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

namespace tb {

class ThingsBoardClient {
 public:
  using RpcHandler = void (*)(const char* method, JsonVariantConst params);
  using AttributesHandler = void (*)(JsonVariantConst root);

  explicit ThingsBoardClient(Client& networkClient);

  void begin(const char* host, uint16_t port, const char* accessToken);
  void loop();

  bool isConnected();

  bool ensureConnected(const char* deviceName);
  bool sendTelemetryJson(const char* json);

  // Request shared attributes once connected.
  bool requestSharedAttributes(uint32_t requestId, const char* keysCsv);

  void setRpcHandler(RpcHandler handler);
  void setAttributesHandler(AttributesHandler handler);

 private:
  PubSubClient mqtt_;

  static ThingsBoardClient* active_;
  static void mqttCallback_(char* topic, uint8_t* payload, unsigned int length);
  void onMqttMessage_(const char* topic, const uint8_t* payload, unsigned int length);

  const char* host_ = nullptr;
  uint16_t port_ = 1883;
  const char* accessToken_ = nullptr;

  RpcHandler rpcHandler_ = nullptr;
  AttributesHandler attributesHandler_ = nullptr;

  uint32_t lastConnectAttemptMs_ = 0;
  static constexpr uint32_t kReconnectIntervalMs_ = 5000;

  static constexpr const char* kTelemetryTopic_ = "v1/devices/me/telemetry";
  static constexpr const char* kRpcRequestPrefix_ = "v1/devices/me/rpc/request/";
  static constexpr const char* kRpcRequestTopic_ = "v1/devices/me/rpc/request/+";

  static constexpr const char* kAttrUpdateTopic_ = "v1/devices/me/attributes";
  static constexpr const char* kAttrResponsePrefix_ = "v1/devices/me/attributes/response/";
  static constexpr const char* kAttrResponseTopic_ = "v1/devices/me/attributes/response/+";

  bool connect_(const char* deviceName);
};

}  // namespace tb
