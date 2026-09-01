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


def main() -> int:
    data = json.loads(DATABASE.read_text(encoding="utf-8"))
    assert data["format"] == "aim-editor.parameter-database"
    assert isinstance(data["schema_version"], int) and data["schema_version"] >= 1

    seen: set[str] = set()
    for item in data["parameters"]:
        pid = item["id"]
        assert pid and pid not in seen, f"duplicate/empty parameter id: {pid!r}"
        seen.add(pid)
        assert item["type"] in VALID_TYPES, pid
        assert item["protocol"]["status"] in VALID_STATUS, pid
        assert set(item["ui"]["pages"]).issubset(VALID_PAGES), pid
        if item["protocol"]["status"] == "verified":
            assert item["protocol"]["nrpn"] is not None or item["protocol"]["sysex"]["offset"] is not None, (
                f"verified mapping lacks a concrete protocol address: {pid}"
            )

    print(f"PASS: {len(seen)} unique parameter definitions")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
