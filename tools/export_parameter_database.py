#!/usr/bin/env python3
"""Export AIM Editor's canonical parameter database to CSV or normalized JSON."""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INPUT = ROOT / "data" / "parameters.json"


def flatten_parameter(parameter: dict) -> dict:
    protocol = parameter.get("protocol", {})
    sysex = protocol.get("sysex", {}) or {}
    domain = parameter.get("domain", {})
    display = parameter.get("display", {})
    ui = parameter.get("ui", {})
    return {
        "id": parameter.get("id"),
        "name": parameter.get("name"),
        "section": parameter.get("section"),
        "type": parameter.get("type"),
        "raw_min": domain.get("raw_min"),
        "raw_max": domain.get("raw_max"),
        "default_raw": domain.get("default_raw"),
        "unit": display.get("unit"),
        "display_transform": display.get("transform"),
        "mapping_status": protocol.get("status"),
        "nrpn": protocol.get("nrpn"),
        "sysex_offset": sysex.get("offset"),
        "sysex_bits": sysex.get("bits"),
        "sysex_encoding": sysex.get("encoding"),
        "ui_pages": ";".join(ui.get("pages", [])),
        "ui_control": ui.get("control"),
        "notes": parameter.get("notes"),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT)
    parser.add_argument("--format", choices=("csv", "json"), default="csv")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    database = json.loads(args.input.read_text(encoding="utf-8"))
    args.output.parent.mkdir(parents=True, exist_ok=True)

    if args.format == "json":
        args.output.write_text(json.dumps(database, indent=2) + "\n", encoding="utf-8")
        return 0

    rows = [flatten_parameter(item) for item in database["parameters"]]
    fieldnames = list(rows[0]) if rows else []
    with args.output.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
