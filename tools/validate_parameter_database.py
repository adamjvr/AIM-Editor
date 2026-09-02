#!/usr/bin/env python3
"""Fast dependency-free structural validation for AIM Editor parameter data.

This validator understands both inline enum domains and reusable enum tables in
``data/enums``.  Reusable tables are deliberately first-class project data:
they keep large protocol enumerations human-readable without duplicating them
across every parameter that consumes the same domain.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DATABASE = ROOT / "data" / "parameters.json"
ENUM_DIR = ROOT / "data" / "enums"
VALID_TYPES = {"continuous", "discrete", "enum", "boolean"}
VALID_STATUS = {"unmapped", "candidate", "verified"}
VALID_PAGES = {"front", "dual1", "dual2", "randomizer", "rear"}
VALID_CONTROLS = {"knob", "wave_knob", "selector", "toggle", "compact_value"}


def load_enum_tables() -> dict[str, list[dict[str, Any]]]:
    tables: dict[str, list[dict[str, Any]]] = {}

    for path in sorted(ENUM_DIR.glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        assert data.get("format") == "aim-editor.enum-table", f"bad enum-table format: {path}"
        assert isinstance(data.get("schema_version"), int) and data["schema_version"] >= 1, path

        table_id = data.get("id")
        assert isinstance(table_id, str) and table_id, f"missing enum-table id: {path}"
        assert table_id not in tables, f"duplicate enum-table id: {table_id}"

        values = data.get("values")
        assert isinstance(values, list) and values, f"empty enum table: {table_id}"

        raws: list[int] = []
        ids: list[str] = []
        for entry in values:
            assert isinstance(entry, dict), f"invalid enum entry: {table_id}"
            raw = entry.get("value")
            value_id = entry.get("id")
            name = entry.get("name")
            assert isinstance(raw, int), f"non-integer enum value: {table_id}"
            assert isinstance(value_id, str) and value_id, f"empty enum id: {table_id}"
            assert isinstance(name, str) and name, f"empty enum name: {table_id}/{value_id}"
            raws.append(raw)
            ids.append(value_id)

        assert len(raws) == len(set(raws)), f"duplicate enum raw values: {table_id}"
        assert len(ids) == len(set(ids)), f"duplicate enum ids: {table_id}"
        tables[table_id] = values

    return tables


def integer_domain_complete(values: list[dict[str, Any]], lo: Any, hi: Any, raw_key: str) -> bool:
    if not isinstance(lo, int) or not isinstance(hi, int):
        return False
    expected = set(range(lo, hi + 1))
    actual = {entry[raw_key] for entry in values}
    return actual == expected


def main() -> int:
    data = json.loads(DATABASE.read_text(encoding="utf-8"))
    assert data["format"] == "aim-editor.parameter-database"
    assert isinstance(data["schema_version"], int) and data["schema_version"] >= 1

    enum_tables = load_enum_tables()
    seen: set[str] = set()
    resolved_enum_parameters = 0
    complete_enum_parameters = 0
    incomplete_enum_parameters = 0
    external_enum_references = 0

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

        inline_values = domain.get("values") or []
        enum_table_id = domain.get("enum_table")
        assert not (inline_values and enum_table_id), f"enum uses inline and external values: {pid}"

        resolved_values: list[dict[str, Any]] = []
        raw_key = "raw"
        if inline_values:
            resolved_values = inline_values
        elif enum_table_id is not None:
            assert isinstance(enum_table_id, str) and enum_table_id, f"invalid enum_table id: {pid}"
            assert enum_table_id in enum_tables, f"unknown enum_table {enum_table_id!r}: {pid}"
            resolved_values = enum_tables[enum_table_id]
            raw_key = "value"
            external_enum_references += 1

        if resolved_values:
            resolved_enum_parameters += 1
            raws = [entry[raw_key] for entry in resolved_values]
            ids = [entry["id"] for entry in resolved_values]
            assert len(raws) == len(set(raws)), f"duplicate enum raw values: {pid}"
            assert len(ids) == len(set(ids)), f"duplicate enum ids: {pid}"
            assert all(value_id for value_id in ids), f"empty enum id: {pid}"
            if lo is not None:
                assert min(raws) >= lo, f"enum raw below raw_min: {pid}"
            if hi is not None:
                assert max(raws) <= hi, f"enum raw above raw_max: {pid}"

        if item["type"] == "enum":
            if resolved_values and integer_domain_complete(resolved_values, lo, hi, raw_key):
                complete_enum_parameters += 1
            else:
                # Incomplete enum tables are legal during reverse engineering.
                # Unknown raw values must remain representable rather than being
                # silently coerced to a known value.
                incomplete_enum_parameters += 1

        if item["protocol"]["status"] == "verified":
            assert item["protocol"]["nrpn"] is not None or item["protocol"]["sysex"]["offset"] is not None, (
                f"verified mapping lacks a concrete protocol address: {pid}"
            )

    print(
        "PASS: "
        f"{len(seen)} unique parameter definitions / "
        f"{len(enum_tables)} reusable enum tables / "
        f"{resolved_enum_parameters} enum parameters resolved "
        f"({external_enum_references} external references) / "
        f"{complete_enum_parameters} complete / "
        f"{incomplete_enum_parameters} incomplete"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
