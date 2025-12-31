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

The light is **dimmable** (PWM controlled, 0–100% intensity), not just ON/OFF.

Telemetry keys:
- `light_on` (boolean) — whether light is active (intensity > 0)
- `light_intensity_pct` (number 0–100) — current intensity percentage
- `light_mode` (string) — `"manual"`, `"auto"`, or `"schedule"`
- `light_remote_override` (boolean)
- `light_temp_limit_enabled` (boolean)
- `light_temp_too_cold_c` (number or null)

### Light control modes

The ESP32 handles all light control logic locally to minimize message transfer:

| Mode | Description | Control source |
|------|-------------|----------------|
| **Manual** | User sets intensity directly | Mobile app slider / RPC |
| **Auto** | ESP32 calculates intensity based on temperature | Edge formula |
| **Schedule** | Time-based intensity rules | Edge schedule |

### 2.1 Manual mode: app-controlled intensity

1. Call RPC `setLightMode` with `"manual"`.
2. Call RPC `setLightIntensity` with value 0–100.

Expected behavior:
- Light intensity changes immediately.
- `light_intensity_pct` telemetry reflects the set value.
- Use the mobile app slider for real-time adjustment.

### 2.2 Auto mode: temperature-based intensity

Auto mode uses the formula: **`light_power = (1 / temperature_c) * constant`**

This is useful for grow lights where colder temperatures require more supplemental light/heat.

Setup:
1. Call RPC `setLightMode` with `"auto"`.
2. Call RPC `setLightAutoConstant` with your desired constant (e.g., `2500`).

Expected behavior:
- At 25°C with constant=2500: `power = (1/25) * 2500 = 100%`
- At 30°C with constant=2500: `power = (1/30) * 2500 = 83%`
- At 20°C with constant=2000: `power = (1/20) * 2000 = 100%`

The constant can be updated remotely via:
- RPC: `setLightAutoConstant`
- Shared Attribute: `lightAutoConstant`

### 2.3 Schedule mode: time-based intensity

Schedule mode follows configured time rules for intensity.

Setup:
1. Call RPC `setLightMode` with `"schedule"`.
2. Call RPC `setLightSchedule` with schedule JSON:

```json
{
  "rules": [
    { "startHour": 6, "endHour": 18, "intensity": 80 },
    { "startHour": 18, "endHour": 22, "intensity": 30 },
    { "startHour": 22, "endHour": 6, "intensity": 0 }
  ]
}
```

Expected behavior:
- 6:00–18:00: Light at 80%
- 18:00–22:00: Light at 30%
- 22:00–6:00: Light OFF

Update the schedule remotely via:
- RPC: `setLightSchedule`
- Shared Attribute: `lightSchedule`

### 2.4 Temperature limit (too cold → boost light)

Works in any mode as an additional override.

1. Call RPC `setTempLimitEnabled` with `true`.
2. Call RPC `setTempLimit` with a temperature threshold (example: `18.0`).

Expected behavior:
- If `temperature_c` falls to or below the threshold, light intensity is boosted.
- Light returns to normal when temperature rises above threshold + hysteresis.

### 2.5 Motion-triggered (optional)

When enabled, motion detection can temporarily boost or activate the light.

1. Configure `lightOnAfterMotionMs` via Shared Attributes.
2. When motion is detected, light activates for the specified duration.

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

## 5) Updating logic remotely

All edge logic (auto formula, schedules) can be updated without reflashing:

| What to update | Method |
|----------------|--------|
| Auto mode constant | Shared Attribute `lightAutoConstant` or RPC `setLightAutoConstant` |
| Schedule rules | Shared Attribute `lightSchedule` or RPC `setLightSchedule` |
| Thresholds | Shared Attributes (see thingsboard-cloud-setup.md) |

This keeps message transfer minimal while allowing full remote configurability.

## 6) Example: complete setup for a grow light

1. **Initial setup:**
   - Set `lightMode` = `"auto"` via Shared Attribute
   - Set `lightAutoConstant` = `2500`
   - Set `tempLightEnabled` = `true`, `tempTooColdC` = `15`

2. **Runtime behavior:**
   - Light intensity auto-adjusts based on temperature
   - If temp drops below 15°C, light boosts to help warm plants
   - All logic runs on ESP32 — no constant cloud communication needed

3. **User override:**
   - User opens mobile app, drags slider to 50%
   - App sends RPC `setLightMode` = `"manual"`, `setLightIntensity` = `50`
   - Light stays at 50% until user changes mode back to auto
