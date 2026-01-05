# Debug ThingsBoard Rule Chain

## ⚠️ Vấn đề hiện tại

ESP32 **GỬI** telemetry thành công với `temperature_c = 23.7°C` (dưới 24°C), nhưng Rule Chain **KHÔNG tự động** set `self_light_enable = true`.

Chỉ khi **manually toggle switch** trên Dashboard thì ESP32 mới nhận được attribute.

---

## 🔍 Các bước debug Rule Chain

### 1. Kiểm tra Rule Chain có active không

**Trên ThingsBoard Dashboard:**
1. Vào **Rule Chains** (menu bên trái)
2. Tìm Rule Chain của bạn (ví dụ: "Smart Garden Auto Light")
3. Kiểm tra trạng thái:
   - ✅ **Active** (màu xanh)
   - ❌ **Inactive** (màu xám) → Click **Activate**

---

### 2. Enable Debug Mode cho Rule Chain

**Bật debug để xem message flow:**

1. Vào **Rule Chains** → Chọn rule chain của bạn
2. Click vào **Root Rule Chain** node (node đầu tiên)
3. Trong panel bên phải, bật **Debug Mode**
4. Click **Apply changes**

**Kết quả:** Mọi message đi qua rule chain sẽ được log ra.

---

### 3. Kiểm tra Latest Telemetry của Device

**Xem ESP32 có gửi data lên không:**

1. Vào **Devices** → Chọn thiết bị Smart Garden
2. Tab **Latest Telemetry**
3. Kiểm tra:
   - ✅ `temperature_c` có xuất hiện không?
   - ✅ Giá trị có đúng? (23.7°C)
   - ✅ Timestamp có update realtime không?

**Nếu KHÔNG thấy `temperature_c`:**
- MQTT connection có issue
- Topic publish sai
- Access Token sai

---

### 4. Kiểm tra Filter Node trong Rule Chain

**Rule Chain phải có node filter temperature:**

#### **Node cần có:**

**A. Message Type Switch Node**
- Input: From **Input** node
- Output routes:
  - `Post telemetry` → Đi tiếp đến temperature filter
  - `Post attributes` → (optional)

**B. Script Filter Node** (hoặc **Switch Node**)

**Config cho Script Filter:**
```javascript
// Check if temperature_c exists and < 24
if (metadata.temperature_c !== undefined) {
    var temp = parseFloat(metadata.temperature_c);
    return temp < 24.0;  // ⚠️ QUAN TRỌNG: Điều kiện này phải match
}
return false;
```

**HOẶC dùng Switch Node:**
- Condition: `$[temperature_c] < 24`
- Output: `True` → Save attribute
- Output: `False` → Do nothing

---

### 5. Kiểm tra Save Attributes Node

**Node cuối cùng để set `self_light_enable`:**

**Config:**
- **Server attributes** hoặc **Shared attributes**
- Key-value pairs:
  ```json
  {
    "self_light_enable": true
  }
  ```

**⚠️ LƯU Ý:**
- Phải chọn **Shared attributes** (không phải Client attributes)
- ESP32 chỉ subscribe `shared` attributes

---

### 6. Test Rule Chain với Debug Events

**Xem message flow realtime:**

1. Enable **Debug Mode** (bước 2)
2. Vào **Events** tab của Device
3. Chọn filter: **Debug events**
4. Quan sát:
   - Message đi vào Input node
   - Message đi qua Filter node
   - Message đi đến Save Attributes node

**Nếu thấy message dừng ở Filter node:**
- Điều kiện filter SAI
- Metadata không có `temperature_c`
- Kiểu dữ liệu sai (string vs number)

---

## 🛠️ Rule Chain mẫu (JSON Export)

**Để import vào ThingsBoard:**

```json
{
  "ruleChain": {
    "name": "Smart Garden Auto Light",
    "type": "CORE",
    "firstRuleNodeId": null,
    "root": false,
    "debugMode": true,
    "configuration": null
  },
  "metadata": {
    "firstNodeIndex": 0,
    "nodes": [
      {
        "name": "Input",
        "type": "org.thingsboard.rule.engine.telemetry.TbMsgTimeseriesNode",
        "configuration": {
          "defaultTTL": 0
        }
      },
      {
        "name": "Message Type Switch",
        "type": "org.thingsboard.rule.engine.filter.TbMsgTypeSwitchNode",
        "configuration": {}
      },
      {
        "name": "Temperature Filter",
        "type": "org.thingsboard.rule.engine.filter.TbJsFilterNode",
        "configuration": {
          "jsScript": "return msg.temperature_c !== undefined && parseFloat(msg.temperature_c) < 24.0;"
        }
      },
      {
        "name": "Save Self Light Enable TRUE",
        "type": "org.thingsboard.rule.engine.action.TbSaveAttributesNode",
        "configuration": {
          "scope": "SHARED_SCOPE",
          "notifyDevice": true
        },
        "additionalInfo": {
          "layoutX": 600,
          "layoutY": 200
        }
      }
    ],
    "connections": [
      {
        "fromIndex": 0,
        "toIndex": 1,
        "type": "Success"
      },
      {
        "fromIndex": 1,
        "toIndex": 2,
        "type": "Post telemetry"
      },
      {
        "fromIndex": 2,
        "toIndex": 3,
        "type": "True"
      }
    ]
  }
}
```

---

## ✅ Checklist debug

- [ ] Rule Chain đang **Active**
- [ ] Debug Mode đang **ON**
- [ ] Device Latest Telemetry có `temperature_c`
- [ ] Filter node có điều kiện `< 24.0`
- [ ] Save Attributes node dùng **Shared Scope**
- [ ] Save Attributes node có **Notify Device** = true
- [ ] Events tab thấy message flow qua các node

---

## 🔧 Test thủ công

**Test trực tiếp Rule Chain:**

1. Vào **Rule Chains** → Chọn rule chain
2. Click nút **Test Rule Chain** (icon play ở góc trên)
3. Nhập JSON test:
   ```json
   {
     "temperature_c": 20.5,
     "humidity_pct": 80
   }
   ```
4. Click **Test**
5. Xem output có `self_light_enable: true` không

---

## 📊 Expected vs Actual

### **Expected Flow:**
```
ESP32 gửi telemetry (temp=23.7°C)
    ↓
ThingsBoard nhận message
    ↓
Rule Chain filter: temp < 24? → TRUE
    ↓
Save Attribute: self_light_enable = true
    ↓
ESP32 nhận attribute update (realtime)
    ↓
ESP32 bật đèn
```

### **Actual Flow hiện tại:**
```
ESP32 gửi telemetry (temp=23.7°C) ✅
    ↓
ThingsBoard nhận message ✅
    ↓
Rule Chain filter: ??? ❌ (Có thể bị skip)
    ↓
Manual toggle switch ✅
    ↓
ESP32 nhận attribute ✅
    ↓
ESP32 bật đèn ✅
```

**→ Vấn đề ở Rule Chain, KHÔNG phải ESP32!**

---

## 💡 Tips

1. **Kiểm tra metadata vs message body:**
   - Telemetry data nằm trong `msg` object (không phải `metadata`)
   - Script filter phải dùng: `msg.temperature_c` (không phải `metadata.temperature_c`)

2. **Kiểm tra kiểu dữ liệu:**
   - ThingsBoard có thể parse `temperature_c` thành string
   - Phải dùng `parseFloat(msg.temperature_c)` để so sánh số

3. **Test với giá trị cực đoan:**
   - Gửi `temperature_c: 10` → Rule phải trigger
   - Gửi `temperature_c: 30` → Rule phải KHÔNG trigger

4. **Check Rule Chain Timeout:**
   - Nếu Rule Chain xử lý quá lâu → có thể bị timeout
   - Enable debug để xem execution time

---

## 📞 Nếu vẫn lỗi

**Export Rule Chain và chia sẻ:**
1. Vào Rule Chains → Click icon **Export**
2. Save file JSON
3. Gửi cho team để review

**Hoặc screenshot:**
- Rule Chain flow diagram
- Filter node configuration
- Save Attributes node configuration
- Events tab với debug messages
