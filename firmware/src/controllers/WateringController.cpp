#include "controllers/WateringController.h"

namespace controllers {

WateringController::WateringController(actuators::RelayActuator &valveRelay)
    : valveRelay_(valveRelay) {}

void WateringController::update(uint32_t nowMs) {
  // ========== DUMB DEVICE MODE ==========
  // WateringController KHÔNG tự động tưới theo lịch nữa.
  // Chỉ nhận lệnh từ Server qua self_valve_enable attribute.
  //
  // Logic đơn giản:
  //   - self_valve_enable = true  => BẬT van
  //   - self_valve_enable = false => TẮT van
  // =======================================

  valveRelay_.setOn(selfValveEnable_);
  state_.valveOn = valveRelay_.isOn();
}

WateringState WateringController::state() const { 
  return state_; 
}

void WateringController::setSelfValveEnable(bool enabled) {
  selfValveEnable_ = enabled;
}

} // namespace controllers
