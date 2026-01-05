# Fix ThingsBoard Rule Chain - ON/OFF Conflict

## ⚠️ Vấn đề phát hiện

Rule Chain đang gửi **2 messages liên tiếp** (cách nhau chỉ 41ms):
```
172709 ms: self_light_enable = TRUE   → Đèn BẬT
172750 ms: self_light_enable = FALSE  → Đèn TẮT ngay (bug!)
```

**Nhiệt độ vẫn = 23.5°C (dưới 24°C)** nhưng đèn bị tắt → Rule Chain logic SAI!

---

## 🔍 Nguyên nhân

### **Có 2 Rule Nodes conflict:**

**Node 1: Temperature-based ON**
```
IF temperature < 24°C:
    Set self_light_enable = TRUE
```

**Node 2: Timer-based OFF (SAI!)**
```
Sau 1 phút:
    Set self_light_enable = FALSE  ← Không kiểm tra nhiệt độ!
```

→ Node 2 tắt đèn **BẤT KỂ** nhiệt độ thế nào!

---

## ✅ Giải pháp: Rule Chain với Hysteresis

### **Logic ĐÚNG:**

```
IF temperature < 23°C:
    Set self_light_enable = TRUE
    
IF temperature > 25°C:
    Set self_light_enable = FALSE
    
ELSE (23°C ≤ temp ≤ 25°C):
    GIỮ NGUYÊN trạng thái cũ (không thay đổi)
```

**Lợi ích:**
- ✅ Tránh đèn nhấp nháy khi nhiệt độ dao động quanh 24°C
- ✅ Đèn chỉ TẮT khi nhiệt độ thực sự ấm (>25°C)
- ✅ Không có timer conflict

---

## 🛠️ Cách sửa Rule Chain

### **Bước 1: Xóa Timer Node (nếu có)**

1. Vào **Rule Chains** → Chọn rule chain của bạn
2. Tìm node **Delay** hoặc **Generator** (tạo message sau X phút)
3. **XÓA** node đó và connections liên quan

### **Bước 2: Sửa Filter Nodes**

**Node A: Turn ON Filter**
```javascript
// Bật đèn khi QUÁ LẠNH
var temp = parseFloat(msg.temperature_c);
return temp < 23.0;  // ← Giảm xuống 23°C (thay vì 24°C)
```

**Output:** TRUE → **Save Attribute: self_light_enable = true**

---

**Node B: Turn OFF Filter**
```javascript
// Tắt đèn khi ĐỦ ẤM
var temp = parseFloat(msg.temperature_c);
return temp > 25.0;  // ← Tăng lên 25°C (hysteresis 2°C)
```

**Output:** TRUE → **Save Attribute: self_light_enable = false**

---

### **Bước 3: Flow hoàn chỉnh**

```
┌─────────────────┐
│  Input (Post    │
│  Telemetry)     │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Check if       │
│  temperature_c  │
│  exists         │
└────┬───────┬────┘
     │       │
   TRUE    FALSE → END
     │
     ▼
┌──────────────────────────────────┐
│  Switch Node:                    │
│  - Route 1: temp < 23  → "Cold"  │
│  - Route 2: temp > 25  → "Warm"  │
│  - Default: 23≤temp≤25 → "OK"    │
└──┬───────┬───────┬───────────────┘
   │       │       │
 Cold    Warm     OK
   │       │       └─→ END (no change)
   │       │
   ▼       ▼
┌─────┐ ┌─────┐
│ Set │ │ Set │
│ ON  │ │ OFF │
└─────┘ └─────┘
```

---

## 📋 Rule Chain JSON Export (Import-ready)

**Copy đoạn này vào ThingsBoard:**

### **Script cho Switch Node:**

```javascript
// Hysteresis control với 2°C gap
var temp = parseFloat(msg.temperature_c);

if (isNaN(temp)) {
    return ['Invalid'];
}

if (temp < 23.0) {
    return ['Cold'];  // Bật đèn
} else if (temp > 25.0) {
    return ['Warm'];  // Tắt đèn
} else {
    return ['OK'];    // Giữ nguyên
}
```

### **Save Attribute Node (Cold → ON):**
```json
{
  "scope": "SHARED_SCOPE",
  "notifyDevice": true,
  "attributes": {
    "self_light_enable": true
  }
}
```

### **Save Attribute Node (Warm → OFF):**
```json
{
  "scope": "SHARED_SCOPE",
  "notifyDevice": true,
  "attributes": {
    "self_light_enable": false
  }
}
```

---

## 🧪 Test Case

| Nhiệt độ | Trạng thái cũ | Hành động | Trạng thái mới |
|----------|---------------|-----------|----------------|
| 22°C     | OFF           | Bật       | ON             |
| 23°C     | OFF           | Không đổi | OFF            |
| 23°C     | ON            | Không đổi | ON ← **Giữ nguyên** |
| 24°C     | ON            | Không đổi | ON             |
| 25°C     | ON            | Không đổi | ON             |
| 26°C     | ON            | Tắt       | OFF            |
| 26°C     | OFF           | Không đổi | OFF            |

**→ Đèn sẽ BẬT ở 22°C và TẮT ở 26°C (gap 4°C = ổn định)**

---

## 🔧 Giải pháp tạm thời (ESP32 side)

**Nếu chưa sửa được Rule Chain**, ESP32 đã có **SAFETY CHECK**:

```cpp
// Nếu Server gửi OFF nhưng temp < 23°C → ESP32 IGNORE lệnh OFF
if (!desiredOn && dht.ok && dht.temperatureC < 23.0f) {
    Serial.println("⚠️ SAFETY: Ignoring OFF - temp too cold!");
    desiredOn = true;  // GIỮ ĐÈN BẬT
}
```

**Log sẽ hiện:**
```
⚠️  SAFETY: Ignoring OFF command - Temperature still too cold!
   Temperature: 23.5°C < 23°C → Keeping light ON
```

---

## 📊 So sánh Before/After

### **TRƯỚC (Bug):**
```
23.5°C → Rule gửi ON
(41ms sau)
23.5°C → Rule gửi OFF  ← BUG: temp vẫn lạnh!
```

### **SAU (Fix):**
```
22.0°C → Rule gửi ON
23.0°C → Không gửi gì (giữ nguyên ON)
24.0°C → Không gửi gì (giữ nguyên ON)
25.0°C → Không gửi gì (giữ nguyên ON)
26.0°C → Rule gửi OFF  ← OK: đủ ấm rồi
```

---

## 🎯 Action Items

- [ ] Vào ThingsBoard Rule Chain Editor
- [ ] Xóa Timer/Delay nodes (nếu có)
- [ ] Thêm Switch Node với hysteresis (23°C / 25°C)
- [ ] Test với nhiệt độ 22°C → Đèn phải BẬT
- [ ] Test với nhiệt độ 26°C → Đèn phải TẮT
- [ ] Test với 23-25°C → Đèn phải GIỮ NGUYÊN

---

## 💡 Tips Debug

**Kiểm tra Rule Chain có Timer không:**
1. Vào Rule Chain Editor
2. Tìm các node:
   - `Delay Node`
   - `Generator Node` (generate message sau X giây)
   - `Schedule Node`
3. Nếu có → XÓA (vì conflict với temperature logic)

**Enable Debug Mode:**
1. Rule Chain → Debug Mode ON
2. Events tab → Xem message flow
3. Nếu thấy 2 messages liên tiếp (ON rồi OFF) → Confirm bug

**Test thủ công:**
1. Device Attributes → Manually set `self_light_enable = true`
2. Đợi 1-2 phút
3. Nếu tự động đổi thành `false` → Có Timer node hidden!
