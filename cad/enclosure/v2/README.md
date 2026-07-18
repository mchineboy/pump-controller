# Enclosure v2 — sectional

Modular dry-side box. Each printable part is ≤ **200 mm** in its longest
dimension. Larger pump counts are built by joining sections.

## Modules

| Part | Role |
|------|------|
| `controller_base/lid` | Half-size breadboard bay, buck bay, barrel jack, USB slot, optional 40 mm fan on lid |
| `driver_x1_base/lid` | One generic TMC2209 + breakout bay |
| `driver_x2_base/lid` | Two generic driver bays |
| `endcap_base/lid` | Three Ø10 mm cable / grommet exits toward the wet side |

Mating: +X tongue into the next module’s −X groove, plus M3 clamp holes at the
joint walls.

## Assembly matrix (N pumps)

Prefer `driver_x2`, remainder `driver_x1`, then always finish with `endcap`:

| N | Sections after controller |
|---|---------------------------|
| 1 | x1 + endcap |
| 2 | x2 + endcap |
| 3 | x2 + x1 + endcap |
| 4 | x2 + x2 + endcap |
| 5 | x2 + x2 + x1 + endcap |
| 6 | x2 + x2 + x2 + endcap |

Preview assemblies: `assembly_1pump.step` … `assembly_6pump.step`.

## Design intent

- **Breadboard:** standard half-size 400-point, nominal **82 × 55 × 10 mm**.
- **ESP32** and logic wiring sit on the breadboard.
- **TMC2209** motor VM / coil current uses rated breakout boards in driver bays.
- **Buck converter:** generic ~40 × 20 × 10 mm bay near the barrel jack.
- Concurrent pumps: space for 1–6 drivers; design assumption **up to 3 concurrent**.
- Corner bosses: **M3 heat-set insert** pilots (Ø4.0 mm).
- Controller lid: optional **40 mm fan** (32 mm screw pitch).

## Envelope (per module)

Widths and heights are shared: **95 × ~53 mm** (base 50 + lid 3). Lengths are
printed by the generator at run time.

## Files

| File | Description |
|------|-------------|
| `generate_enclosure_v2.py` | Parametric generator |
| `enclosure_sectional.FCStd` | FreeCAD document |
| `controller_*`, `driver_*`, `endcap_*` | Module STEP exports |
| `assembly_*pump.step` | Preview assemblies |

## Regenerate

```bash
/Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd \
  cad/enclosure/v2/generate_enclosure_v2.py
```

Writes only into `cad/enclosure/v2/` — does not modify [v1](../v1/) exports.

## Material

**PETG** — required for print (see [parent README](../README.md#material)).

## Print / assembly notes

- Install M3 heat-set inserts in base bosses; M3×8–12 mm lid screws.

- Confirm barrel-jack panel diameter (`BARREL_D`).
- Fit rubber grommets in endcap cable exits.
- Revise `DRIVER_BAY_*` once the exact breakout board is chosen.
