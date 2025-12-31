# Dumb Device Mode - ThingsBoard Integration

## Tổng quan

ESP32 hoạt động theo mô hình **"Dumb Device"** (Thiết bị chấp hành):
- ✅ Gửi dữ liệu cảm biến lên Server
- ✅ Nhận lệnh từ Server để điều khiển đèn
- ❌ KHÔNG tự tính toán logic bật/tắt

**Lợi ích:**
- Logic tập trung ở Server → dễ thay đổi/nâng cấp không cần flash lại ESP32
- Giảm độ phức tạp code ESP32
- Dễ debug/monitor từ ThingsBoard Dashboard

---

## 1. Giao thức MQTT với ThingsBoard

### A. Gửi Telemetry (ESP32 → Server)

**Topic:** `v1/devices/me/telemetry`

**Payload JSON:**
```json
{
  "temperature_c": 25.5,
  "humidity_pct": 60.0,
  "light_lux": 450.2,
  "motion": false,
  "air_quality_raw": 512,
  "light_on": true,
  "valve_on": false
}
```

**⚠️ QUAN TRỌNG:**
- Key `temperature_c` phải chính xác (Rule Chain filter theo tên này)
- ESP32 gửi định kỳ (mặc định: 10 giây)

### B. Nhận Shared Attributes (Server → ESP32)

**Topic:** `v1/devices/me/attributes`

**Attribute Key:** `self_light_enable`

**Kiểu dữ liệu:** `boolean` (true/false)

**Hành động ESP32:**
- `true` → Bật Relay/Đèn (GPIO HIGH)
- `false` → Tắt Relay/Đèn (GPIO LOW)

**Cơ chế nhận:**
1. **Subscribe realtime:** ESP32 tự động nhận khi Server thay đổi attribute
2. **Request on boot:** ESP32 hỏi Server về trạng thái hiện tại khi khởi động

---

## 2. Luồng hoạt động (Use Case)

### Kịch bản: Nhiệt độ xuống thấp

```
Bước 1: ESP32 gửi {"temperature_c": 15} (dưới 18°C)
        ↓
Bước 2: Server Rule Chain nhận thấy → Set self_light_enable = true
        ↓
Bước 3: ESP32 nhận attribute update → Bật đèn relay
        ↓
Bước 4: (Sau 1 phút) Server tự động set self_light_enable = false
        ↓
Bước 5: ESP32 nhận lệnh mới → Tắt đèn
```

### Kịch bản: Mất điện giữa chừng

```
Bước 1: ESP32 mất điện/reset
        ↓
Bước 2: ESP32 boot lại → Connect ThingsBoard
        ↓
Bước 3: ESP32 request shared attributes
        ↓
Bước 4: Server trả về self_light_enable = true (vẫn đang trong chu kỳ sưởi)
        ↓
Bước 5: ESP32 đồng bộ → Bật đèn tiếp
```

---

## 3. Cấu hình Code ESP32

### File quan trọng:

| File | Nhiệm vụ |
|------|----------|
| `src/controllers/LightController.cpp` | Logic đơn giản: chỉ nghe `self_light_enable` |
| `src/app/Telemetry.cpp` | Build JSON với `temperature_c` |
| `src/app/RemoteConfigManager.cpp` | Parse attributes từ ThingsBoard |
| `src/main.cpp` | Subscribe attributes + request on boot |

### Các thông số cấu hình:

```cpp
// include/Config.h
constexpr uint32_t kTelemetryIntervalMs = 10000;    // Gửi telemetry 10s/lần
constexpr uint32_t kSensorReadIntervalMs = 5000;    // Đọc cảm biến 5s/lần
```

**⚠️ Lưu ý:** Có thể override từ ThingsBoard Shared Attributes:
```json
{
  "telemetryIntervalMs": 15000,
  "sensorReadIntervalMs": 8000
}
```

---

## 4. Testing & Debugging

### A. Test trên Serial Monitor

**Khi gửi telemetry:**
```
Telemetry: {"temperature_c":15.5,"humidity_pct":60.0,...}
```

**Khi nhận attribute:**
```
✅ Applied remote config from ThingsBoard attributes
💡 Light state changed: ON
```

**Khi request attributes:**
```
📡 Requested shared attributes from ThingsBoard
```

### B. Test thủ công trên ThingsBoard Dashboard

1. **Vào Device → Attributes tab**
2. **Thêm/sửa Shared Attribute:**
   - Key: `self_light_enable`
   - Value: `true` hoặc `false`
3. **Save** → ESP32 nhận ngay lập tức

### C. Kiểm tra Rule Chain

1. **Latest Telemetry tab:** Xem `temperature_c` có đẩy lên đúng không
2. **Rule Chain Debug:** Bật debug mode xem flow chạy qua node nào
3. **Audit Logs:** Xem lịch sử thay đổi attributes

---

## 5. Manual Override (Nút nhấn Local)

ESP32 vẫn hỗ trợ **nút nhấn vật lý** để tắt đèn khẩn cấp:

```cpp
// GPIO button (kéo xuống GND để kích hoạt)
if (lightManualButton.update(nowMs)) {
  settings.toggleManualOff();
  // manualOff = true → Đèn bị force OFF bất kể self_light_enable
}
```

**Ưu tiên:**
1. ❶ Manual OFF button (local physical button)
2. ❷ self_light_enable (from ThingsBoard)

---

## 6. Troubleshooting

### ❌ Đèn không bật dù Server đã set self_light_enable = true

**Kiểm tra:**
1. ESP32 có nhận được attribute? → Check Serial: `Applied remote config`
2. Nút manual OFF có bị bấm? → Check Serial: `Manual light OFF latch: ON`
3. Relay wiring đúng? → Test bằng RPC `setLight`

### ❌ ESP32 không nhận được attribute update

**Kiểm tra:**
1. MQTT connected? → Check Serial: `MQTT connected`
2. Subscribe đúng topic? → `setAttributesHandler` đã được gọi
3. ThingsBoard Access Token đúng? → Check `include/Secrets.h`

### ❌ Telemetry không lên Server

**Kiểm tra:**
1. WiFi connected? → Check Serial: `WiFi connected`
2. JSON format đúng? → Check Serial: `Telemetry: {...}`
3. PubSubClient buffer đủ lớn? → Mặc định 512 bytes

---

## 7. Migration từ mode cũ

### Thay đổi chính:

| Trước (Smart Device) | Sau (Dumb Device) |
|---------------------|-------------------|
| ESP32 tính toán logic nhiệt độ | Server tính toán |
| Cần flash code để thay đổi logic | Chỉ cần sửa Rule Chain |
| Phức tạp, khó debug | Đơn giản, dễ monitor |

### Code cũ (DEPRECATED):

```cpp
// ❌ KHÔNG còn dùng logic này
if (dht.temperatureC <= 18.0f) {
  lightRelay.setOn(true);  // ESP32 tự quyết định
}
```

### Code mới:

```cpp
// ✅ Chỉ nghe lệnh từ Server
desiredOn = settings.selfLightEnable();  // Server quyết định
relay_.setOn(desiredOn);
```

---

## 8. Tham khảo

- ThingsBoard MQTT API: https://thingsboard.io/docs/reference/mqtt-api/
- Rule Chain Documentation: https://thingsboard.io/docs/user-guide/rule-engine-2-0/overview/
- Shared Attributes: https://thingsboard.io/docs/user-guide/attributes/
