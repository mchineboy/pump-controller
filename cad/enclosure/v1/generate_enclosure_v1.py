#!/usr/bin/env freecadcmd
"""
v1 — Monolithic dry-side electronics enclosure (single box).

Material: PETG (see cad/enclosure/README.md).

Simple one-piece base + lid. Prefer v2 (sectional) for multi-pump builds
and parts that must stay ≤ 200 mm.

Exports (this directory only — does not touch v2/):
  enclosure_base.step
  enclosure_lid.step
  enclosure_assembly.step
  enclosure_monolithic.FCStd

Run:
  /Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd \\
    cad/enclosure/v1/generate_enclosure_v1.py
"""

from __future__ import annotations

import os
import sys

import FreeCAD as App
import Part
from FreeCAD import Vector

# ---------------------------------------------------------------------------
# Design parameters (mm). Tune here and re-run to regenerate STEP.
# ---------------------------------------------------------------------------

OUTER_L = 120.0  # X — barrel jack on -X, cables on +X
OUTER_W = 90.0
OUTER_H = 42.0
WALL = 2.5
FLOOR = 2.5
LID_H = 3.0
LID_LIP = 1.5
CLEARANCE = 0.3

BOSS_OD = 8.0
BOSS_ID = 2.8
BOSS_H = OUTER_H - FLOOR - 1.0
SCREW_HOLE_LID = 3.2
COUNTERSINK_D = 6.0
COUNTERSINK_H = 1.8

# ESP32 50x28 on solderless carrier — reserve ~70x48
ESP_ZONE_L = 70.0
ESP_ZONE_W = 48.0
ESP_ZONE_X = 12.0
ESP_ZONE_Y = 10.0

# TMC2209 20x15 + heatsink ~18 tall — ~40x30 zone
TMC_ZONE_L = 40.0
TMC_ZONE_W = 30.0
TMC_ZONE_X = 12.0
TMC_ZONE_Y = 10.0 + ESP_ZONE_W + 8.0

RIDGE_W = 1.5
RIDGE_H = 1.2

BARREL_D = 11.0
BARREL_Z = FLOOR + 14.0
BARREL_Y = OUTER_W / 2.0

CABLE_D = 10.0
CABLE_Z = FLOOR + 12.0
CABLE_Y_SPACING = 22.0

USB_SLOT_W = 14.0
USB_SLOT_H = 8.0
USB_SLOT_Z = FLOOR + 10.0
USB_SLOT_X = ESP_ZONE_X + WALL + ESP_ZONE_L / 2.0

VENT_COUNT = 5
VENT_W = 2.5
VENT_H = 12.0
VENT_Z = FLOOR + 16.0


def _out_dir() -> str:
    try:
        return os.path.dirname(os.path.abspath(__file__))
    except NameError:
        return os.path.abspath("cad/enclosure/v1")


OUT_DIR = _out_dir()


def box(x, y, z, sx, sy, sz):
    b = Part.makeBox(sx, sy, sz)
    b.translate(Vector(x, y, z))
    return b


def cyl(dx, dy, dz, radius, height, axis="z"):
    c = Part.makeCylinder(radius, height)
    if axis == "x":
        c.rotate(Vector(0, 0, 0), Vector(0, 1, 0), 90)
    elif axis == "y":
        c.rotate(Vector(0, 0, 0), Vector(1, 0, 0), -90)
    c.translate(Vector(dx, dy, dz))
    return c


def make_base():
    outer = box(0, 0, 0, OUTER_L, OUTER_W, OUTER_H)
    inner = box(
        WALL,
        WALL,
        FLOOR,
        OUTER_L - 2 * WALL,
        OUTER_W - 2 * WALL,
        OUTER_H,
    )
    shell = outer.cut(inner)

    ledge = box(
        WALL,
        WALL,
        OUTER_H - LID_LIP,
        OUTER_L - 2 * WALL,
        OUTER_W - 2 * WALL,
        LID_LIP + 0.1,
    )
    shell = shell.cut(ledge)

    inset = WALL + BOSS_OD / 2.0 + 0.5
    boss_centers = [
        (inset, inset),
        (OUTER_L - inset, inset),
        (inset, OUTER_W - inset),
        (OUTER_L - inset, OUTER_W - inset),
    ]
    for cx, cy in boss_centers:
        boss = cyl(cx, cy, FLOOR, BOSS_OD / 2.0, BOSS_H)
        hole = cyl(cx, cy, FLOOR - 0.1, BOSS_ID / 2.0, BOSS_H + 0.2)
        shell = shell.fuse(boss).cut(hole)

    def zone_ridges(zx, zy, zl, zw):
        return [
            box(WALL + zx, WALL + zy, FLOOR, zl, RIDGE_W, RIDGE_H),
            box(WALL + zx, WALL + zy + zw - RIDGE_W, FLOOR, zl, RIDGE_W, RIDGE_H),
            box(WALL + zx, WALL + zy, FLOOR, RIDGE_W, zw, RIDGE_H),
            box(WALL + zx + zl - RIDGE_W, WALL + zy, FLOOR, RIDGE_W, zw, RIDGE_H),
        ]

    for r in zone_ridges(ESP_ZONE_X, ESP_ZONE_Y, ESP_ZONE_L, ESP_ZONE_W):
        shell = shell.fuse(r)
    for r in zone_ridges(TMC_ZONE_X, TMC_ZONE_Y, TMC_ZONE_L, TMC_ZONE_W):
        shell = shell.fuse(r)

    shell = shell.cut(
        cyl(-0.5, BARREL_Y, BARREL_Z, BARREL_D / 2.0, WALL + 1.0, axis="x")
    )

    for dy in (-CABLE_Y_SPACING, 0.0, CABLE_Y_SPACING):
        shell = shell.cut(
            cyl(
                OUTER_L - WALL - 0.5,
                OUTER_W / 2.0 + dy,
                CABLE_Z,
                CABLE_D / 2.0,
                WALL + 1.0,
                axis="x",
            )
        )

    shell = shell.cut(
        box(
            USB_SLOT_X - USB_SLOT_W / 2.0,
            -0.5,
            USB_SLOT_Z - USB_SLOT_H / 2.0,
            USB_SLOT_W,
            WALL + 1.0,
            USB_SLOT_H,
        )
    )

    vent_span = TMC_ZONE_L
    start_x = WALL + TMC_ZONE_X
    step = vent_span / (VENT_COUNT + 1)
    for i in range(VENT_COUNT):
        vx = start_x + step * (i + 1) - VENT_W / 2.0
        shell = shell.cut(
            box(
                vx,
                OUTER_W - WALL - 0.5,
                VENT_Z - VENT_H / 2.0,
                VENT_W,
                WALL + 1.0,
                VENT_H,
            )
        )

    return shell.removeSplitter()


def make_lid():
    top = box(0, 0, 0, OUTER_L, OUTER_W, LID_H)
    lip_l = OUTER_L - 2 * WALL - 2 * CLEARANCE
    lip_w = OUTER_W - 2 * WALL - 2 * CLEARANCE
    lip = box(
        WALL + CLEARANCE,
        WALL + CLEARANCE,
        -LID_LIP,
        lip_l,
        lip_w,
        LID_LIP,
    )
    lip_inner = box(
        WALL + CLEARANCE + 1.5,
        WALL + CLEARANCE + 1.5,
        -LID_LIP - 0.1,
        lip_l - 3.0,
        lip_w - 3.0,
        LID_LIP + 0.2,
    )
    lid = top.fuse(lip.cut(lip_inner))

    inset = WALL + BOSS_OD / 2.0 + 0.5
    boss_centers = [
        (inset, inset),
        (OUTER_L - inset, inset),
        (inset, OUTER_W - inset),
        (OUTER_L - inset, OUTER_W - inset),
    ]
    for cx, cy in boss_centers:
        through = cyl(
            cx, cy, -LID_LIP - 0.1, SCREW_HOLE_LID / 2.0, LID_H + LID_LIP + 0.2
        )
        cs = cyl(
            cx, cy, LID_H - COUNTERSINK_H, COUNTERSINK_D / 2.0, COUNTERSINK_H + 0.05
        )
        lid = lid.cut(through).cut(cs)

    for i in range(4):
        vx = WALL + TMC_ZONE_X + 6.0 + i * 8.0
        vy = WALL + TMC_ZONE_Y + TMC_ZONE_W / 2.0 - 8.0
        lid = lid.cut(box(vx, vy, -0.1, 2.0, 16.0, LID_H + 0.2))

    return lid.removeSplitter()


def export_step(shape, path):
    shape.exportStep(path)
    print(f"Wrote {path}")


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    doc = App.newDocument("enclosure_monolithic")
    base = make_base()
    lid = make_lid()

    lid_asm = lid.copy()
    lid_asm.translate(Vector(0, 0, OUTER_H + 0.5))

    Part.show(base, "base")
    Part.show(lid, "lid")
    Part.show(base.fuse(lid_asm), "assembly")

    export_step(base, os.path.join(OUT_DIR, "enclosure_base.step"))
    export_step(lid, os.path.join(OUT_DIR, "enclosure_lid.step"))
    export_step(base.fuse(lid_asm), os.path.join(OUT_DIR, "enclosure_assembly.step"))

    fcstd = os.path.join(OUT_DIR, "enclosure_monolithic.FCStd")
    doc.recompute()
    doc.saveAs(fcstd)
    print(f"Wrote {fcstd}")
    print(
        f"v1 envelope: {OUTER_L} x {OUTER_W} x {OUTER_H + LID_H:.1f} mm "
        f"(base {OUTER_H} + lid {LID_H})"
    )
    return 0


sys.exit(main())
