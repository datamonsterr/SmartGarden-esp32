#pragma once

#include <Arduino.h>

namespace app {

class Settings {
 public:
  // Manual OFF latch (local button). When latched, light stays off.
  void toggleManualOff();
  void setManualOff(bool manualOff);
  bool manualOff() const;

  // Remote override (for ThingsBoard schedule or dashboard button).
  void setRemoteLightOverride(bool enabled, bool lightOn);
  bool remoteOverrideEnabled() const;
  bool remoteLightOn() const;

  // Temperature-based light request ("too cold => turn on")
  void setTempLimitEnabled(bool enabled);
  bool tempLimitEnabled() const;

  void setTempTooColdC(float tempC);
  float tempTooColdC() const;

  // Self light enable from ThingsBoard (global enable/disable for light relay)
  void setSelfLightEnable(bool enable);
  bool selfLightEnable() const;

  // Self valve enable from ThingsBoard (global enable/disable for watering valve)
  void setSelfValveEnable(bool enable);
  bool selfValveEnable() const;

 private:
  bool manualOff_ = false;

  bool remoteOverrideEnabled_ = false;
  bool remoteLightOn_ = false;

  bool tempLimitEnabled_ = false;
  float tempTooColdC_ = 18.0f;

  bool selfLightEnable_ = true;  // Default: enabled
  bool selfValveEnable_ = false; // Default: disabled (Server controls)
};

}  // namespace app
