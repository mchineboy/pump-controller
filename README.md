# Fluid Dispensing Controller

Open-source ESP32 firmware and local web UI for **precision fluid dispensing**
with a stepper-driven peristaltic pump. Suitable for lab dosing, hydroponics,
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
- STEP/DIR/ENABLE motor control (GPIO 26 / 27 / 25)
- Timed calibration with multi-sample averaging and history
- Configurable speed / acceleration (speed change clears calibration)
- Volume dispense via `steps_per_ml`
- Optional basic auth, event log, config export/import
- Safe boot: driver disabled, valve closed, no operation resume

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

## Default pins

See [docs/WIRING.md](docs/WIRING.md) for power distribution, TMC2209 connections,
and optional ESTOP / valve / UART / reservoir wiring.

| Signal | GPIO |
|--------|------|
| STEP   | 26 |
| DIR    | 27 |
| ENABLE | 25 (active low) |
| VALVE  | 33 (optional, disabled by default) |
| ESTOP  | 32 (optional, disabled by default) |
| TMC RX | 16 (optional UART) |
| TMC TX | 17 (optional UART) |
| RESERVOIR | 34 (optional empty sense) |

## Project layout

```text
src/           firmware
include/       headers
data/          LittleFS web assets
cad/           enclosure STEP / FreeCAD source
docs/          design notes, BOM, hardware credit, wiring
docs/hardware/ pump author documentation (PDF)
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for build setup, scope, and pull request
expectations.

## Notes

- Calibration and hardware settings live in **NVS** and survive `upload` /
  `uploadfs` / OTA. Clear them only via Diagnostics **Factory Reset** (or a full
  `erase_flash`).
- `pio run -t uploadfs` rewrites LittleFS (web UI). Event logs and on-device
  calibration *history files* on LittleFS are lost; profile calibrations in NVS
  are not.
- ENABLE polarity assumes a typical TMC2209 active-low enable.
- Valve and emergency-stop hardware are off by default until enabled in settings.
