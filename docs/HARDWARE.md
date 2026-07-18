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
Wiring diagrams and connection tables: [WIRING.md](WIRING.md).  
Dry-side enclosure STEP models: [../cad/enclosure/](../cad/enclosure/).

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

## TMC2209 UART (optional)

Default pins: **RX GPIO 16**, **TX GPIO 17** (`PUMP_TMC_RX_PIN` / `PUMP_TMC_TX_PIN`).
Baud: 115200. Driver address: `0b00` (MS1/MS2 low on BIGTREETECH V1.3).

Wiring notes:

- Connect ESP32 TX → TMC PDN_UART (through the board’s UART path; do not leave
  PDN_UART floating if UART mode is intended).
- Connect ESP32 RX ← TMC UART TX / PDN_UART sense path per the StepStick silkscreen.
- Keep STEP/DIR/ENABLE wired regardless; UART only configures current,
  microsteps, and StealthChop. Motion remains STEP/DIR.
- Sense resistor assumed **0.11 Ω** (standard on BTT TMC2209 V1.3).
- Enable “TMC2209 UART” in Diagnostics and set run/hold current and microsteps.
  On miswire or no response, boot continues in STEP/DIR-only mode and logs
  `tmc_uart_warning` / `tmc_uart_no_response`.

When UART is ready, firmware polls `DRV_STATUS` about every 100 ms. Overtemperature
or short-circuit flags raise `motor_driver_fault`, stop motion, and close the valve.
Open-load is only treated as a fault while coils are driven. Acknowledge after the
condition clears (Diagnostics includes a simulate-fault control for testing without
hardware).

## Reservoir level sensor (optional)

Default pin: **GPIO 34** (`PUMP_RESERVOIR_PIN`). This pin is input-only and has no
internal pull-up; use an external pull-up (e.g. 10 kΩ to 3.3 V) with a
normally-open float or optical empty switch to GND for active-low empty.

Recommended wiring:

- Float switch or optical liquid sensor that asserts empty when the reservoir is
  low/dry.
- Default polarity: empty = LOW (enable “Empty signal is active-low” in
  Diagnostics). Invert if your sensor is active-high.
- Policy options: **warn** (status only), **block** (refuse new dispense/calibrate),
  **fault** (also stop an in-progress operation with `reservoir_empty`).
- With the sensor disabled, dispense behavior is unchanged.
