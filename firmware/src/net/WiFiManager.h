#pragma once

#include <Arduino.h>
#include <WiFi.h>

namespace net {

class WiFiManager {
 public:
  void begin(const char* ssid, const char* password);

  // Ensures we are connected; tries reconnect if not.
  // This is safe to call from loop() frequently.
  void ensureConnected();

  bool isConnected() const;
  IPAddress localIp() const;

 private:
  const char* ssid_ = nullptr;
  const char* password_ = nullptr;

  uint32_t lastAttemptMs_ = 0;
  static constexpr uint32_t kRetryIntervalMs_ = 5000;

  void connect_();
};

}  // namespace net
