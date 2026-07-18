# Enclosure v1 — monolithic

Single-box dry-side enclosure for ESP32 + TMC2209 on solderless carriers.
Superseded for multi-pump work by [v2](../v2/); kept for simple prototypes and
CAD reference.

## Files

| File | Description |
|------|-------------|
| `enclosure_base.step` | Box with board zones, barrel jack, cable exits, USB, vents |
| `enclosure_lid.step` | Lid with lip, M3 holes, TMC vents |
| `enclosure_assembly.step` | Base + lid preview |
| `enclosure_monolithic.FCStd` | FreeCAD document |
| `generate_enclosure_v1.py` | Parametric generator |

## Material

**PETG** — required for print (see [parent README](../README.md#material)).

## Envelope

120 × 90 × ~45 mm (base 42 + lid 3). Wall / floor 2.5 mm.

## Regenerate

```bash
/Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd \
  cad/enclosure/v1/generate_enclosure_v1.py
```

Writes only into `cad/enclosure/v1/`.
