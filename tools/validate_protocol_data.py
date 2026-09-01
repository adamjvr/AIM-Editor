#!/usr/bin/env python3
"""Validate AIM Editor's human-readable protocol research datasets."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def die(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def main() -> int:
    params = json.loads((ROOT / "data/parameters.json").read_text())
    nrpn = json.loads((ROOT / "data/protocol/ion-nrpn.json").read_text())
    sysex = json.loads((ROOT / "data/protocol/ion-sysex.json").read_text())

    nrpn_numbers: set[int] = set()
    nrpn_ids: set[str] = set()
    for entry in nrpn["parameters"]:
        number = entry["number"]
        if not 0 <= number <= 16383:
            die(f"NRPN outside 14-bit range: {number}")
        if number in nrpn_numbers:
            die(f"duplicate NRPN number: {number}")
        if entry["id"] in nrpn_ids:
            die(f"duplicate NRPN id: {entry['id']}")
        if entry["min"] > entry["max"]:
            die(f"reversed NRPN range: {entry['id']}")
        nrpn_numbers.add(number)
        nrpn_ids.add(entry["id"])

    sysex_ids: set[str] = set()
    for field in sysex["fields"]:
        if field["id"] in sysex_ids:
            die(f"duplicate SysEx field id: {field['id']}")
        sysex_ids.add(field["id"])

    mapped_nrpn = 0
    mapped_sysex = 0
    for param in params["parameters"]:
        protocol = param["protocol"]
        if protocol.get("nrpn") is not None:
            mapped_nrpn += 1
            evidence = protocol.get("nrpn_evidence") or {}
            if evidence.get("source") != "data/protocol/ion-nrpn.json":
                die(f"NRPN mapping lacks canonical evidence: {param['id']}")
            if evidence.get("value_encoding") not in {"unsigned_14", "signed_14_wrap"}:
                die(f"NRPN mapping lacks value encoding: {param['id']}")
        sysex_mapping = protocol.get("sysex") or {}
        if sysex_mapping.get("offset") is not None:
            mapped_sysex += 1

    print(f"PASS: {len(nrpn_numbers)} candidate NRPN definitions")
    print(f"PASS: {len(sysex_ids)} candidate SysEx fields")
    print(f"PASS: {mapped_nrpn}/{len(params['parameters'])} UI parameters have candidate NRPN mappings")
    print(f"PASS: {mapped_sysex}/{len(params['parameters'])} UI parameters have candidate SysEx mappings")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
