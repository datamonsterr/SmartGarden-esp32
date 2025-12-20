#include "net/WiFiManager.h"

namespace net {

void WiFiManager::begin(const char* ssid, const char* password) {
  ssid_ = ssid;
  password_ = password;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
}

bool WiFiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

IPAddress WiFiManager::localIp() const {
  return WiFi.localIP();
}

void WiFiManager::ensureConnected() {
  if (isConnected()) {
    return;
  }

  const uint32_t nowMs = millis();
  if (nowMs - lastAttemptMs_ < kRetryIntervalMs_) {
    return;
  }

  lastAttemptMs_ = nowMs;
  connect_();
}

void WiFiManager::connect_() {
  if (ssid_ == nullptr || ssid_[0] == '\0') {
    Serial.println("WiFi SSID is empty. Check include/Secrets.h");
    return;
  }

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid_);

  WiFi.begin(ssid_, password_);

  // Quick wait loop (bounded) for nicer UX at boot.
  const uint32_t startMs = millis();
  while (!isConnected() && millis() - startMs < 12000) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (isConnected()) {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi not connected yet; will retry...");
  }
}

}  // namespace net
