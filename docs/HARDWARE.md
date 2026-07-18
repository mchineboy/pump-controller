# Hardware

## Primary credit — peristaltic pump mechanical design

The printable peristaltic pump (housing, tube-pressure part, lid, and bearing
holder/rotor) is **not** an original design of this repository.

**Primary designer:** [Maximilian Puschmann](https://www.printables.com/@MaxPuschmann_1657997)  
**Model:** [V2 — Nema 17 Peristaltic Pump / Water Pump / Measuring Pump](https://www.printables.com/model/1556845-v2-nema-17-peristaltic-pump-water-pump-measuring-p)  
**Local copy of the author’s documentation:**
[hardware/maximilian-puschmann-nema17-peristaltic-pump-v2.pdf](hardware/maximilian-puschmann-nema17-peristaltic-pump-v2.pdf)

Download the STEP/STL files and follow print/assembly guidance from the
Printables page. Adjust the bearing-holder fit for your printer before pressing
on 608 bearings.

This project contributes the ESP32 controller firmware, electronics integration,
optional valve path, calibration/dispense software, and local web UI around that
mechanical design.

## Electronics overview

| Role | Part |
|------|------|
| Controller | ESP32 development board (Wi-Fi, USB programming) |
| Motor | STEPPERONLINE NEMA 17 bipolar stepper (1.5 A/phase) |
| Driver | BIGTREETECH TMC2209 V1.3 UART StepStick |
| Motor supply | 12 V regulated DC supply |
| Logic supply | 5 V buck converter feeding ESP32 5V/VIN |

Full part list: [BOM.md](BOM.md).

## Safety notes

- Verify the 5 V buck output with a multimeter before connecting the ESP32.
- Keep liquid plumbing physically separated from electronics.
- Do not drive a piezoelectric valve directly from an ESP32 GPIO; use a matched
  valve driver.
- Place an inline fuse near the 12 V power-entry point when building a permanent
  enclosure.

## Emergency stop (optional)

Default pin: **GPIO 32** (`PUMP_ESTOP_PIN`), active-low with ESP32 internal pull-up.

Recommended wiring:

- Use a normally-open (NO) momentary or maintained ESTOP that connects GPIO 32
  to GND when pressed/latched.
- Keep the switch on a short, dedicated cable; do not share with motor power
  returns in a way that injects noise onto the sense line.
- Enable “Emergency stop input” in the Diagnostics settings before relying on
  the switch. When enabled, ESTOP is polled in the firmware loop (not via HTTP).
- On assert: step pulses stop, valve closes, driver disables, system enters
  fault. Clear the switch, then acknowledge the fault in the UI before starting
  another operation.
