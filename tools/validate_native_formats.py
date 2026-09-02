#!/usr/bin/env python3
"""Validate AIM Editor's native JSON interchange examples against schemas."""
from __future__ import annotations

import json
from pathlib import Path

try:
    from jsonschema import Draft202012Validator
    from referencing import Registry, Resource
except ImportError as exc:
    raise SystemExit(
        "ERROR: Draft 2020-12 validation support is unavailable in this Python. "
        "Run tools/bootstrap_python_tools.py and execute repository checks with "
        ".deps/python-tools/bin/python (Windows: .deps\\python-tools\\Scripts\\python.exe)."
    ) from exc

ROOT = Path(__file__).resolve().parents[1]
SCHEMAS = ROOT / "schemas"


def load(name: str):
    return json.loads((SCHEMAS / name).read_text())


PROGRAM_SCHEMA = load("program.schema.json")
BANK_SCHEMA = load("bank.schema.json")
REGISTRY = (
    Registry()
    .with_resource("https://aim-editor.org/schemas/program.schema.json", Resource.from_contents(PROGRAM_SCHEMA))
    .with_resource("https://aim-editor.org/schemas/bank.schema.json", Resource.from_contents(BANK_SCHEMA))
    .with_resource("program.schema.json", Resource.from_contents(PROGRAM_SCHEMA))
)


def validate(schema: dict, value: object) -> None:
    Draft202012Validator(schema, registry=REGISTRY).validate(value)


def main() -> int:
    program = {
        "format": "aim-editor.program",
        "schema_version": 1,
        "name": "Schema Test",
        "category": "Test",
        "parameters": {
            "filter1": {"frequency": 777, "resonance": 12},
            "voice": {"unison": 2},
        },
        "unmapped_bytes": [{"offset": 206, "value": 37}],
        "source_patch": None,
    }
    validate(PROGRAM_SCHEMA, program)

    source_backed_program = {**program, "source_patch": {
        "format": "ion-decoded-patch-v1",
        "bytes": [0] * 378,
    }}
    validate(PROGRAM_SCHEMA, source_backed_program)

    bank = {
        "format": "aim-editor.bank",
        "schema_version": 1,
        "name": "Schema Test Bank",
        "hardware_bank": "yellow",
        "programs": [
            {"slot": 0, "program": program},
            {"slot": 127, "program": {**program, "name": "Last Slot"}},
        ],
    }
    validate(BANK_SCHEMA, bank)

    print("PASS: native program/bank JSON examples validate against Draft 2020-12 schemas")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
