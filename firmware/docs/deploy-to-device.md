# Deploy to a real ESP32 (PlatformIO + VS Code)

You already have PlatformIO + the VS Code extension installed.

## 1) Prepare hardware

1. Plug the ESP32 DevKit into your PC via USB
2. Ensure drivers are installed (CP2102 / CH340 depending on your board)

## 2) Configure secrets

Choose ONE option:

- Option A (recommended): copy `.env.example` → `.env` and fill values (gitignored). PlatformIO will auto-generate `include/Secrets.h` during build.
- Option B: copy `include/Secrets.h.example` → `include/Secrets.h` and fill values (gitignored).

## 3) Configure pins (if needed)

Open `include/Config.h` and adjust pin numbers to match your wiring.

## 4) Build

From VS Code (PlatformIO toolbar) or terminal:

```bash
pio run
```

## 5) Upload firmware

```bash
pio run -t upload
```

If upload fails, check:

- Correct COM port (PlatformIO usually auto-detects)
- USB cable supports data (some cables are charge-only)
- Hold `BOOT` button during upload on some boards

## 6) Monitor logs

```bash
pio device monitor
```

You should see:

- Wi-Fi connecting and IP address
- ThingsBoard MQTT connection status
- Periodic telemetry publishes

## 7) Configure behavior per-device (optional)

After the device is online in ThingsBoard, you can override default thresholds/intervals by setting **Shared Attributes** on the device.

See `docs/thingsboard-cloud-setup.md` → “Remote config (Shared Attributes)”.
