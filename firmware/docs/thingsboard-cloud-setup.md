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
   - `light_on`, `valve_on`, `soil_is_dry`

## 7) Control the light (schedule / manual)

The firmware supports server-side control via ThingsBoard **RPC**.

Supported RPC methods:

- `setLight` (params: `true`/`false`) – force light ON/OFF (useful for schedules)
- `clearLightOverride` (no params) – return to automatic logic
- `setTempLimit` (params: number, °C) – enable “too cold → light ON” and set threshold
- `setTempLimitEnabled` (params: `true`/`false`)
- `setManualOff` (params: `true`/`false`)
- `toggleManualOff` (no params)

Ways to use it:

- Dashboard: add an RPC control widget calling `setLight`
- Schedule: create a scheduler/rule-chain that triggers `setLight` at specific times

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

Supported keys (name → type → meaning):

- `telemetryIntervalMs` → number (ms) → telemetry publish period
- `sensorReadIntervalMs` → number (ms) → sensor read period
- `lightOnAfterMotionMs` → number (ms) → motion timeout for light
- `tempLightEnabled` → boolean → enable “too cold → light ON”
- `tempTooColdC` → number (°C) → cold threshold
- `soilWetThreshold` → number → stop watering when `soil_raw >= soilWetThreshold`
- `soilDryThreshold` → number → start watering when `soil_raw <= soilDryThreshold`
- `minValveOnMs` → number (ms) → minimum ON duration for valve
- `minValveOffMs` → number (ms) → minimum OFF duration for valve

### 8.2 Verify it worked

1. Watch the device serial log for `Applied remote config (attributes)`.
2. Confirm behavior changes (watering threshold / light timeout changes).

Tip: see `docs/use-cases.md` for expected behaviors per sensor and control mode.
