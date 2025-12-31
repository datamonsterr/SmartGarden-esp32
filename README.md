# Smart Garden Edge Devices (ESP32 + PlatformIO)

This repository contains an ESP32 (Arduino framework) edge-device project for a simple smart garden:

- Temperature + humidity (DHT22)
- Motion detection (HC-SR501 PIR)
- Air quality (MQ-135 analog)
- Soil moisture (capacitive analog)
- Light bulb control (relay)
- Watering control using a solenoid valve (relay)
- Telemetry sent to ThingsBoard Cloud via MQTT

Documentation:

- Hardware: [docs/hardware.md](docs/hardware.md)
- ThingsBoard Cloud setup: [docs/thingsboard-cloud-setup.md](docs/thingsboard-cloud-setup.md)
- Deploy to a real device: [docs/deploy-to-device.md](docs/deploy-to-device.md)
- Use-cases (expected behavior): [docs/use-cases.md](docs/use-cases.md)

## Quick start

1. Configure secrets (choose ONE option):
   - Option A (recommended): copy `.env.example` to `.env` and fill values (gitignored). PlatformIO will auto-generate `include/Secrets.h` during build.
   - Option B: copy `include/Secrets.h.example` to `include/Secrets.h` and fill values (gitignored).
2. Build and upload:
   - `pio run -t upload`
3. Monitor serial:
   - `pio device monitor`

## Wokwi simulation

This repo includes a Wokwi wiring diagram and configuration:

- `diagram.json`
- `wokwi.toml`

Steps:

1. Build the firmware: `pio run`
2. Start Wokwi using the VS Code Wokwi extension (it will load the `.pio` firmware via `wokwi.toml`).


## [ThingsBoard PE Mobile Application](https://thingsboard.io/products/mobile-pe/) is an open-source project based on [Flutter](https://flutter.dev/)
Powered by [ThingsBoard PE](https://thingsboard.io/products/thingsboard-pe/) IoT Platform

Build your own advanced IoT mobile application **with minimum coding efforts**

## Please be informed the Web platform is not supported, because it's a part of our main platform!

## Resources

- [Getting started](https://thingsboard.io/docs/pe/mobile/getting-started/) - learn how to set up and run your first IoT mobile app
- [Customize your app](https://thingsboard.io/docs/pe/mobile/customization/) - learn how to customize the app
- [Publish your app](https://thingsboard.io/docs/pe/mobile/release/) - learn how to publish app to Google Play or App Store
