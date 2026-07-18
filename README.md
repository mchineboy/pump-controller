# Fluid Dispensing Controller

Open-source ESP32 firmware and local web UI for **precision fluid dispensing**
with stepper-driven peristaltic pump(s). Suitable for lab dosing, hydroponics,
coolant metering, photography chemistry, laundry soap, and other applications
where calibrated volume delivery matters.

**Board:** ELEGOO ESP32 Dev Board (ESP32-WROOM-32, CP2102, USB-C)  
**Framework:** PlatformIO + Arduino  
**UI:** HTML / CSS / vanilla JS served from LittleFS  
**License:** [MIT](LICENSE)

## Hardware credit

The 3D-printable peristaltic pump mechanical design is by
**[Maximilian Puschmann](https://www.printables.com/@MaxPuschmann_1657997)**:

- [V2 — Nema 17 Peristaltic Pump on Printables](https://www.printables.com/model/1556845-v2-nema-17-peristaltic-pump-water-pump-measuring-p)
- Local author documentation: [docs/hardware/maximilian-puschmann-nema17-peristaltic-pump-v2.pdf](docs/hardware/maximilian-puschmann-nema17-peristaltic-pump-v2.pdf)

This repository provides the controller firmware, electronics integration, and
web UI around that design. See [docs/HARDWARE.md](docs/HARDWARE.md),
[docs/BOM.md](docs/BOM.md), [docs/WIRING.md](docs/WIRING.md), and
[cad/enclosure/](cad/enclosure/) for dry-side enclosure STEP files
([v1 monolithic](cad/enclosure/v1/), [v2 sectional](cad/enclosure/v2/)).

## Features

- DHCP Wi-Fi (credentials in ignored `include/secrets.h`) with soft-AP fallback
- Local web UI: dispense, calibration, profiles, diagnostics, hardware
- Six generic fluid profiles (Fluid 1–6), renameable, with independent calibration
- **Multi-pump:** 1–3 fluid paths (`pump_count` in Diagnostics; default **1**)
  - Each profile binds to `pump_1`, `pump_2`, or `pump_3`
  - Sequential motion only (one path at a time); ESTOP stops all paths
  - Shared TMC2209 UART bus (addresses `0b00` / `0b01` / `0b10`)
- STEP/DIR/ENABLE motor control per path (see pin tables below)
- Timed calibration with multi-sample averaging and history
- Configurable speed / acceleration (speed change clears calibration)
- Volume dispense via `steps_per_ml`
- Optional sensors: load cell, temperature, pulse flow; optional closed-loop feedback
- Optional basic auth, event log, config export/import, Factory Reset
- Safe boot: drivers disabled, valves closed, no operation resume
- Durable config in **NVS** (survives firmware and LittleFS uploads)

## Quick start

1. Copy `include/secrets.example.h` to `include/secrets.h` and set Wi-Fi credentials.
2. Connect the board with a data-capable USB-C cable.
3. Build and upload:

```bash
pio run -t upload
pio run -t uploadfs
```

4. Open the DHCP address printed on serial (or fallback AP `PumpController` /
   `pumpsetup` at http://192.168.4.1).

Optional web auth uses `Secrets::kWebUsername` / `kWebPassword` when enabled on
the Diagnostics page.

### Enabling a second or third pump

1. Wire the extra TMC2209 + NEMA 17 using the pump 2 / pump 3 pins below.
2. Strap UART addresses (`0b01`, `0b10`) on the shared RX/TX lines.
3. In **Diagnostics → Multi-pump**, set **Pump count** to 2 or 3.
4. On **Profiles**, bind each fluid to the correct pump path, then calibrate.

Full detail: [docs/HARDWARE.md](docs/HARDWARE.md#multi-pump--additional-fluid-paths-optional)
and [docs/WIRING.md](docs/WIRING.md).

## Default pins

See [docs/WIRING.md](docs/WIRING.md) for power distribution, TMC2209 connections,
and optional ESTOP / valve / UART / reservoir / sensor wiring.

### Pump 1 (always present)

| Signal | GPIO |
|--------|------|
| STEP   | 26 |
| DIR    | 27 |
| ENABLE | 25 (active low) |
| VALVE  | 33 (optional, disabled by default) |

### Pump 2 (optional, `pump_count` ≥ 2)

| Signal | GPIO |
|--------|------|
| STEP   | 5 |
| DIR    | 13 |
| ENABLE | 14 (active low) |
| VALVE  | 21 (optional) |
| TMC UART address | `0b01` |

### Pump 3 (optional, `pump_count` ≥ 3)

| Signal | GPIO |
|--------|------|
| STEP   | 22 |
| DIR    | 15 (strapping — keep quiet at boot) |
| ENABLE | 2 (active low) |
| VALVE  | 12 (optional; strapping) |
| TMC UART address | `0b10` |

### Shared / optional

| Signal | GPIO |
|--------|------|
| ESTOP  | 32 (optional, disabled by default) |
| TMC RX | 16 (optional UART, shared by all drivers) |
| TMC TX | 17 (optional UART, shared by all drivers) |
| RESERVOIR | 34 (optional empty sense) |
| HX711 DT / SCK | 19 / 18 (optional load cell) |
| TEMP | 23 (optional DS18B20) |
| FLOW | 4 (optional pulse) |

## Project layout

```text
src/           firmware
include/       headers
data/          LittleFS web assets
cad/           enclosure STEP / FreeCAD source
docs/          design notes, BOM, hardware credit, wiring
docs/hardware/ pump author documentation (PDF)
.cursor/rules/ agent workflow and documentation rules
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for build setup, scope, and pull request
expectations. Product changes that affect pins, features, storage, or APIs must
update the matching docs in the same PR.

## Notes

- Calibration and hardware settings live in **NVS** and survive `upload` /
  `uploadfs` / OTA. Clear them only via Diagnostics **Factory Reset** (or a full
  `erase_flash`).
- `pio run -t uploadfs` rewrites LittleFS (web UI). Event logs and on-device
  calibration *history files* on LittleFS are lost; profile calibrations in NVS
  are not.
- ENABLE polarity assumes a typical TMC2209 active-low enable.
- Valve, ESTOP, sensors, and multi-pump paths are off/inactive until enabled in
  Diagnostics (`pump_count` defaults to 1).
- Size the 12 V supply for **one** NEMA 17 at a time unless you validate
  concurrent motor load (firmware does not run paths concurrently).
