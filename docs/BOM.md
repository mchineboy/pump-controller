# Bill of Materials

Parts for a complete precision fluid dispensing build using Maximilian
Puschmann’s NEMA 17 peristaltic pump and this ESP32 controller.

| Category | Item | Description / Specification | Qty | Required | Notes |
|----------|------|-----------------------------|-----|----------|-------|
| Controller | ESP32 development board | ESP32 with Wi-Fi, USB programming, 5V/VIN input, and sufficient GPIO | 1 | Yes | Exact board model not yet specified; scaffold targets ELEGOO ESP32 Dev Board (ESP32-WROOM-32, CP2102, USB-C) |
| Motor | Stepper motor | STEPPERONLINE NEMA 17 bipolar stepper, 1.5 A/phase, 42 N·cm, 42×42×38 mm, 1.8°, 4-wire, 1 m cable | 1 | Yes | Directly drives the peristaltic pump rotor |
| Motor Driver | Stepper driver | BIGTREETECH TMC2209 V1.3 UART StepStick, 2.8 A peak | 1 | Yes | Two-pack may be purchased; one module is required and one can be retained as a spare |
| Power | 12V power adapter | ALITOVE regulated 12 V DC, 3 A, 36 W, 100–240 V AC input, center-positive 5.5×2.1/2.5 mm plug | 1 | Yes | Powers the stepper driver and feeds the 5 V buck converter |
| Power | 5V buck converter | Fixed-output DC-DC step-down module, 5–30 V input to regulated 5 V output, rated up to 3 A | 1 | Yes | Feeds the ESP32 5V/VIN input; verify output with a multimeter before connection |
| Power | Barrel jack adapter | Female 5.5×2.1/2.5 mm barrel-jack to screw-terminal adapter | 1 | Recommended | Convenient connection from the power adapter to project wiring |
| Power | Motor supply capacitor | 220–470 µF, 25 V electrolytic capacitor | 1 | Recommended | Install close to TMC2209 VMOT and GND |
| Pump | 608 bearings | Standard 608 bearing, nominal 8 mm bore × 22 mm OD × 7 mm width | 5 | Yes | Press onto the printed bearing-holder posts after adjusting the STEP model for fit |
| Pump | Tubing | Soft 8 mm OD tubing; silicone recommended by the pump design | As needed | Yes | Use tubing chemically compatible with the target fluid; dedicate a line per fluid when cross-contamination matters |
| Pump | Motor mounting screws | M3 × 16 mm screws | 4 | Yes | Mount pump housing to NEMA 17 motor |
| Pump | Housing assembly screws | M3 screws, 15–30 mm length | 4 | Yes | Final length depends on the printed housing and lid |
| Pump | Printed pump components | Main housing, tube-pressure part, lid, and bearing holder/rotor from [Puschmann’s design](HARDWARE.md) | 1 set | Yes | PETG recommended for the functional pump parts |
| Valve | Piezoelectric valve | Normally closed, chemically compatible fluid valve sized for the selected tubing and required flow | 1 | Optional | Sharper cutoff, reduced dripping, improved small-volume control |
| Valve | Valve driver | Driver/interface matched to the selected piezoelectric valve voltage and waveform requirements | 1 | Optional | Do not drive a piezoelectric valve directly from an ESP32 GPIO |
| Wiring | Hookup wire and connectors | Wire, ferrules or crimp terminals, and connectors suitable for 12 V power, motor phases, and low-voltage control | 1 set | Yes | Keep motor wiring separated from ESP32 signal wiring where practical |
| Thermal | TMC2209 heatsink | Small adhesive heatsink appropriate for the StepStick module | 1 | Recommended | Often included with the BIGTREETECH driver package |
| Safety | Fuse | Inline fuse and holder sized for the 12 V input, approximately 3 A | 1 | Recommended | Place near the 12 V power-entry point |
| Safety | Emergency stop switch | Normally-open switch or latching ESTOP that connects GPIO 32 to GND when asserted | 1 | Optional | Enable in Diagnostics; see [HARDWARE.md](HARDWARE.md) |
| Sensing | Load cell | Strain-gauge load cell, typically 1–5 kg capacity, matched mount for vessel | 1 | Optional | Phase 4 verification; pair with HX711 |
| Sensing | HX711 amplifier | HX711 load-cell ADC module, ESP32-compatible logic levels | 1 | Optional | DT GPIO 19, SCK GPIO 18 by default |
| Enclosure | Controller enclosure | Ventilated enclosure for ESP32, driver, buck converter, terminals, and optional valve driver | 1 | Recommended | Keep liquid plumbing physically separated from electronics |
