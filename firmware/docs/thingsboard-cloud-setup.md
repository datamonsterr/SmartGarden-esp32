# ThingsBoard Cloud setup (telemetry via MQTT)

This project uses **ThingsBoard Cloud** MQTT device connectivity with **Access Token** authentication.

## 1) Create account / tenant

1. Go to <https://thingsboard.cloud/>
2. Sign up (or log in)

## 2) Create a device

1. In ThingsBoard UI: **Devices** → **+ Add new device**
2. Enter a name (e.g. `Garden ESP32`)
3. Create

## 3) Get the device Access Token

1. Open the device details
2. Find **Credentials**
3. Copy the **Access token**

This token is used by the ESP32 as MQTT **username**.

## 4) Configure the ESP32 secrets

1. Copy `include/Secrets.h.example` → `include/Secrets.h`
2. Set:
   - `kWifiSsid`, `kWifiPassword`
   - `kThingsBoardHost` (default `thingsboard.cloud`)
   - `kThingsBoardPort` (default `1883`)
   - `kThingsBoardAccessToken`

## 5) Verify MQTT connectivity (optional)

If you have `mosquitto_pub` installed, you can test with the same token:

```bash
mosquitto_pub -d -q 1 -h "thingsboard.cloud" -p "1883" \
  -t "v1/devices/me/telemetry" -u "YOUR_ACCESS_TOKEN" -m '{"temperature":25}'
```

## 6) See incoming data

1. Open the device in ThingsBoard
2. Go to **Latest telemetry**
3. You should see keys like:
   - `temperature_c`, `humidity_pct`
   - `motion`, `air_quality_raw`, `soil_raw`
   - `light_on`, `light_intensity_pct`, `valve_on`, `soil_is_dry`

## 7) Control the light (intensity / schedule / auto)

The firmware supports server-side control via ThingsBoard **RPC**. The light is **dimmable** (PWM controlled, 0–100% intensity), not just ON/OFF.

### Supported RPC methods

| Method | Params | Description |
|--------|--------|-------------|
| `setLightIntensity` | `0–100` (number) | Set light intensity percentage (0 = off, 100 = full) |
| `setLightMode` | `"manual"`, `"auto"`, `"schedule"` | Switch light control mode |
| `setLightAutoConstant` | number | Set the constant for auto formula: `power = (1/temp) * constant` |
| `setLightSchedule` | JSON object | Set schedule rules (see below) |
| `clearLightOverride` | none | Return to automatic logic |
| `setTempLimit` | number (°C) | Enable "too cold → light ON" and set threshold |
| `setTempLimitEnabled` | `true`/`false` | Enable/disable temperature-based light control |

### Light control modes

1. **Manual mode:** User sets intensity directly via `setLightIntensity` RPC or mobile app slider
2. **Auto mode:** ESP32 calculates intensity using: `light_power = (1 / temperature_c) * lightAutoConstant`
3. **Schedule mode:** Light follows time-based rules

### Schedule format

```json
{
  "rules": [
    { "startHour": 6, "endHour": 18, "intensity": 80 },
    { "startHour": 18, "endHour": 22, "intensity": 30 },
    { "startHour": 22, "endHour": 6, "intensity": 0 }
  ]
}
```

Ways to use RPC:
- **Dashboard:** Add an RPC control widget (slider for `setLightIntensity`, buttons for mode)
- **Mobile app:** Send RPC requests via ThingsBoard REST API
- **Rule chain:** Create automation rules that trigger RPC based on conditions

## 8) Remote config (Shared Attributes) — per user/device settings

This firmware supports overriding `include/Config.h` defaults from ThingsBoard, per device, using **Shared Attributes**:

- Device requests these keys after MQTT connects.
- Any values found override local defaults.
- The last received values are persisted on the ESP32 (NVS) so they survive reboots.
- If you never set attributes, the firmware behaves exactly like `include/Config.h`.

### 8.1 Add Shared Attributes

1. In ThingsBoard, open **Devices** → your device.
2. Go to **Attributes**.
3. In the **Shared attributes** tab, add the keys you want to override.

### Supported keys (name → type → meaning)

#### Telemetry & Sensors
| Key | Type | Description |
|-----|------|-------------|
| `telemetryIntervalMs` | number (ms) | Telemetry publish period |
| `sensorReadIntervalMs` | number (ms) | Sensor read period |

#### Light Control
| Key | Type | Description |
|-----|------|-------------|
| `lightMode` | string | `"manual"`, `"auto"`, or `"schedule"` |
| `lightIntensityPct` | number (0–100) | Manual mode intensity |
| `lightAutoConstant` | number | Constant for auto formula |
| `lightSchedule` | JSON | Schedule rules array |
| `lightOnAfterMotionMs` | number (ms) | Motion timeout for light (when motion mode enabled) |
| `tempLightEnabled` | boolean | Enable "too cold → light ON" |
| `tempTooColdC` | number (°C) | Cold threshold |

#### Watering Control
| Key | Type | Description |
|-----|------|-------------|
| `soilWetThreshold` | number | Stop watering when `soil_raw >= soilWetThreshold` |
| `soilDryThreshold` | number | Start watering when `soil_raw <= soilDryThreshold` |
| `minValveOnMs` | number (ms) | Minimum ON duration for valve |
| `minValveOffMs` | number (ms) | Minimum OFF duration for valve |

### 8.2 Verify it worked

1. Watch the device serial log for `Applied remote config (attributes)`.
2. Confirm behavior changes (light intensity / watering threshold changes).

## 9) Mobile app integration

To control the light from a mobile app:

1. Use ThingsBoard REST API to send RPC commands
2. Example: Set light intensity to 75%

```bash
curl -X POST \
  "https://thingsboard.cloud/api/plugins/rpc/twoway/DEVICE_ID" \
  -H "X-Authorization: Bearer YOUR_JWT_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"method": "setLightIntensity", "params": 75}'
```

3. Or use the ThingsBoard mobile app / custom app with REST API integration

Tip: see `docs/use-cases.md` for expected behaviors per sensor and control mode.
