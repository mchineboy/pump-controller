# Documentation

Index of project documentation for the ESP32 fluid dispensing controller.

## Contents

| Document | Description |
|----------|-------------|
| [DESIGN.md](DESIGN.md) | Software design summary: calibration/dispense rules, web stack choices, storage, safety, and phased feature status. |
| [HARDWARE.md](HARDWARE.md) | Mechanical pump credit, electronics overview, safety notes, GPIO roles, and links to enclosure CAD. |
| [BOM.md](BOM.md) | Bill of materials for a complete build (controller, motor, driver, power, pump parts, optional valve and sensors). |
| [WIRING.md](WIRING.md) | Default ESP32 / TMC2209 pin map, power distribution, connection tables, and pre-power safety checks. |
| [hardware/maximilian-puschmann-nema17-peristaltic-pump-v2.pdf](hardware/maximilian-puschmann-nema17-peristaltic-pump-v2.pdf) | Local copy of the upstream peristaltic pump author documentation (print/assembly). |

Dry-side enclosure STEP models live under [`cad/enclosure/`](../cad/enclosure/) (not under `docs/`).
