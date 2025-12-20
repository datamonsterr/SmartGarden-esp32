# Copilot instructions (SmartGarden edge devices)

- Target: ESP32 (Arduino framework) built with PlatformIO.
- Keep code modular: one responsibility per file/class (sensors/, actuators/, controllers/, net/, thingsboard/).
- Avoid blocking delays in `loop()`; schedule work with `millis()`.
- Never hardcode secrets:
	- Preferred: use `.env` (gitignored) and generate `include/Secrets.h` via the PlatformIO pre-build script.
	- Fallback: `include/Secrets.h` (gitignored) copied from `include/Secrets.h.example`.
	- If adding new secret keys, update `.env.example` and `include/Secrets.h.example`.
- ThingsBoard:
	- Telemetry: publish compact JSON to topic `v1/devices/me/telemetry`.
	- Control: accept RPC on `v1/devices/me/rpc/request/+` (e.g., setLight / temp limit / manual off).
	- Runtime config: fetch per-device settings from Shared Attributes and persist locally (NVS/Preferences). `include/Config.h` remains the safe fallback defaults.
- Prefer clear names and small functions; no one-letter variables.
- When adding new user-configurable behavior, prefer server-fetched Shared Attributes (runtime config) instead of adding more compile-time constants.
- When adding hardware support, document wiring + pin changes in docs and keep pins compile-time (avoid remote pin remapping).
