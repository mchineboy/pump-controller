# Software Design Summary

Source: project software design document (2026-07-18).

## Core decisions

1. **Calibration:** timed dispense (default 1000 ms) → user enters measured mL → store `steps_per_ml`.
2. **Dispense:** `target_steps = volume_ml * steps_per_ml` (step-count based, not wall-clock only).
3. **Speed rule:** calibration speed must equal dispense speed.
4. **Web client:** HTML + CSS + vanilla JS + ES modules + `fetch` + SSE. No React/Vue/etc.
5. **Storage:** Preferences/NVS for settings; LittleFS for profiles, history, logs, and web assets.
6. **Safety:** safe boot, no auto-resume after power loss, software stop, optional ESTOP later.

## Implemented

- Six generic fluid profiles (Fluid 1–6), renameable per use case
- Repeated calibration samples with averaging / CV warning (>3%)
- Calibration history per profile
- Configurable speed and acceleration
- Optional local basic auth (`admin` / `secrets.h`)
- Event logging
- Configuration export / import

Product branding is use-case agnostic: **Fluid Dispensing Controller**.

## Board

ELEGOO ESP32 Dev Board · ESP32-WROOM-32 · CP2102 · USB-C · PlatformIO `esp32dev`.
