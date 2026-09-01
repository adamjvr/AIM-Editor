#!/usr/bin/env python3
"""Fast dependency-free structural validation for AIM Editor parameter data."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATABASE = ROOT / "data" / "parameters.json"
VALID_TYPES = {"continuous", "discrete", "enum", "boolean"}
VALID_STATUS = {"unmapped", "candidate", "verified"}
VALID_PAGES = {"front", "dual1", "dual2", "randomizer", "rear"}
VALID_CONTROLS = {"knob", "wave_knob", "selector", "toggle", "compact_value"}


def main() -> int:
    data = json.loads(DATABASE.read_text(encoding="utf-8"))
    assert data["format"] == "aim-editor.parameter-database"
    assert isinstance(data["schema_version"], int) and data["schema_version"] >= 1

    seen: set[str] = set()
    enum_count = 0
    incomplete_enum_count = 0
    for item in data["parameters"]:
        pid = item["id"]
        assert pid and pid not in seen, f"duplicate/empty parameter id: {pid!r}"
        seen.add(pid)
        assert item["type"] in VALID_TYPES, pid
        assert item["protocol"]["status"] in VALID_STATUS, pid
        assert set(item["ui"]["pages"]).issubset(VALID_PAGES), pid
        assert item["ui"]["control"] in VALID_CONTROLS, pid

        domain = item["domain"]
        lo, hi = domain.get("raw_min"), domain.get("raw_max")
        if lo is not None and hi is not None:
            assert lo <= hi, f"invalid raw domain: {pid}"
        default = domain.get("default_raw")
        if default is not None and lo is not None:
            assert default >= lo, f"default below raw_min: {pid}"
        if default is not None and hi is not None:
            assert default <= hi, f"default above raw_max: {pid}"

        values = domain.get("values") or []
        if values:
            enum_count += 1
            raws = [entry["raw"] for entry in values]
            ids = [entry["id"] for entry in values]
            assert len(raws) == len(set(raws)), f"duplicate enum raw values: {pid}"
            assert len(ids) == len(set(ids)), f"duplicate enum ids: {pid}"
            assert all(value_id for value_id in ids), f"empty enum id: {pid}"
            if lo is not None:
                assert min(raws) >= lo, f"enum raw below raw_min: {pid}"
            if hi is not None:
                assert max(raws) <= hi, f"enum raw above raw_max: {pid}"

        if item["type"] == "enum" and not values and (lo is None or hi is None):
            # This is legal during reverse engineering, but it is tracked so the
            # count cannot silently disappear from the project status.
            incomplete_enum_count += 1

        if item["protocol"]["status"] == "verified":
            assert item["protocol"]["nrpn"] is not None or item["protocol"]["sysex"]["offset"] is not None, (
                f"verified mapping lacks a concrete protocol address: {pid}"
            )

    print(f"PASS: {len(seen)} unique parameter definitions / {enum_count} explicit enum domains / {incomplete_enum_count} incomplete enum domains")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
