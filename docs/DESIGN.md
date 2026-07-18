# Software Design Summary

Source: project software design document (2026-07-18).

## Core decisions

1. **Calibration:** timed dispense (default 1000 ms) → user enters measured mL → store `steps_per_ml`.
2. **Dispense:** `target_steps = volume_ml * steps_per_ml` (step-count based, not wall-clock only).
3. **Speed rule:** calibration speed must equal dispense speed.
4. **Web client:** HTML + CSS + vanilla JS + ES modules + `fetch` + SSE. No React/Vue/etc.
5. **Storage:** Preferences/NVS for settings **and** fluid profiles/calibration;
   LittleFS for web assets, optional calibration history, and event logs.
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
5. Multi-pump support — done (up to three paths, sequential motion; `pump_count` default 1)
6. Automated mixing jobs
7. Recipe engine (use-case agnostic)

Product branding is use-case agnostic: **Fluid Dispensing Controller**.

## Durable storage / flash survival

| Data | Location | Survives `upload` | Survives `uploadfs` | Survives OTA app | Cleared by |
|------|----------|-------------------|---------------------|------------------|------------|
| Hardware settings | NVS `pump_settings` | Yes | Yes | Yes | Factory Reset / `erase_flash` |
| Profiles + calibration | NVS `pump_prof` | Yes | Yes | Yes | Factory Reset / `erase_flash` |
| Web UI assets | LittleFS | Yes | Replaced | Yes | `uploadfs` |
| Event log / cal history | LittleFS | Yes | Lost | Yes | `uploadfs` / Factory Reset |

Firmware never erases NVS on boot or upgrade. Explicit **Factory Reset**
(`POST /api/factory-reset` with `{"confirm":"FACTORY_RESET"}`) clears NVS
namespaces and reseeds defaults, then reboots.

## Board

ELEGOO ESP32 Dev Board · ESP32-WROOM-32 · CP2102 · USB-C · PlatformIO `esp32dev`.
