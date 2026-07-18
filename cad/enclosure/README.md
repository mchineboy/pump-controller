# Dry-side electronics enclosure

Two designs share this folder. Prefer **v2** for new builds.

| Version | Layout | When to use |
|---------|--------|-------------|
| **[v1](v1/)** — monolithic | Single 120×90 mm box + lid | Simple 1-driver prototypes |
| **[v2](v2/)** — sectional | Controller + driver modules + endcap | 1–6 pumps; each printable part ≤ 200 mm |

Generators write **only** into their own directory and do not delete the other
version’s exports.

## Material

**Print in PETG** (both v1 and v2).

| Material | Verdict |
|----------|---------|
| **PETG** | Required / recommended — heat near TMC2209 and buck, M3 heat-set inserts, toughness for sectional joints |
| PLA | Avoid — softens near ~60 °C; poor near drivers and for inserts |
| ABS / ASA | Acceptable if you already print them well; more warp/fume hassle than PETG for this box |

Same filament family as the functional pump parts ([BOM](../../docs/BOM.md)).

## Regenerate

```bash
# v1 monolithic
/Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd \
  cad/enclosure/v1/generate_enclosure_v1.py

# v2 sectional
/Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd \
  cad/enclosure/v2/generate_enclosure_v2.py
```

Pumps do not mount to these enclosures. Keep liquid plumbing physically
separated from the dry-side box. See [docs/HARDWARE.md](../../docs/HARDWARE.md)
and [docs/WIRING.md](../../docs/WIRING.md).
