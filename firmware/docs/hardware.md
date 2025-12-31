# Hardware (Vietnam-friendly BOM)

This project uses common, low-cost parts that are widely available in Vietnam (e.g., Shopee/Lazada, local electronics shops like Nhật Tảo, online stores such as linh kiện điện tử / arduino shops).

## Recommended parts

### Core

- **ESP32 DevKit V1** (ESP32-WROOM-32)
  - Easy to find, USB programming, lots of GPIO
  - Built-in PWM support for dimming control

### Sensors

- **Temperature + Humidity:** DHT22 (AM2302)
  - More accurate than DHT11, common and cheap
  - 3.3V compatible

- **Motion:** HC-SR501 PIR sensor
  - Adjustable sensitivity + delay
  - Usually outputs 3.3V/5V logic (works with ESP32)

- **Air quality (simple):** MQ-135 analog gas sensor module
  - Provides an **analog value**; good for relative changes (not calibrated PPM by default)
  - Many modules are powered at 5V; analog output must stay within ESP32 ADC range (0–3.3V)

- **Soil moisture (recommended):** Capacitive soil moisture sensor (analog)
  - Prefer capacitive over resistive (resistive corrodes quickly)
  - Many modules output 0–3V analog and can be powered at 3.3–5V depending on version

### Actuators

- **Dimmable LED grow light** (recommended) or **AC dimmer module**
  - For PWM-controlled DC LED: connect via MOSFET driver (IRLZ44N or similar logic-level N-channel)
  - For AC dimming: use an **AC dimmer module** (e.g., RobotDyn AC dimmer) with zero-cross detection
  - ESP32 GPIO26 provides PWM signal for intensity control (0–100%)

- **1-channel relay module (5V)** x1
  - For the **solenoid valve** / pump
  - Choose relay modules with optocoupler if possible

- **Solenoid valve 12V** ("van điện từ" / irrigation valve)
  - Typical: 12V DC plastic solenoid valve
  - Alternatively: a small DC pump instead of valve

### Power

- **ESP32 power:** 5V via USB (or a stable 5V regulator)
- **Valve power:** 12V DC power supply sized for valve current
- **LED light power:** depends on your LED driver (12V/24V DC typical for grow lights)
- **Relays:** typically need 5V; if your relay input is not 3.3V-compatible, use a transistor driver or a 3.3V-compatible relay board.

## Wiring (example)

This is a simple wiring recommendation; adjust pins in `include/Config.h`.

- DHT22 data -> GPIO15 (with a pull-up resistor 4.7k–10k to 3.3V if your module doesn't include it)
- PIR output -> GPIO27
- MQ-135 AO -> GPIO34 (ADC)  **(ensure max 3.3V to ESP32)**
- Soil moisture AO -> GPIO35 (ADC)
- Light PWM control -> GPIO26 (PWM output to MOSFET gate or AC dimmer control)
- Relay IN (Valve) -> GPIO25

### Light dimming circuit (DC LED)

For DC LED grow lights with PWM dimming:

```
ESP32 GPIO26 ──┬──[10kΩ]──> GND
               │
               └──> MOSFET Gate (IRLZ44N)
                    MOSFET Drain ──> LED- (negative)
                    MOSFET Source ──> GND
                    LED+ ──> +12V/24V power supply
```

### Light dimming circuit (AC with dimmer module)

For AC lights using a dimmer module (e.g., RobotDyn):
- PWM signal -> Dimmer module PWM input
- Zero-cross -> Optional GPIO for synchronization
- Follow the dimmer module's wiring guide for AC mains safety

**⚠️ WARNING: AC mains voltage is dangerous. If you're not experienced with AC wiring, use a pre-built AC dimmer module or stick to DC LED solutions.**

### Notes for MQ-135 + ESP32 ADC safety

Many MQ-135 boards run at 5V and may output up to 5V on AO.

- If your MQ-135 AO can reach 5V, use a **voltage divider** (e.g., 100k/200k) to scale to ≤3.3V.
- If you don't want analog scaling issues, you can power the MQ-135 heater at 5V but ensure the analog stage output is limited, or use an ADC module with proper range.

## Light control modes

The light supports **variable intensity** (PWM dimming), not just ON/OFF. Control is managed via ThingsBoard:

1. **Manual mode (mobile app):** User sets intensity (0–100%) via slider in the app
2. **Auto mode (edge logic):** ESP32 calculates intensity using formula:
   - `light_power = (1 / temperature_c) * constant`
   - Automatically turns off at scheduled times
3. **Schedule mode:** Light follows time-based rules configured via ThingsBoard

All control logic runs on the ESP32 to minimize message transfer. The formula and schedule can be updated remotely via ThingsBoard Shared Attributes or RPC.

## Calibration tips (quick)

- Soil sensor: read raw values for **dry air**, **dry soil**, **wet soil**, then set thresholds in `include/Config.h`.
- MQ-135: treat as **relative air quality** trend unless you implement a full calibration routine.
- Light auto-mode constant: tune `lightAutoConstant` based on your grow light and plant requirements.
