# ESP32-CAM Migration Guide

## Tổng quan

Dự án đã được chuyển từ **ESP32-DevKit V1** sang **ESP32-CAM** với các thay đổi sau:

### Loại bỏ tính năng
- ❌ **MQ-135** (cảm biến khí gas) - Đã loại bỏ hoàn toàn
- ❌ **Button thủ công** (manual override button) - Đã loại bỏ
- ❌ **Camera module** - Không sử dụng trên ESP32-CAM
- ❌ **SD Card** - Không sử dụng trên ESP32-CAM

### Các tính năng còn lại
- ✅ **DHT22** - Cảm biến nhiệt độ và độ ẩm
- ✅ **PIR** - Cảm biến chuyển động
- ✅ **BH1750** - Cảm biến ánh sáng (I2C)
- ✅ **RTC DS1307** - Đồng hồ thời gian thực (I2C)
- ✅ **2 Relay** - Điều khiển đèn và van tưới
- ✅ **WiFi & ThingsBoard** - Kết nối IoT

## Sơ đồ chân mới (ESP32-CAM)

### Cảm biến
| Thiết bị | Chân ESP32-CAM | Chân cũ (DevKit) | Ghi chú |
|----------|----------------|------------------|---------|
| DHT22    | **GPIO 14**    | GPIO 4           | Data pin |
| PIR      | **GPIO 13**    | GPIO 27          | Motion detection |
| BH1750 (SDA) | **GPIO 12** | GPIO 21      | I2C Data |
| BH1750 (SCL) | **GPIO 4**  | GPIO 22      | I2C Clock |
| RTC DS1307 (SDA) | **GPIO 12** | GPIO 21  | I2C Data (shared) |
| RTC DS1307 (SCL) | **GPIO 4**  | GPIO 22  | I2C Clock (shared) |

### Actuators (Relay)
| Thiết bị | Chân ESP32-CAM | Chân cũ (DevKit) | Ghi chú |
|----------|----------------|------------------|---------|
| Relay 1 (Đèn) | **GPIO 15** | GPIO 26      | Light control |
| Relay 2 (Van)  | **GPIO 2**  | GPIO 25      | Valve control |

### Chân bị loại bỏ
| Thiết bị | Chân cũ | Lý do |
|----------|---------|-------|
| MQ-135   | GPIO 34 | Không sử dụng nữa |
| Button   | GPIO 14 | Không sử dụng nữa |

## Lưu ý quan trọng về ESP32-CAM

### 1. I2C với chân tùy chỉnh
ESP32-CAM **KHÔNG sử dụng** chân I2C mặc định (GPIO 21/22) vì các chân này không khả dụng. 

**Code khởi tạo I2C:**
```cpp
Wire.begin(12, 4);  // SDA=GPIO12, SCL=GPIO4
```

### 2. Chân cần tránh trên ESP32-CAM
- **GPIO 0**: Flash button (dùng để vào chế độ bootloader)
- **GPIO 1, 3**: UART TX/RX (dùng cho Serial)
- **GPIO 6-11**: Kết nối với SPI Flash (KHÔNG được dùng)
- **GPIO 16**: Kết nối với PSRAM (nếu có)

### 3. Giải phóng tài nguyên
Do ESP32-CAM có RAM hạn chế hơn DevKit, dự án đã:
- Loại bỏ thư viện `AnalogSensor` (cho MQ-135)
- Loại bỏ thư viện `Button` (manual control)
- Không sử dụng Camera libraries (esp_camera.h, etc.)
- Không sử dụng SD Card libraries

## Thay đổi code chính

### 1. `include/Config.h`
```cpp
// ESP32-CAM Pin Configuration
constexpr uint8_t kPinDht = 14;          // DHT22
constexpr uint8_t kPinPir = 13;          // PIR
constexpr uint8_t kPinRelayLight = 15;   // Relay 1
constexpr uint8_t kPinRelayValve = 2;    // Relay 2
constexpr uint8_t kPinI2cSda = 12;       // I2C SDA
constexpr uint8_t kPinI2cScl = 4;        // I2C SCL
```

### 2. `src/main.cpp`
- ❌ Loại bỏ `#include "sensors/AnalogSensor.h"`
- ❌ Loại bỏ `#include "inputs/Button.h"`
- ❌ Loại bỏ biến `mq135` và `lightManualButton`
- ✅ Cập nhật `Wire.begin(12, 4)` với chân I2C tùy chỉnh
- ✅ Truyền `0` thay vì `mq135Raw` vào `telemetry.updateSensors()`

### 3. `platformio.ini`
```ini
[platformio]
default_envs = esp32cam

[env:esp32cam]
platform = espressif32
board = esp32cam
framework = arduino
```

## Kiểm tra sau khi migration

### 1. Build project
```bash
pio run
```

### 2. Upload lên ESP32-CAM
```bash
pio run --target upload
```

**Lưu ý:** Để vào chế độ flash:
1. Kết nối GPIO 0 với GND
2. Nhấn nút RESET
3. Bắt đầu upload
4. Sau khi upload xong, ngắt GPIO 0 và nhấn RESET lại

### 3. Monitor Serial
```bash
pio device monitor
```

Kiểm tra các log:
- ✅ "Smart Garden ESP32-CAM starting..."
- ✅ "Note: Camera and SD Card modules are NOT used"
- ✅ "RTC DS1307 found"
- ✅ "BH1750 light: ... lux"
- ✅ "DHT ok: T=...C H=...%"
- ✅ "PIR motion: ..."

## Kết nối phần cứng

### Sơ đồ đấu nối đơn giản

```
ESP32-CAM                   Component
=========================================
GPIO 14  ------------->    DHT22 (Data)
GPIO 13  ------------->    PIR (Out)
GPIO 15  ------------->    Relay 1 (IN)
GPIO 2   ------------->    Relay 2 (IN)
GPIO 12  ------------->    BH1750 SDA, RTC SDA
GPIO 4   ------------->    BH1750 SCL, RTC SCL
GND      ------------->    GND (all components)
5V       ------------->    VCC (DHT22, BH1750, RTC)
VIN      ------------->    VCC (Relay board, PIR)
```

### Pull-up cho I2C
BH1750 và RTC DS1307 có thể cần điện trở pull-up 4.7kΩ hoặc 10kΩ từ SDA/SCL lên 3.3V nếu bus I2C không ổn định.

## Khắc phục sự cố

### 1. I2C không hoạt động
- Kiểm tra pull-up resistors (4.7kΩ)
- Kiểm tra đúng chân SDA=12, SCL=4
- Dùng I2C scanner để phát hiện địa chỉ thiết bị

### 2. Upload thất bại
- Đảm bảo GPIO 0 nối GND trước khi upload
- Nhấn nút RESET trên ESP32-CAM
- Kiểm tra USB-to-TTL adapter (3.3V, không phải 5V)

### 3. DHT22 đọc NaN
- Kiểm tra đúng chân GPIO 14
- Đảm bảo DHT22 có VCC 5V
- Pull-up 10kΩ từ data pin lên VCC (nếu cần)

### 4. Relay không chuyển đổi
- Kiểm tra `kRelayActiveLow` trong `Config.h`
- ESP32-CAM có thể cần logic HIGH để bật relay
- Đảm bảo relay board có nguồn riêng đủ mạnh

## Tính năng đã loại bỏ - Giải pháp thay thế

### 1. MQ-135 (Air Quality)
**Lý do loại bỏ:** Không cần thiết cho hệ thống tưới cây cơ bản.

**Giải pháp thay thế (nếu cần):**
- Sử dụng ESP32 DevKit riêng cho monitoring không khí
- Thay thế bằng cảm biến I2C (CCS811, BME680) để tiết kiệm chân

### 2. Manual Button
**Lý do loại bỏ:** Điều khiển hoàn toàn qua ThingsBoard.

**Giải pháp thay thế:**
- Sử dụng ThingsBoard Dashboard để bật/tắt đèn
- Sử dụng RPC commands từ mobile app
- Nếu thực sự cần button: dùng GPIO 0 (lưu ý xung đột với flash button)

## Checklist hoàn thành migration

- [x] Cập nhật pin definitions trong `Config.h`
- [x] Loại bỏ MQ-135 code và includes
- [x] Loại bỏ Button code và includes
- [x] Cập nhật I2C initialization: `Wire.begin(12, 4)`
- [x] Cập nhật `platformio.ini` board: `esp32cam`
- [x] Cập nhật startup message: "ESP32-CAM starting..."
- [x] Build thành công không lỗi
- [ ] Test phần cứng: DHT22, PIR, BH1750, RTC
- [ ] Test I2C bus hoạt động ổn định
- [ ] Test relay switching
- [ ] Test WiFi connection
- [ ] Test ThingsBoard telemetry
- [ ] Test ThingsBoard RPC commands

## Tài liệu tham khảo

- [ESP32-CAM Pinout](https://randomnerdtutorials.com/esp32-cam-ai-thinker-pinout/)
- [ESP32 I2C with Custom Pins](https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/)
- [Deployment Guide](./deploy-to-device.md)
- [Hardware Wiring](./hardware.md) (cần cập nhật cho ESP32-CAM)

---

**Migration Date:** January 2026  
**Target Board:** ESP32-CAM (AI-Thinker)  
**Status:** ✅ Code migration completed - Hardware testing pending
