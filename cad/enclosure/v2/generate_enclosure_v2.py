#!/usr/bin/env freecadcmd
"""
v2 — Sectional dry-side electronics enclosure (per-print ≤ 200 mm).

Modules:
  controller  — breadboard + buck bay + barrel jack + USB + fan lid
  driver_x1   — one generic TMC/breakout bay
  driver_x2   — two generic TMC/breakout bays
  endcap      — cable exits toward wet side

Mating: +X tongue into next module's −X groove; M3 clamp holes at the joint.

Writes only into cad/enclosure/v2/ — does not delete or overwrite v1 exports.

Run:
  /Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd \\
    cad/enclosure/v2/generate_enclosure_v2.py
"""

from __future__ import annotations

import os
import sys

import FreeCAD as App
import Part
from FreeCAD import Vector

# ---------------------------------------------------------------------------
# Shared parameters (mm)
# ---------------------------------------------------------------------------

MAX_PRINT_L = 200.0

WALL = 2.5
FLOOR = 2.5
OUTER_H = 50.0
OUTER_W = 95.0
LID_H = 3.0
LID_LIP = 1.5
CLEARANCE = 0.35

# M3 heat-set inserts in base bosses; clearance + countersink in lid
INSERT_HOLE_D = 4.0
BOSS_OD = 9.0
BOSS_H = OUTER_H - FLOOR - 1.0
SCREW_HOLE_LID = 3.2
COUNTERSINK_D = 6.0
COUNTERSINK_H = 1.8

# Tongue / groove joint along X
TONGUE_DEPTH = 6.0
TONGUE_T = 2.0
TONGUE_CLEAR = 0.3
JOINT_BOLT_D = 3.2
JOINT_BOLT_Z = FLOOR + 18.0

# Floor ridge guides
RIDGE_W = 1.5
RIDGE_H = 1.2

# Half-size breadboard 400-point
BB_L = 82.0
BB_W = 55.0

# Generic buck bay (~40 × 20 × 10 mm part + margin)
BUCK_L = 42.0
BUCK_W = 22.0

# Generic driver / breakout bay (placeholder until exact board chosen)
DRIVER_BAY_L = 55.0
DRIVER_BAY_W = 50.0

# Panel features
BARREL_D = 11.0
BARREL_Z = FLOOR + 14.0
USB_SLOT_W = 14.0
USB_SLOT_H = 8.0
USB_SLOT_Z = FLOOR + 12.0
CABLE_D = 10.0
CABLE_Z = FLOOR + 14.0
CABLE_Y_SPACING = 22.0

VENT_W = 2.5
VENT_H = 14.0
VENT_Z = FLOOR + 18.0

# 40 mm fan mount on controller lid
FAN_SIZE = 40.0
FAN_HOLE_PITCH = 32.0
FAN_OPEN_D = 37.0
FAN_SCREW_D = 3.2

FILLET = 0.0  # keep sharp for reliable boolean export


def _out_dir() -> str:
    try:
        return os.path.dirname(os.path.abspath(__file__))
    except NameError:
        return os.path.abspath("cad/enclosure/v2")


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


def boss_inset() -> float:
    return WALL + BOSS_OD / 2.0 + 0.5


def corner_boss_xy(length: float):
    inset = boss_inset()
    return [
        (inset, inset),
        (length - inset, inset),
        (inset, OUTER_W - inset),
        (length - inset, OUTER_W - inset),
    ]


def zone_ridges(zx, zy, zl, zw):
    return [
        box(WALL + zx, WALL + zy, FLOOR, zl, RIDGE_W, RIDGE_H),
        box(WALL + zx, WALL + zy + zw - RIDGE_W, FLOOR, zl, RIDGE_W, RIDGE_H),
        box(WALL + zx, WALL + zy, FLOOR, RIDGE_W, zw, RIDGE_H),
        box(WALL + zx + zl - RIDGE_W, WALL + zy, FLOOR, RIDGE_W, zw, RIDGE_H),
    ]


def add_corner_bosses(shell, length: float):
    for cx, cy in corner_boss_xy(length):
        boss = cyl(cx, cy, FLOOR, BOSS_OD / 2.0, BOSS_H)
        hole = cyl(cx, cy, FLOOR - 0.1, INSERT_HOLE_D / 2.0, BOSS_H + 0.2)
        shell = shell.fuse(boss).cut(hole)
    return shell


def cut_lid_ledge(shell, length: float):
    ledge = box(
        WALL,
        WALL,
        OUTER_H - LID_LIP,
        length - 2 * WALL,
        OUTER_W - 2 * WALL,
        LID_LIP + 0.1,
    )
    return shell.cut(ledge)


def make_hollow_shell(length: float):
    outer = box(0, 0, 0, length, OUTER_W, OUTER_H)
    inner = box(
        WALL,
        WALL,
        FLOOR,
        length - 2 * WALL,
        OUTER_W - 2 * WALL,
        OUTER_H,
    )
    return outer.cut(inner)


def tongue_solid(length: float):
    """Protruding tongue on +X face (male joint)."""
    y0 = WALL + 4.0
    yw = OUTER_W - 2 * WALL - 8.0
    z0 = FLOOR + 2.0
    zh = OUTER_H - FLOOR - LID_LIP - 4.0
    return box(length - 0.05, y0, z0, TONGUE_DEPTH + 0.05, yw, zh)


def groove_cut(length_ignored: float = 0.0):
    """Female groove cut into −X face."""
    y0 = WALL + 4.0 - TONGUE_CLEAR
    yw = OUTER_W - 2 * WALL - 8.0 + 2 * TONGUE_CLEAR
    z0 = FLOOR + 2.0 - TONGUE_CLEAR
    zh = OUTER_H - FLOOR - LID_LIP - 4.0 + 2 * TONGUE_CLEAR
    return box(
        -0.1,
        y0,
        z0,
        WALL + TONGUE_DEPTH + TONGUE_CLEAR + 0.2,
        yw,
        zh,
    )


def joint_bolt_holes(shell, x_face: float, from_inside: bool):
    """Horizontal M3 clamp holes near a mating face (through ±Y walls)."""
    ys = (WALL / 2.0, OUTER_W - WALL / 2.0)
    x = x_face - 3.0 if from_inside else x_face + 3.0
    for y in ys:
        # drill through full width for simpler tooling; only walls matter
        hole = cyl(
            x,
            -1.0,
            JOINT_BOLT_Z,
            JOINT_BOLT_D / 2.0,
            OUTER_W + 2.0,
            axis="y",
        )
        shell = shell.cut(hole)
    return shell


def side_vents(shell, length: float, x0: float, span: float, count: int = 5):
    step = span / (count + 1)
    for i in range(count):
        vx = x0 + step * (i + 1) - VENT_W / 2.0
        vent = box(
            vx,
            OUTER_W - WALL - 0.5,
            VENT_Z - VENT_H / 2.0,
            VENT_W,
            WALL + 1.0,
            VENT_H,
        )
        shell = shell.cut(vent)
        vent2 = box(
            vx,
            -0.5,
            VENT_Z - VENT_H / 2.0,
            VENT_W,
            WALL + 1.0,
            VENT_H,
        )
        shell = shell.cut(vent2)
    return shell


def make_lid(length: float, fan: bool = False, vent_x0: float | None = None, vent_span: float = 40.0):
    top = box(0, 0, 0, length, OUTER_W, LID_H)
    lip_l = length - 2 * WALL - 2 * CLEARANCE
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

    for cx, cy in corner_boss_xy(length):
        through = cyl(
            cx, cy, -LID_LIP - 0.1, SCREW_HOLE_LID / 2.0, LID_H + LID_LIP + 0.2
        )
        cs = cyl(cx, cy, LID_H - COUNTERSINK_H, COUNTERSINK_D / 2.0, COUNTERSINK_H + 0.05)
        lid = lid.cut(through).cut(cs)

    if fan:
        fx = length / 2.0
        fy = OUTER_W / 2.0
        lid = lid.cut(cyl(fx, fy, -0.1, FAN_OPEN_D / 2.0, LID_H + 0.2))
        half = FAN_HOLE_PITCH / 2.0
        for dx, dy in ((-half, -half), (half, -half), (-half, half), (half, half)):
            lid = lid.cut(
                cyl(fx + dx, fy + dy, -0.1, FAN_SCREW_D / 2.0, LID_H + 0.2)
            )

    if vent_x0 is not None:
        for i in range(4):
            vx = vent_x0 + 4.0 + i * 8.0
            vy = OUTER_W / 2.0 - 10.0
            lid = lid.cut(box(vx, vy, -0.1, 2.0, 20.0, LID_H + 0.2))

    return lid.removeSplitter()


# ---------------------------------------------------------------------------
# Module lengths
# ---------------------------------------------------------------------------

# controller: wall | margin | buck | gap | breadboard | gap | tongue seat | wall
CONTROLLER_L = (
    WALL
    + 4.0
    + BUCK_L
    + 8.0
    + BB_L
    + 6.0
    + TONGUE_DEPTH
    + WALL
)

DRIVER_X1_L = WALL + TONGUE_DEPTH + 4.0 + DRIVER_BAY_L + 4.0 + TONGUE_DEPTH + WALL
DRIVER_X2_L = (
    WALL
    + TONGUE_DEPTH
    + 4.0
    + DRIVER_BAY_L
    + 8.0
    + DRIVER_BAY_L
    + 4.0
    + TONGUE_DEPTH
    + WALL
)
ENDCAP_L = WALL + TONGUE_DEPTH + 8.0 + 20.0 + WALL


def assert_print_limit(name: str, length: float):
    if length > MAX_PRINT_L:
        raise RuntimeError(f"{name} length {length:.1f} mm exceeds {MAX_PRINT_L} mm")


def make_controller_base():
    assert_print_limit("controller", CONTROLLER_L)
    length = CONTROLLER_L
    shell = make_hollow_shell(length)
    shell = cut_lid_ledge(shell, length)
    shell = add_corner_bosses(shell, length)

    buck_x = 4.0
    buck_y = (OUTER_W - 2 * WALL - BUCK_W) / 2.0
    bb_x = buck_x + BUCK_L + 8.0
    bb_y = (OUTER_W - 2 * WALL - BB_W) / 2.0

    for r in zone_ridges(buck_x, buck_y, BUCK_L, BUCK_W):
        shell = shell.fuse(r)
    for r in zone_ridges(bb_x, bb_y, BB_L, BB_W):
        shell = shell.fuse(r)

    # Barrel jack (−X)
    shell = shell.cut(
        cyl(-0.5, OUTER_W / 2.0, BARREL_Z, BARREL_D / 2.0, WALL + 1.0, axis="x")
    )

    # USB slot (−Y), aligned to breadboard mid
    usb_x = WALL + bb_x + BB_L / 2.0
    shell = shell.cut(
        box(
            usb_x - USB_SLOT_W / 2.0,
            -0.5,
            USB_SLOT_Z - USB_SLOT_H / 2.0,
            USB_SLOT_W,
            WALL + 1.0,
            USB_SLOT_H,
        )
    )

    shell = side_vents(shell, length, WALL + bb_x, BB_L, count=6)
    shell = shell.fuse(tongue_solid(length))
    shell = joint_bolt_holes(shell, length - WALL, from_inside=True)
    return shell.removeSplitter()


def make_driver_base(bay_count: int):
    length = DRIVER_X1_L if bay_count == 1 else DRIVER_X2_L
    name = f"driver_x{bay_count}"
    assert_print_limit(name, length)

    shell = make_hollow_shell(length)
    shell = cut_lid_ledge(shell, length)
    shell = add_corner_bosses(shell, length)
    shell = shell.cut(groove_cut())

    bay_y = (OUTER_W - 2 * WALL - DRIVER_BAY_W) / 2.0
    x = TONGUE_DEPTH + 4.0
    for _ in range(bay_count):
        for r in zone_ridges(x, bay_y, DRIVER_BAY_L, DRIVER_BAY_W):
            shell = shell.fuse(r)
        shell = side_vents(shell, length, WALL + x, DRIVER_BAY_L, count=4)
        x += DRIVER_BAY_L + 8.0

    shell = shell.fuse(tongue_solid(length))
    shell = joint_bolt_holes(shell, WALL, from_inside=False)
    shell = joint_bolt_holes(shell, length - WALL, from_inside=True)
    return shell.removeSplitter()


def make_endcap_base():
    assert_print_limit("endcap", ENDCAP_L)
    length = ENDCAP_L
    shell = make_hollow_shell(length)
    shell = cut_lid_ledge(shell, length)
    shell = add_corner_bosses(shell, length)
    shell = shell.cut(groove_cut())
    shell = joint_bolt_holes(shell, WALL, from_inside=False)

    # Cable exits on +X (wet side)
    for dy in (-CABLE_Y_SPACING, 0.0, CABLE_Y_SPACING):
        shell = shell.cut(
            cyl(
                length - WALL - 0.5,
                OUTER_W / 2.0 + dy,
                CABLE_Z,
                CABLE_D / 2.0,
                WALL + 1.0,
                axis="x",
            )
        )
    return shell.removeSplitter()


def make_controller_lid():
    return make_lid(CONTROLLER_L, fan=True)


def make_driver_lid(bay_count: int):
    length = DRIVER_X1_L if bay_count == 1 else DRIVER_X2_L
    x0 = WALL + TONGUE_DEPTH + 4.0
    return make_lid(length, fan=False, vent_x0=x0, vent_span=DRIVER_BAY_L)


def make_endcap_lid():
    return make_lid(ENDCAP_L, fan=False)


def export_step(shape, path):
    shape.exportStep(path)
    print(f"Wrote {path}")


def place(shape, x: float):
    s = shape.copy()
    s.translate(Vector(x, 0, 0))
    return s


def assemble(modules):
    """modules: list of (base, lid, length)."""
    asm = None
    x = 0.0
    for base, lid, length in modules:
        b = place(base, x)
        l = place(lid, x)
        l.translate(Vector(0, 0, OUTER_H + 0.5))
        chunk = b.fuse(l)
        asm = chunk if asm is None else asm.fuse(chunk)
        # nest tongue into next groove: advance by length - tongue overlap
        x += length - TONGUE_DEPTH
    return asm


def pump_bill(n: int):
    """Return list of (kind, count) sections after controller for n pumps."""
    # Prefer driver_x2 blocks, remainder driver_x1
    twos = n // 2
    ones = n % 2
    parts = []
    for _ in range(twos):
        parts.append("x2")
    for _ in range(ones):
        parts.append("x1")
    return parts


def main():
    for name, length in (
        ("controller", CONTROLLER_L),
        ("driver_x1", DRIVER_X1_L),
        ("driver_x2", DRIVER_X2_L),
        ("endcap", ENDCAP_L),
    ):
        assert_print_limit(name, length)
        print(f"{name}: {length:.1f} x {OUTER_W:.1f} x {OUTER_H + LID_H:.1f} mm")

    doc = App.newDocument("enclosure_sectional")

    controller_base = make_controller_base()
    controller_lid = make_controller_lid()
    driver1_base = make_driver_base(1)
    driver1_lid = make_driver_lid(1)
    driver2_base = make_driver_base(2)
    driver2_lid = make_driver_lid(2)
    endcap_base = make_endcap_base()
    endcap_lid = make_endcap_lid()

    Part.show(controller_base, "controller_base")
    Part.show(controller_lid, "controller_lid")
    Part.show(driver1_base, "driver_x1_base")
    Part.show(driver1_lid, "driver_x1_lid")
    Part.show(driver2_base, "driver_x2_base")
    Part.show(driver2_lid, "driver_x2_lid")
    Part.show(endcap_base, "endcap_base")
    Part.show(endcap_lid, "endcap_lid")

    exports = [
        (controller_base, "controller_base.step"),
        (controller_lid, "controller_lid.step"),
        (driver1_base, "driver_x1_base.step"),
        (driver1_lid, "driver_x1_lid.step"),
        (driver2_base, "driver_x2_base.step"),
        (driver2_lid, "driver_x2_lid.step"),
        (endcap_base, "endcap_base.step"),
        (endcap_lid, "endcap_lid.step"),
    ]
    for shape, filename in exports:
        export_step(shape, os.path.join(OUT_DIR, filename))

    # Assembly previews for 1- and 6-pump (and all N as lightweight notes)
    catalog = {
        "x1": (driver1_base, driver1_lid, DRIVER_X1_L),
        "x2": (driver2_base, driver2_lid, DRIVER_X2_L),
    }

    for n in (1, 2, 3, 4, 5, 6):
        modules = [
            (controller_base, controller_lid, CONTROLLER_L),
        ]
        for kind in pump_bill(n):
            modules.append(catalog[kind])
        modules.append((endcap_base, endcap_lid, ENDCAP_L))
        asm = assemble(modules)
        path = os.path.join(OUT_DIR, f"assembly_{n}pump.step")
        export_step(asm, path)
        # assembled length estimate
        total = CONTROLLER_L + ENDCAP_L - TONGUE_DEPTH
        for kind in pump_bill(n):
            total += (DRIVER_X1_L if kind == "x1" else DRIVER_X2_L) - TONGUE_DEPTH
        print(f"assembly_{n}pump assembled length ≈ {total:.1f} mm")

    fcstd = os.path.join(OUT_DIR, "enclosure_sectional.FCStd")
    doc.recompute()
    doc.saveAs(fcstd)
    print(f"Wrote {fcstd}")

    return 0


sys.exit(main())
