# Hardware (Vietnam-friendly BOM)

This project uses common, low-cost parts that are widely available in Vietnam (e.g., Shopee/Lazada, local electronics shops like Nhật Tảo, online stores such as linh kiện điện tử / arduino shops).

## Recommended parts

### Core

- **ESP32 DevKit V1** (ESP32-WROOM-32)
  - Easy to find, USB programming, lots of GPIO

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

- **1-channel relay module (5V)** x2
  - One for the **light bulb** (AC mains) or a DC lamp
  - One for the **solenoid valve** / pump
  - Choose relay modules with optocoupler if possible

- **Solenoid valve 12V** ("van điện từ" / irrigation valve)
  - Typical: 12V DC plastic solenoid valve
  - Alternatively: a small DC pump instead of valve

### Power

- **ESP32 power:** 5V via USB (or a stable 5V regulator)
- **Valve power:** 12V DC power supply sized for valve current
- **Relays:** typically need 5V; if your relay input is not 3.3V-compatible, use a transistor driver or a 3.3V-compatible relay board.

## Wiring (example)

This is a simple wiring recommendation; adjust pins in `include/Config.h`.

- DHT22 data -> GPIO4 (with a pull-up resistor 4.7k–10k to 3.3V if your module doesn’t include it)
- PIR output -> GPIO27
- MQ-135 AO -> GPIO34 (ADC)  **(ensure max 3.3V to ESP32)**
- Soil moisture AO -> GPIO35 (ADC)
- Relay IN (Light) -> GPIO26
- Relay IN (Valve) -> GPIO25
- Manual light override button -> GPIO14 (button to GND, uses internal pull-up)

### Notes for MQ-135 + ESP32 ADC safety

Many MQ-135 boards run at 5V and may output up to 5V on AO.

- If your MQ-135 AO can reach 5V, use a **voltage divider** (e.g., 100k/200k) to scale to ≤3.3V.
- If you don’t want analog scaling issues, you can power the MQ-135 heater at 5V but ensure the analog stage output is limited, or use an ADC module with proper range.

## Calibration tips (quick)

- Soil sensor: read raw values for **dry air**, **dry soil**, **wet soil**, then set thresholds in `include/Config.h`.
- MQ-135: treat as **relative air quality** trend unless you implement a full calibration routine.
