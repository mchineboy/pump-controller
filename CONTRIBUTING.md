# Contributing

Contributions that improve calibrated dispensing, safety, hardware portability,
or documentation are welcome. This document covers how to build, what is in
scope, and how to submit changes.

## Before you start

1. Open an [issue](https://github.com/mchineboy/pump-controller/issues) for
   non-trivial work so direction can be agreed before code lands.
2. Search existing issues and PRs for duplicates.
3. Read [docs/DESIGN.md](docs/DESIGN.md) and [docs/HARDWARE.md](docs/HARDWARE.md).

Small fixes (typos, obvious bugs, pin-map typos) do not need an issue first.

## Scope

**In scope**

- Firmware (`src/`, `include/`)
- Local web UI assets (`data/`)
- Docs, BOM accuracy, wiring notes
- Portability (other ESP32 boards, pin maps, driver variants)
- Safety, calibration accuracy, diagnostics

**Out of scope / special rules**

- **Pump meshes:** The printable peristaltic pump is
  [Maximilian Puschmann’s design](https://www.printables.com/model/1556845-v2-nema-17-peristaltic-pump-water-pump-measuring-p).
  Do not submit STL/STEP redesigns here as if they were original to this repo.
  Suggest pump-geometry changes on Printables; we can link useful remakes from
  `docs/HARDWARE.md`.
- **Secrets:** Never commit `include/secrets.h` or real Wi-Fi/password values.
  Only update `include/secrets.example.h` with placeholders.
- **Dependency churn:** Prefer pinned, known-good PlatformIO packages unless
  there is a clear bugfix or security reason.

## Development setup

### Requirements

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html)
  or the PlatformIO IDE extension
- ESP32 board with USB serial (scaffold targets ELEGOO ESP32 Dev Board /
  `esp32dev`)
- CP2102 (or board-appropriate) USB drivers
- Data-capable USB cable

### Secrets

```bash
cp include/secrets.example.h include/secrets.h
```

Edit `include/secrets.h` with your Wi-Fi SSID/password. That file is gitignored.

### Serial ports

`platformio.ini` ships with example macOS ports:

```ini
upload_port = /dev/cu.usbserial-0001
monitor_port = /dev/cu.usbserial-0001
```

Override for your machine without committing local paths, for example:

```bash
pio run -t upload --upload-port /dev/cu.usbserial-XXXX
pio device monitor --port /dev/cu.usbserial-XXXX
```

Or set `upload_port` / `monitor_port` locally and leave them out of PRs.

### Build, flash, filesystem

```bash
pio run                 # compile only
pio run -t upload       # flash firmware
pio run -t uploadfs     # upload data/ to LittleFS
pio device monitor      # serial log (115200)
```

**Warning:** `uploadfs` rewrites the LittleFS image and clears profiles,
calibration history, and logs stored on the device.

After flashing, use the DHCP address from serial, or join the fallback AP
`PumpController` / `pumpsetup` and open http://192.168.4.1.

### Web UI changes

UI files live under `data/` (HTML/CSS/vanilla JS modules). After editing:

```bash
pio run -t uploadfs
```

No bundler or npm build step.

## Project layout

| Path | Role |
|------|------|
| `src/` | Firmware implementation |
| `include/` | Headers / config |
| `data/` | LittleFS web assets |
| `docs/` | Design, BOM, hardware credit |
| `platformio.ini` | Board, pins, libraries |

Default GPIO map is documented in the [README](README.md). Override pins via
`build_flags` in `platformio.ini` (`PUMP_STEP_PIN`, etc.).

## Coding guidelines

- Match the style of surrounding files (Arduino/C++ with headers under
  `include/`, implementation under `src/`).
- Prefer small, focused PRs over large mixed changes.
- Keep the web client vanilla JS (ES modules + `fetch` + SSE). Do not add
  React/Vue/etc. without prior agreement.
- Preserve safe-boot behavior: driver disabled, valve closed, no auto-resume
  of a dispense after power loss.
- Calibration and dispense speed must stay coupled unless the design doc and
  UI are updated together.
- When changing API JSON shapes, update both firmware handlers and `data/js/`.

## Testing expectations

There is no automated on-device test suite yet. For behavioral changes, state
in the PR what you verified, for example:

- [ ] Firmware builds (`pio run`)
- [ ] Uploads and boots; serial shows Wi-Fi or AP fallback
- [ ] Dispense of a known volume and measured result (if motor hardware available)
- [ ] Calibration sample flow still stores `steps_per_ml`
- [ ] Soft stop still halts motion
- [ ] UI pages load after `uploadfs`

If you lack pump hardware, note that and limit claims to compile/UI checks.

## Pull requests

1. Fork and branch from `main`.
2. Keep commits readable; squash locally if the history is noisy.
3. Describe **why** the change exists, not only what files moved.
4. Link related issues (`Fixes #123`).
5. Call out breaking changes (pin defaults, LittleFS schema, API fields).
6. Do not include `secrets.h`, local `upload_port` values, or unrelated
   reformatting.

Maintainers may ask for narrower PRs if firmware, UI, and docs are tangled.

## Reporting bugs

Include:

- Board / driver (e.g. ESP32 + TMC2209)
- PlatformIO / platform versions if relevant
- Steps to reproduce
- Expected vs actual behavior
- Serial excerpt (redact Wi-Fi credentials)
- Whether the issue is firmware, UI, or hardware wiring

## Security / safety

This project moves fluid and drives motors. Treat PRs that touch stop paths,
enable polarity, ESTOP, or valve timing as safety-sensitive: explain the
failure mode you are fixing and how you tested it.

If you discover a vulnerability in the web auth or network surface, open a
private security advisory on GitHub or contact the maintainer rather than
filing a public issue with an exploit.

## License

By contributing, you agree that your contributions are licensed under the
repository’s [MIT License](LICENSE).
