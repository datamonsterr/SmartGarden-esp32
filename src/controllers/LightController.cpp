#include "controllers/LightController.h"

namespace controllers {

LightController::LightController(actuators::RelayActuator &relay,
                                 float tempHysteresisC)
    : relay_(relay), tempHysteresisC_(tempHysteresisC) {
}

void LightController::update(
    uint32_t nowMs,
    bool motionDetected,
    const sensors::DhtReading& dht,
    const app::Settings& settings) {
  // Store for telemetry only - not used for control
  state_.motionDetected = motionDetected;
  state_.manualOff = settings.manualOff();
  state_.remoteOverrideEnabled = settings.remoteOverrideEnabled();
  state_.tempLimitEnabled = settings.tempLimitEnabled();
  state_.tempTooColdC = settings.tempTooColdC();

  // ========== DUMB DEVICE MODE với SAFETY CHECK ==========
  // ESP32 KHÔNG tự tính toán logic bật/tắt đèn.
  // Server (ThingsBoard) quyết định và gửi xuống qua Shared Attribute: self_light_enable
  // 
  // ⚠️ SAFETY: Nếu Server gửi OFF nhưng nhiệt độ vẫn < 23°C, ESP32 sẽ GIỮ ĐÈN BẬT
  // (Tránh tình huống Rule Chain conflict gửi ON rồi OFF liên tiếp)
  // ========================================================

  bool desiredOn = false;

  if (settings.manualOff()) {
    // Local manual button has priority to force OFF
    desiredOn = false;
  } else {
    // Listen to Server's command via self_light_enable
    desiredOn = settings.selfLightEnable();
    
    // SAFETY CHECK: Nếu Server muốn TẮT nhưng nhiệt độ vẫn QUÁ LẠNH
    if (!desiredOn && dht.ok && dht.temperatureC < 23.0f) {
      Serial.println("⚠️  SAFETY: Ignoring OFF command - Temperature still too cold!");
      Serial.print("   Temperature: ");
      Serial.print(dht.temperatureC);
      Serial.println("°C < 23°C → Keeping light ON");
      desiredOn = true;  // Override: GIỮ ĐÈN BẬT
    }
  }

  relay_.setOn(desiredOn);
  state_.lightOn = relay_.isOn();
}

LightState LightController::state() const {
  return state_;
}

} // namespace controllers
