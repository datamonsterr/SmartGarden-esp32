# Smart Garden use-cases (step-by-step)

This guide explains how to use the firmware end-to-end (Wokwi or real ESP32), what telemetry to expect for each sensor, and how to control the light/valve.

## 0) Before you start

1. Build and run the firmware (Wokwi) or upload it (real device).
2. Confirm the device shows **ACTIVE** in ThingsBoard.
3. Open the device in ThingsBoard → **Latest telemetry**.

## 1) Sensor telemetry: expected keys and behavior

### Temperature / Humidity (DHT22)

Telemetry keys:
- `temperature_c` (number or null)
- `humidity_pct` (number or null)

Expected behavior:
- Normal: `temperature_c` and `humidity_pct` are numbers.
- If the DHT read fails: both keys become `null` (the device stays online).

### Motion (PIR)

Telemetry keys:
- `motion` (boolean)

Expected behavior:
- `motion=true` when PIR output is HIGH.
- `motion=false` when PIR output is LOW.

### Air quality (MQ-135 raw analog)

Telemetry keys:
- `air_quality_raw` (integer)

Expected behavior:
- This is a *raw ADC value* (trend indicator, not calibrated PPM).
- Higher/lower values depend on your module + wiring; watch trends over time.

### Soil moisture (raw analog)

Telemetry keys:
- `soil_raw` (integer)
- `soil_is_dry` (boolean)

Expected behavior:
- `soil_raw` is a raw ADC value.
- `soil_is_dry` toggles based on thresholds (see Remote Config section below).

## 2) Light control use-cases

Telemetry keys:
- `light_on` (boolean)
- `light_manual_off` (boolean)
- `light_remote_override` (boolean)
- `light_temp_limit_enabled` (boolean)
- `light_temp_too_cold_c` (number or null)

Decision priority (highest wins):
1. Manual OFF latch (local button)
2. ThingsBoard remote override (RPC / schedule)
3. Auto mode (motion OR “too cold”)

### 2.1 Auto: motion-triggered light

1. Ensure `light_manual_off=false`.
2. Ensure `light_remote_override=false` (call RPC `clearLightOverride` if needed).
3. Trigger motion (PIR HIGH).

Expected behavior:
- Light turns ON immediately when motion is detected.
- Light turns OFF after `lightOnAfterMotionMs` since the last motion.

### 2.2 Auto: temperature limit (too cold → light ON)

1. Call RPC `setTempLimitEnabled` with `true`.
2. Call RPC `setTempLimit` with a temperature threshold (example: `18.0`).
3. If `temperature_c` falls to or below the threshold, the light is requested ON.

Expected behavior:
- Light turns ON when temperature is below/equal the threshold.
- Light turns OFF when temperature rises above threshold + hysteresis.

### 2.3 Schedule / manual remote control (ThingsBoard)

1. Call RPC `setLight` with `true` (force ON) or `false` (force OFF).

Expected behavior:
- `light_remote_override=true` and `light_on` follows the forced state.
- Auto logic is ignored until you call `clearLightOverride`.

### 2.4 Manual OFF button (physical)

1. Press the physical button on GPIO14 (to GND, uses internal pull-up).

Expected behavior:
- The “manual OFF latch” toggles each press.
- When latched: `light_manual_off=true` and light stays OFF.

## 3) Watering control use-cases

Telemetry keys:
- `valve_on` (boolean)
- `soil_is_dry` (boolean)

Expected behavior:
- When `soil_is_dry=true`, the valve turns ON after `minValveOffMs` has elapsed.
- When `soil_is_dry=false`, the valve turns OFF after `minValveOnMs` has elapsed.

## 4) Remote config use-case (per-device settings)

This project supports configuring behavior per device via **ThingsBoard Shared Attributes**.

- `include/Config.h` provides safe compile-time defaults.
- The device requests Shared Attributes after connecting.
- If attributes exist, they override defaults.
- The last received config is persisted on the ESP32 (NVS) so it survives reboot.

See `docs/thingsboard-cloud-setup.md` for the exact keys and setup steps.
