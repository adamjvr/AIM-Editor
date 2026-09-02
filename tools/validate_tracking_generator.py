#!/usr/bin/env python3
"""Verify the 33-point candidate Tracking Generator model stays internally consistent."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    params = json.loads((ROOT / "data/parameters.json").read_text())["parameters"]
    nrpn = json.loads((ROOT / "data/protocol/ion-nrpn.json").read_text())["parameters"]
    sysex = json.loads((ROOT / "data/protocol/ion-sysex.json").read_text())["fields"]

    by_id = {p["id"]: p for p in params}
    nrpn_by_id = {p["id"]: p for p in nrpn}
    sysex_by_offset = {p["offset"]: p for p in sysex if isinstance(p.get("offset"), int)}

    defaults = [-100, -93, -87, -81, -75, -68, -62, -56, -50, -43, -37, -31, -25, -18, -12, -6,
                0, 6, 12, 18, 25, 31, 37, 43, 50, 56, 62, 68, 75, 81, 87, 93, 100]

    for index, point in enumerate(range(-16, 17)):
        suffix = f"minus_{abs(point)}" if point < 0 else f"plus_{point}"
        pid = f"tracking_generator.point_{suffix}"
        assert pid in by_id, pid
        p = by_id[pid]
        assert p["domain"]["raw_min"] == -100 and p["domain"]["raw_max"] == 100, pid
        assert p["domain"]["default_raw"] == defaults[index], pid
        assert p["protocol"]["nrpn"] == 121 + index, pid
        assert p["protocol"]["sysex"]["offset"] == 304 + index, pid
        assert p["protocol"]["sysex"]["encoding"] == "s8", pid
        assert nrpn_by_id[pid]["number"] == 121 + index, pid
        assert sysex_by_offset[304 + index]["raw_min"] == -100, pid
        assert sysex_by_offset[304 + index]["raw_max"] == 100, pid

    assert by_id["tracking_generator.preset"]["protocol"]["nrpn"] == 119
    assert by_id["tracking_generator.preset"]["protocol"]["sysex"]["offset"] == 337
    assert by_id["tracking_generator.point_count"]["domain"]["raw_min"] == 0
    assert by_id["tracking_generator.point_count"]["domain"]["raw_max"] == 1

    print("PASS: 33 Tracking Generator curve points + preset are internally consistent")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
