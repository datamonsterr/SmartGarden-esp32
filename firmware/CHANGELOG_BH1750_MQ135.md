# Changelog: Thêm BH1750 & MQ135, Loại bỏ Soil Moisture Sensor

## 📅 Ngày: 28/12/2025

---

## 🎯 Mục tiêu

- ✅ Thêm cảm biến cường độ ánh sáng **BH1750** (I2C)
- ✅ Giữ nguyên cảm biến chất lượng không khí **MQ135** (analog)
- ❌ Loại bỏ cảm biến độ ẩm đất **Soil Moisture Sensor**
- 🔄 Chuyển hệ thống tưới từ **dựa trên độ ẩm đất** sang **hẹn giờ tự động (timer-based)**
- 📡 Đẩy thêm dữ liệu `light_lux` và `air_quality_raw` lên ThingsBoard telemetry

---

## 📝 Chi tiết thay đổi

### 1️⃣ **platformio.ini** - Thêm thư viện BH1750

**File:** `platformio.ini`

**Thay đổi:**
```diff
lib_deps =
  knolleary/PubSubClient
  bblanchon/ArduinoJson
  adafruit/DHT sensor library
  adafruit/Adafruit Unified Sensor
  adafruit/RTClib
+ claws/BH1750@^1.3.0
```

**Lý do:** Cần thư viện để giao tiếp với BH1750 qua I2C.

---

### 2️⃣ **Tạo sensor class cho BH1750**

#### File mới: `src/sensors/Bh1750Sensor.h`

```cpp
#pragma once

#include <Arduino.h>
#include <BH1750.h>

namespace sensors {

class Bh1750Sensor {
 public:
  Bh1750Sensor();
  void begin();
  float readLux();
  bool isOk() const { return initialized_; }

 private:
  BH1750 lightMeter_;
  bool initialized_ = false;
};

} // namespace sensors
```

#### File mới: `src/sensors/Bh1750Sensor.cpp`

```cpp
#include "sensors/Bh1750Sensor.h"

namespace sensors {

Bh1750Sensor::Bh1750Sensor() {}

void Bh1750Sensor::begin() {
  if (lightMeter_.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    initialized_ = true;
    Serial.println("BH1750 initialized");
  } else {
    Serial.println("Error initializing BH1750");
  }
}

float Bh1750Sensor::readLux() {
  if (!initialized_) {
    return -1.0f;
  }
  return lightMeter_.readLightLevel();
}

} // namespace sensors
```

**Lý do:** Đóng gói logic đọc cảm biến BH1750 theo pattern hiện tại của project.

---

### 3️⃣ **include/Config.h** - Cập nhật cấu hình

**Thay đổi:**

#### A. Loại bỏ pin Soil Moisture:
```diff
// ESP32 ADC pins (input-only is ok for sensors)
constexpr uint8_t kPinMq135Analog = 34;
- constexpr uint8_t kPinSoilMoistureAnalog = 35;
+ // Soil moisture sensor removed - using timer-based watering instead
```

#### B. Thay đổi cấu hình tưới nước (timer-based):
```diff
// ---- Auto watering ----
- // Soil moisture analog readings vary by sensor and soil. Calibrate and adjust.
- // Convention in this project: higher value = wetter.
- constexpr int kSoilWetThreshold = 2400;
- constexpr int kSoilDryThreshold = 1800;
- constexpr uint32_t kMinValveOnMs = 2000;
- constexpr uint32_t kMinValveOffMs = 5000;
- constexpr uint32_t kWateringIntervalMs = 10000;
- constexpr uint32_t kWateringDurationMs = 5000;

+ // Timer-based watering (no soil sensor)
+ constexpr uint32_t kMinValveOnMs = 30000;      // 30 seconds
+ constexpr uint32_t kMinValveOffMs = 3600000;   // 1 hour
+ constexpr uint32_t kWateringIntervalMs = 3600000; // 1 hour
+ constexpr uint32_t kWateringDurationMs = 30000;   // 30 seconds
```

**Lý do:**
- GPIO 35 không còn được dùng
- Chuyển từ logic "tưới khi đất khô" sang "tưới định kỳ mỗi 1 giờ trong 30 giây"

---

### 4️⃣ **src/app/Telemetry.h & .cpp** - Thêm BH1750 vào telemetry

**File:** `src/app/Telemetry.h`

```diff
void updateSensors(
    const sensors::DhtReading& dht,
    bool motionDetected,
    int mq135Raw,
-   int soilRaw);
+   float lightLux);

private:
  sensors::DhtReading dht_;
  bool motionDetected_ = false;
  int mq135Raw_ = -1;
- int soilRaw_ = -1;
+ float lightLux_ = -1.0f;
```

**File:** `src/app/Telemetry.cpp`

```diff
void Telemetry::updateSensors(
    const sensors::DhtReading& dht,
    bool motionDetected,
    int mq135Raw,
-   int soilRaw) {
+   float lightLux) {
  dht_ = dht;
  motionDetected_ = motionDetected;
  mq135Raw_ = mq135Raw;
- soilRaw_ = soilRaw;
+ lightLux_ = lightLux;
}
```

**Cập nhật JSON telemetry:**

```diff
String Telemetry::buildTelemetryJson(...) const {
  JsonDocument doc;

+ // DHT22 sensor data
  if (dht_.ok) {
    doc["temperature_c"] = dht_.temperatureC;
    doc["humidity_pct"] = dht_.humidityPct;
  } else {
    doc["temperature_c"] = nullptr;
    doc["humidity_pct"] = nullptr;
  }

+ // Motion sensor
+ doc["motion"] = motionDetected_;

+ // Air quality (MQ135)
+ doc["air_quality_raw"] = mq135Raw_;

+ // Light intensity (BH1750)
+ if (lightLux_ >= 0) {
+   doc["light_lux"] = lightLux_;
+ } else {
+   doc["light_lux"] = nullptr;
+ }

+ // Light controller state
+ doc["light_on"] = light.lightOn;
+ doc["manual_off"] = light.manualOff;

+ // Watering controller state
+ doc["valve_on"] = watering.valveOn;

  String out;
  serializeJson(doc, out);
  return out;
}
```

**Lý do:** 
- Thay `soilRaw` bằng `lightLux`
- Thêm các trường telemetry: `motion`, `air_quality_raw`, `light_lux`, `light_on`, `manual_off`, `valve_on`

---

### 5️⃣ **src/controllers/WateringController** - Đơn giản hóa thành timer

**File:** `src/controllers/WateringController.h`

**Trước:**
```cpp
class WateringController {
 public:
  WateringController(
      actuators::RelayActuator& valveRelay,
      sensors::AnalogSensor& soilSensor,  // ❌ Cần soil sensor
      int dryThreshold,
      int wetThreshold,
      uint32_t minOnMs,
      uint32_t minOffMs);
  
  void setThresholds(int dryThreshold, int wetThreshold);
  void setMinDurations(uint32_t minOnMs, uint32_t minOffMs);
  void setInterval(uint32_t intervalMs, uint32_t durationMs);
  
 private:
  sensors::AnalogSensor& soilSensor_;
  int dryThreshold_;
  int wetThreshold_;
  // ... logic phức tạp với hysteresis
};
```

**Sau:**
```cpp
struct WateringState {
  bool valveOn = false;
  uint32_t lastWateringMs = 0;
  uint32_t nextWateringMs = 0;
};

class WateringController {
 public:
  WateringController(
      actuators::RelayActuator& valveRelay,
      uint32_t intervalMs,    // ✅ Chỉ cần interval
      uint32_t durationMs);   // ✅ Và duration

  void update(uint32_t nowMs);
  WateringState state() const;
  void setInterval(uint32_t intervalMs, uint32_t durationMs);

 private:
  actuators::RelayActuator& valveRelay_;
  uint32_t intervalMs_;
  uint32_t durationMs_;
  uint32_t lastWateringStartMs_ = 0;
  bool isWatering_ = false;
  WateringState state_;
};
```

**File:** `src/controllers/WateringController.cpp`

**Logic mới (đơn giản):**
```cpp
void WateringController::update(uint32_t nowMs) {
  // Timer-based watering logic
  if (intervalMs_ > 0 && durationMs_ > 0) {
    if (!isWatering_) {
      // Check if it's time to start watering
      if (lastWateringStartMs_ == 0 || 
          (nowMs - lastWateringStartMs_) >= intervalMs_) {
        isWatering_ = true;
        lastWateringStartMs_ = nowMs;
        valveRelay_.setOn(true);
        Serial.println("Watering: Timer START");
      }
    } else {
      // Check if watering duration has elapsed
      if ((nowMs - lastWateringStartMs_) >= durationMs_) {
        isWatering_ = false;
        valveRelay_.setOn(false);
        Serial.println("Watering: Timer STOP");
      }
    }
  }
  
  // Update state
  state_.valveOn = valveRelay_.isOn();
  state_.lastWateringMs = lastWateringStartMs_;
  state_.nextWateringMs = lastWateringStartMs_ + intervalMs_;
}
```

**Lý do:**
- Loại bỏ phụ thuộc vào soil sensor
- Đơn giản hóa logic: mỗi `intervalMs` (1 giờ), tưới trong `durationMs` (30 giây)
- Giảm code từ ~120 dòng xuống ~50 dòng

---

### 6️⃣ **src/main.cpp** - Tích hợp BH1750, loại bỏ soil sensor

#### A. Thêm include:
```diff
#include "sensors/AnalogSensor.h"
#include "sensors/DhtSensor.h"
#include "sensors/PirSensor.h"
+ #include "sensors/Bh1750Sensor.h"
```

#### B. Khai báo sensors:
```diff
sensors::DhtSensor dht(config::kPinDht);
sensors::PirSensor pir(config::kPinPir);
+ sensors::Bh1750Sensor bh1750;

- // MQ-135 and soil moisture are treated as raw analog values.
+ // MQ-135 is treated as raw analog value
sensors::AnalogSensor mq135(config::kPinMq135Analog);
- sensors::AnalogSensor soil(config::kPinSoilMoistureAnalog);
```

#### C. Khởi tạo WateringController (constructor khác):
```diff
controllers::WateringController wateringController(
    valveRelay,
-   soil,
-   config::kSoilDryThreshold,
-   config::kSoilWetThreshold,
-   config::kMinValveOnMs,
-   config::kMinValveOffMs);
+   config::kWateringIntervalMs,
+   config::kWateringDurationMs);
```

#### D. Setup sensors:
```diff
dht.begin();
pir.begin();
mq135.begin();
- soil.begin();
+ bh1750.begin();
```

#### E. Loop - đọc sensors và update telemetry:
```diff
// Periodic sensor read
if (nowMs - lastSensorReadMs >= runtimeConfig.sensorReadIntervalMs) {
  lastSensorReadMs = nowMs;

  lastDhtReading = dht.read();
  // ... DHT logging ...
  
  lastMotionDetected = pir.readMotion();
+ Serial.print("PIR motion: ");
+ Serial.println(lastMotionDetected ? "DETECTED" : "none");
  
  const int mq135Raw = mq135.readRaw();
+ Serial.print("MQ135 raw: ");
+ Serial.println(mq135Raw);
  
- const int soilRaw = soil.readRaw();
+ const float lightLux = bh1750.readLux();
+ if (bh1750.isOk()) {
+   Serial.print("BH1750 light: ");
+   Serial.print(lightLux);
+   Serial.println(" lux");
+ } else {
+   Serial.println("BH1750 not initialized");
+ }

- telemetry.updateSensors(lastDhtReading, lastMotionDetected, mq135Raw, soilRaw);
+ telemetry.updateSensors(lastDhtReading, lastMotionDetected, mq135Raw, lightLux);

  wateringController.update(nowMs);
}
```

**Lý do:**
- Đọc BH1750 thay vì soil sensor
- Thêm debug logging cho tất cả sensors
- Truyền `lightLux` thay vì `soilRaw` vào telemetry

---

### 7️⃣ **src/app/RuntimeConfig.h** - Loại bỏ soil config

```diff
// Runtime-configurable parameters
struct RuntimeConfig {
  uint32_t telemetryIntervalMs = 10000;
  uint32_t sensorReadIntervalMs = 2000;
  uint32_t lightOnAfterMotionMs = 60000;

  // Temperature-light feature
  bool tempLightEnabled = false;
  float tempTooColdC = 18.0f;

- // Watering
- int soilWetThreshold = 2400;
- int soilDryThreshold = 1800;
- uint32_t minValveOnMs = 10000;
- uint32_t minValveOffMs = 30000;

+ // Watering (timer-based, no soil sensor)
+ uint32_t minValveOnMs = 30000;
+ uint32_t minValveOffMs = 3600000;
};
```

**Lý do:** Không còn cần threshold và duration cho soil-based watering.

---

### 8️⃣ **src/app/RemoteConfigManager.cpp** - Loại bỏ soil keys

```diff
const char* RemoteConfigManager::sharedKeysCsv() {
- return "telemetryIntervalMs,sensorReadIntervalMs,lightOnAfterMotionMs,tempLightEnabled,tempTooColdC,soilWetThreshold,soilDryThreshold,minValveOnMs,minValveOffMs";
+ return "telemetryIntervalMs,sensorReadIntervalMs,lightOnAfterMotionMs,tempLightEnabled,tempTooColdC,minValveOnMs,minValveOffMs";
}
```

**Lý do:** Không còn fetch `soilWetThreshold` và `soilDryThreshold` từ ThingsBoard Shared Attributes.

---

## 📊 Tóm tắt thay đổi

| File | Thay đổi | Lý do |
|------|---------|-------|
| **platformio.ini** | + Thêm `BH1750@^1.3.0` | Cần thư viện I2C cho BH1750 |
| **src/sensors/Bh1750Sensor.{h,cpp}** | + Tạo mới | Đóng gói logic đọc BH1750 |
| **include/Config.h** | - Bỏ `kPinSoilMoistureAnalog`<br>- Bỏ soil thresholds<br>+ Timer watering config | Không dùng soil sensor nữa |
| **src/app/Telemetry.{h,cpp}** | - Bỏ `soilRaw`<br>+ Thêm `lightLux`<br>+ Thêm fields telemetry | Đẩy BH1750 lên ThingsBoard |
| **src/controllers/WateringController.{h,cpp}** | - Bỏ soil sensor dependency<br>- Đơn giản hóa logic<br>+ Timer-based watering | Tưới định kỳ thay vì theo độ ẩm |
| **src/main.cpp** | + Thêm BH1750<br>- Bỏ soil sensor<br>+ Debug logging | Tích hợp sensor mới |
| **src/app/RuntimeConfig.h** | - Bỏ soil config fields | Không cần nữa |
| **src/app/RemoteConfigManager.cpp** | - Bỏ soil keys khỏi CSV | Không fetch từ ThingsBoard |

---

## 🔌 Sơ đồ đấu dây mới

### **ESP32 Pinout:**

| Thiết bị | Chân ESP32 | Ghi chú |
|----------|------------|---------|
| DHT22 DATA | GPIO 4 | Nhiệt độ/độ ẩm |
| PIR OUT | GPIO 27 | Chuyển động |
| MQ135 AOUT | GPIO 34 | Chất lượng không khí (analog) |
| **BH1750 SDA** | **GPIO 21** | **Cường độ ánh sáng (I2C)** |
| **BH1750 SCL** | **GPIO 22** | **Dùng chung với RTC** |
| RTC DS1307 SDA | GPIO 21 | I2C shared |
| RTC DS1307 SCL | GPIO 22 | I2C shared |
| Relay Light IN | GPIO 26 | Điều khiển đèn |
| Relay Valve IN | GPIO 25 | Điều khiển van tưới |
| Button | GPIO 14 | Nút thủ công |

### **Lưu ý I2C:**
- BH1750 và RTC DS1307 **dùng chung I2C bus** (GPIO 21/22)
- I2C addresses:
  - DS1307: `0x68`
  - BH1750: `0x23` (ADDR→GND) hoặc `0x5C` (ADDR→VCC)
- Cần điện trở pull-up 4.7kΩ trên SDA/SCL nếu module không có

---

## 📡 ThingsBoard Telemetry mới

### **Dữ liệu gửi lên (mỗi 10 giây):**

```json
{
  "temperature_c": 26.5,
  "humidity_pct": 65.0,
  "motion": false,
  "air_quality_raw": 1234,
  "light_lux": 450.5,
  "light_on": true,
  "manual_off": false,
  "valve_on": false
}
```

### **Shared Attributes (remote config):**

Bỏ:
- `soilWetThreshold`
- `soilDryThreshold`

Giữ nguyên:
- `telemetryIntervalMs`
- `sensorReadIntervalMs`
- `lightOnAfterMotionMs`
- `tempLightEnabled`
- `tempTooColdC`
- `minValveOnMs`
- `minValveOffMs`

---

## 🧪 Testing

### **Serial Monitor output mong đợi:**

```
Smart Garden ESP32 starting...
BH1750 initialized
RTC is running

DHT ok: T=26.5C H=65%
PIR motion: none
MQ135 raw: 1234
BH1750 light: 450.5 lux

Telemetry: {"temperature_c":26.5,"humidity_pct":65,...}

Watering: Timer START
... (sau 30 giây)
Watering: Timer STOP
```

### **Các case cần test:**

1. ✅ BH1750 khởi tạo thành công (`BH1750 initialized`)
2. ✅ Đọc được giá trị lux hợp lý (0-65535 lux)
3. ✅ MQ135 đọc được ADC (0-4095)
4. ✅ Tưới tự động mỗi 1 giờ trong 30 giây
5. ✅ Telemetry gửi đầy đủ 8 trường dữ liệu
6. ✅ Button manual light vẫn hoạt động
7. ✅ PIR motion vẫn trigger light

---

## ⚠️ Lưu ý quan trọng

### **1. Diagram.json chưa được cập nhật**
- File `diagram.json` vẫn có soil sensor
- **Nếu chạy Wokwi simulation → Cần cập nhật diagram**
- **Nếu chỉ deploy lên mạch thật → Không cần**

### **2. Soil sensor hoàn toàn bị loại bỏ**
- Không thể quay lại logic tưới theo độ ẩm đất mà không revert code
- Nếu muốn giữ cả 2 mode (soil + timer) → Cần refactor lại

### **3. Calibration cần thiết**
- **MQ135:** Cần thời gian "preheat" 24-48 giờ khi mới lắp
- **BH1750:** Có thể cần hiệu chỉnh nếu module có lens/cover

### **4. I2C conflict detection**
- Nếu BH1750 không init, check:
  ```bash
  # Scan I2C bus (nếu có i2c-tools)
  i2cdetect -y 1
  ```
  Phải thấy `0x68` (RTC) và `0x23` (BH1750)

---

## 🚀 Bước tiếp theo

1. **Build và upload:**
   ```bash
   platformio run --target upload
   ```

2. **Mở Serial Monitor:**
   ```bash
   platformio device monitor
   ```

3. **Kiểm tra ThingsBoard:**
   - Vào **Latest Telemetry** → Xem có `light_lux` và `air_quality_raw` không

4. **Test watering:**
   - Đợi 1 giờ hoặc sửa `kWateringIntervalMs = 60000` (1 phút) để test nhanh

---

## 📞 Support

Nếu có lỗi, hãy gửi:
1. **Serial Monitor output** đầy đủ
2. **Thông báo lỗi** cụ thể (nếu có)
3. **Mô tả hardware** đang dùng (module nào, đấu dây như nào)

---

**Tác giả:** GitHub Copilot  
**Ngày:** 28/12/2025  
**Version:** 2.0 (BH1750 + Timer Watering)
