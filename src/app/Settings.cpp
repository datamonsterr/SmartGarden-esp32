#include "app/Settings.h"

namespace app {

void Settings::toggleManualOff() {
  manualOff_ = !manualOff_;
}

void Settings::setManualOff(bool manualOff) {
  manualOff_ = manualOff;
}

bool Settings::manualOff() const {
  return manualOff_;
}

void Settings::setRemoteLightOverride(bool enabled, bool lightOn) {
  remoteOverrideEnabled_ = enabled;
  remoteLightOn_ = lightOn;
}

bool Settings::remoteOverrideEnabled() const {
  return remoteOverrideEnabled_;
}

bool Settings::remoteLightOn() const {
  return remoteLightOn_;
}

void Settings::setTempLimitEnabled(bool enabled) {
  tempLimitEnabled_ = enabled;
}

bool Settings::tempLimitEnabled() const {
  return tempLimitEnabled_;
}

void Settings::setTempTooColdC(float tempC) {
  tempTooColdC_ = tempC;
}

float Settings::tempTooColdC() const {
  return tempTooColdC_;
}

void Settings::setSelfLightEnable(bool enable) {
  selfLightEnable_ = enable;
}

bool Settings::selfLightEnable() const {
  return selfLightEnable_;
}

}  // namespace app
