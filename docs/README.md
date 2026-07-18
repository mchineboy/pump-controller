# Documentation

Index of project documentation for the ESP32 fluid dispensing controller.

## Contents

| Document | Description |
|----------|-------------|
| [DESIGN.md](DESIGN.md) | Software design summary: calibration/dispense rules, web stack, NVS storage, multi-pump model, safety, and phased feature status. |
| [HARDWARE.md](HARDWARE.md) | Mechanical pump credit, electronics overview, optional sensors, **multi-pump pin map** (1–3 paths), safety notes, enclosure CAD links. |
| [BOM.md](BOM.md) | Bill of materials for a complete build; notes how motor/driver/pump parts scale with pump count. |
| [WIRING.md](WIRING.md) | Default ESP32 / TMC2209 pin map for pump 1–3, power distribution, UART addressing, and pre-power safety checks. |
| [hardware/maximilian-puschmann-nema17-peristaltic-pump-v2.pdf](hardware/maximilian-puschmann-nema17-peristaltic-pump-v2.pdf) | Local copy of the upstream peristaltic pump author documentation (print/assembly). |

Dry-side enclosure STEP models live under [`cad/enclosure/`](../cad/enclosure/) (not under `docs/`). The v2 enclosure assumes multi-pump layouts; firmware supports up to **three** sequential paths (`pump_count` 1–3).

**Quick links:** [Multi-pump (HARDWARE)](HARDWARE.md#multi-pump--additional-fluid-paths-optional) · [Pin summary (WIRING)](WIRING.md#pin-summary) · [Flash survival (DESIGN)](DESIGN.md#durable-storage--flash-survival)
