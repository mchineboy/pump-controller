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

### Phase 1–2
- Six generic fluid profiles (Fluid 1–6), renameable per use case
- Repeated calibration samples with averaging / CV warning (>3%)
- Calibration history per profile
- Configurable speed and acceleration
- Optional local basic auth (`admin` / `secrets.h`)
- Event logging
- Configuration export / import

### Phase 3 (complete)
- Optional valve pre-open / post-close, anti-drip reverse
- TMC2209 UART config + diagnostics
- Physical ESTOP
- Reservoir empty sensor (warn/block/fault)

### Phase 4 (in progress)
Tracked on GitHub milestone **Phase 4**, in design-doc order:
1. Load-cell verification (HX711) — done
2. Temperature monitoring — done
3. Flow sensor — done
4. Closed-loop dispensing — done
5. Multi-pump support — done (two paths, sequential motion; `pump_count` default 1)
6. Automated mixing jobs
7. Recipe engine (use-case agnostic)

Product branding is use-case agnostic: **Fluid Dispensing Controller**.

## Board

ELEGOO ESP32 Dev Board · ESP32-WROOM-32 · CP2102 · USB-C · PlatformIO `esp32dev`.
