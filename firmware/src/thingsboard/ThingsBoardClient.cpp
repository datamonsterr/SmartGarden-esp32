#include "thingsboard/ThingsBoardClient.h"

namespace tb {

ThingsBoardClient *ThingsBoardClient::active_ = nullptr;

ThingsBoardClient::ThingsBoardClient(Client &networkClient)
    : mqtt_(networkClient) {}

void ThingsBoardClient::begin(const char *host, uint16_t port,
                              const char *accessToken) {
  host_ = host;
  port_ = port;
  accessToken_ = accessToken;

  mqtt_.setServer(host_, port_);
  mqtt_.setBufferSize(512);
  mqtt_.setKeepAlive(60);
  mqtt_.setSocketTimeout(15);  // Increase socket timeout for Wokwi gateway

  active_ = this;
  mqtt_.setCallback(mqttCallback_);
}

void ThingsBoardClient::loop() { mqtt_.loop(); }

bool ThingsBoardClient::isConnected() { return mqtt_.connected(); }

void ThingsBoardClient::setRpcHandler(RpcHandler handler) {
  rpcHandler_ = handler;
}

void ThingsBoardClient::setAttributesHandler(AttributesHandler handler) {
  attributesHandler_ = handler;
}

bool ThingsBoardClient::requestSharedAttributes(uint32_t requestId,
                                                const char *keysCsv) {
  if (!mqtt_.connected()) {
    return false;
  }
  if (keysCsv == nullptr || keysCsv[0] == '\0') {
    return false;
  }

  char topic[96];
  snprintf(topic, sizeof(topic), "v1/devices/me/attributes/request/%lu",
           (unsigned long)requestId);

  JsonDocument doc;
  doc["sharedKeys"] = keysCsv;

  String payload;
  serializeJson(doc, payload);
  return mqtt_.publish(topic, payload.c_str());
}

void ThingsBoardClient::mqttCallback_(char *topic, uint8_t *payload,
                                      unsigned int length) {
  if (active_ == nullptr) {
    return;
  }
  active_->onMqttMessage_(topic, payload, length);
}

void ThingsBoardClient::onMqttMessage_(const char *topic,
                                       const uint8_t *payload,
                                       unsigned int length) {
  if (topic == nullptr || payload == nullptr || length == 0) {
    return;
  }

  const String topicStr(topic);

  if (topicStr.startsWith(kRpcRequestPrefix_)) {
    const String requestIdStr = topicStr.substring(strlen(kRpcRequestPrefix_));
    const int requestId = requestIdStr.toInt();

    JsonDocument doc;
    const auto err = deserializeJson(doc, payload, length);
    if (err) {
      Serial.print("RPC JSON parse failed: ");
      Serial.println(err.c_str());
      return;
    }

    const char *method = doc["method"] | "";
    const JsonVariantConst params = doc["params"];

    if (rpcHandler_ != nullptr && method != nullptr && method[0] != '\0') {
      rpcHandler_(method, params);
    }

    // Reply to the RPC request so ThingsBoard doesn't keep it pending.
    if (requestId > 0) {
      char responseTopic[96];
      snprintf(responseTopic, sizeof(responseTopic),
               "v1/devices/me/rpc/response/%d", requestId);
      mqtt_.publish(responseTopic, "{\"ok\":true}");
    }
    return;
  }

  if (topicStr == kAttrUpdateTopic_ ||
      topicStr.startsWith(kAttrResponsePrefix_)) {
    if (attributesHandler_ == nullptr) {
      return;
    }

    JsonDocument doc;
    const auto err = deserializeJson(doc, payload, length);
    if (err) {
      Serial.print("Attributes JSON parse failed: ");
      Serial.println(err.c_str());
      return;
    }
    attributesHandler_(doc.as<JsonVariantConst>());
    return;
  }
}

bool ThingsBoardClient::ensureConnected(const char *deviceName) {
  if (mqtt_.connected()) {
    return true;
  }

  const uint32_t nowMs = millis();
  if (nowMs - lastConnectAttemptMs_ < kReconnectIntervalMs_) {
    return false;
  }
  lastConnectAttemptMs_ = nowMs;

  return connect_(deviceName);
}

bool ThingsBoardClient::sendTelemetryJson(const char *json) {
  if (!mqtt_.connected()) {
    return false;
  }
  if (json == nullptr || json[0] == '\0') {
    return false;
  }

  const bool ok = mqtt_.publish(kTelemetryTopic_, json);
  if (!ok) {
    Serial.println("MQTT publish failed (telemetry)");
  }
  return ok;
}

bool ThingsBoardClient::connect_(const char *deviceName) {
  if (host_ == nullptr || host_[0] == '\0') {
    Serial.println("ThingsBoard host is empty. Check include/Secrets.h");
    return false;
  }
  if (accessToken_ == nullptr || accessToken_[0] == '\0') {
    Serial.println(
        "ThingsBoard access token is empty. Check include/Secrets.h");
    return false;
  }

  char clientId[96];
  snprintf(clientId, sizeof(clientId), "%s-%06X", deviceName,
           (uint32_t)ESP.getEfuseMac());

  Serial.print("Connecting to ThingsBoard MQTT ");
  Serial.print(host_);
  Serial.print(':');
  Serial.print(port_);
  Serial.print(" as clientId=");
  Serial.print(clientId);
  Serial.print(" token=");
  Serial.println(accessToken_);

  // ThingsBoard access token is used as MQTT username; password is empty.
  const bool ok = mqtt_.connect(clientId, accessToken_, nullptr);
  if (ok) {
    Serial.println("ThingsBoard MQTT connected!");

    // Subscribe to RPC requests (used for schedule & dashboard control).
    if (!mqtt_.subscribe(kRpcRequestTopic_)) {
      Serial.println("MQTT subscribe failed (RPC)");
    }

    // Subscribe to shared attributes updates and responses.
    if (!mqtt_.subscribe(kAttrUpdateTopic_)) {
      Serial.println("MQTT subscribe failed (attributes update)");
    }
    if (!mqtt_.subscribe(kAttrResponseTopic_)) {
      Serial.println("MQTT subscribe failed (attributes response)");
    }
  } else {
    const int state = mqtt_.state();
    Serial.print("ThingsBoard MQTT connect FAILED. state=");
    Serial.print(state);
    Serial.print(" (");
    // Decode PubSubClient state codes
    switch (state) {
      case -4: Serial.print("MQTT_CONNECTION_TIMEOUT"); break;
      case -3: Serial.print("MQTT_CONNECTION_LOST"); break;
      case -2: Serial.print("MQTT_CONNECT_FAILED"); break;
      case -1: Serial.print("MQTT_DISCONNECTED"); break;
      case 0: Serial.print("MQTT_CONNECTED"); break;
      case 1: Serial.print("MQTT_CONNECT_BAD_PROTOCOL"); break;
      case 2: Serial.print("MQTT_CONNECT_BAD_CLIENT_ID"); break;
      case 3: Serial.print("MQTT_CONNECT_UNAVAILABLE"); break;
      case 4: Serial.print("MQTT_CONNECT_BAD_CREDENTIALS"); break;
      case 5: Serial.print("MQTT_CONNECT_UNAUTHORIZED"); break;
      default: Serial.print("UNKNOWN"); break;
    }
    Serial.println(")");
  }
  return ok;
}

} // namespace tb
