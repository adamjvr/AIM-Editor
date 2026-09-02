#!/usr/bin/env python3
"""Validate AIM Editor's human-readable protocol research datasets."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def die(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def validate_evidence_refs(owner: str, item: dict) -> None:
    refs = item.get("verification_evidence")
    if item.get("status") != "verified":
        return
    if item.get("hardware_verified") is not True:
        die(f"verified mapping is not marked hardware_verified: {owner}")
    if not isinstance(refs, list) or not refs:
        die(f"verified mapping lacks verification evidence: {owner}")
    for ref in refs:
        if not isinstance(ref, str) or not ref.startswith("research/verification/"):
            die(f"invalid verification evidence reference for {owner}: {ref!r}")
        if not (ROOT / ref).is_file():
            die(f"verification evidence file does not exist for {owner}: {ref}")


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
        validate_evidence_refs(f"NRPN {entry['id']}", entry)
        nrpn_numbers.add(number)
        nrpn_ids.add(entry["id"])

    sysex_ids: set[str] = set()
    for field in sysex["fields"]:
        if field["id"] in sysex_ids:
            die(f"duplicate SysEx field id: {field['id']}")
        validate_evidence_refs(f"SysEx {field['id']}", field)
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

        nrpn_present = protocol.get("nrpn") is not None
        sysex_present = sysex_mapping.get("offset") is not None
        nrpn_evidence = protocol.get("nrpn_evidence") or {}
        sysex_evidence = protocol.get("evidence") or {}
        if nrpn_evidence.get("status") == "verified":
            validate_evidence_refs(f"parameter NRPN {param['id']}", nrpn_evidence)
        if sysex_evidence.get("status") == "verified":
            validate_evidence_refs(f"parameter SysEx {param['id']}", sysex_evidence)

        if not nrpn_present and not sysex_present:
            expected_status = "unmapped"
        else:
            nrpn_ok = (not nrpn_present) or nrpn_evidence.get("status") == "verified"
            sysex_ok = (not sysex_present) or sysex_evidence.get("status") == "verified"
            expected_status = "verified" if nrpn_ok and sysex_ok else "candidate"
        if protocol.get("status") != expected_status:
            die(f"aggregate protocol status mismatch for {param['id']}: {protocol.get('status')} != {expected_status}")

    print(f"PASS: {len(nrpn_numbers)} candidate NRPN definitions")
    print(f"PASS: {len(sysex_ids)} candidate SysEx fields")
    print(f"PASS: {mapped_nrpn}/{len(params['parameters'])} UI parameters have candidate NRPN mappings")
    print(f"PASS: {mapped_sysex}/{len(params['parameters'])} UI parameters have candidate SysEx mappings")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
