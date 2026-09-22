#!/usr/bin/env python3
"""Summarize AIM Editor protocol-verification coverage.

The report is derived from the canonical parameter database plus sealed evidence
records under research/verification. It never changes mapping status; promotion
remains the job of ion_protocol_verification.py apply.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import sys
from typing import Any

from ion_protocol_verification import verify_seal

ROOT = Path(__file__).resolve().parents[2]
PARAMETERS = ROOT / "data" / "parameters.json"
EVIDENCE_DIR = ROOT / "research" / "verification"


def load(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path}: JSON root must be an object")
    return value


def transport_status(item: dict[str, Any], protocol: str) -> str:
    proto = item.get("protocol") or {}
    if protocol == "nrpn":
        evidence = proto.get("nrpn_evidence") or {}
    else:
        evidence = proto.get("evidence") or {}
    status = evidence.get("status") or proto.get("status") or "unmapped"
    return str(status)


def mapped(item: dict[str, Any], protocol: str) -> bool:
    proto = item.get("protocol") or {}
    if protocol == "nrpn":
        return isinstance(proto.get("nrpn"), int) and not isinstance(proto.get("nrpn"), bool)
    sysex = proto.get("sysex") or {}
    return isinstance(sysex, dict) and (isinstance(sysex.get("offset"), int) or bool(sysex.get("field_id")))


def evidence_records() -> list[tuple[Path, dict[str, Any]]]:
    records: list[tuple[Path, dict[str, Any]]] = []
    if not EVIDENCE_DIR.exists():
        return records
    for path in sorted(EVIDENCE_DIR.glob("*.json")):
        data = load(path)
        if data.get("format") == "aim-editor.protocol-verification":
            records.append((path, data))
    return records


def build_report() -> dict[str, Any]:
    db = load(PARAMETERS)
    parameters = list(db.get("parameters") or [])
    if not all(isinstance(item, dict) for item in parameters):
        raise ValueError("parameter database contains a non-object entry")

    protocol_summary: dict[str, Any] = {}
    for protocol in ("nrpn", "sysex"):
        statuses = Counter(transport_status(item, protocol) for item in parameters)
        mapped_ids = [str(item.get("id")) for item in parameters if mapped(item, protocol)]
        verified_ids = [str(item.get("id")) for item in parameters
                        if mapped(item, protocol) and transport_status(item, protocol) == "verified"]
        protocol_summary[protocol] = {
            "mapped": len(mapped_ids),
            "verified": len(verified_ids),
            "candidate": sum(1 for item in parameters
                             if mapped(item, protocol) and transport_status(item, protocol) == "candidate"),
            "unmapped": len(parameters) - len(mapped_ids),
            "status_counts": dict(sorted(statuses.items())),
            "verified_parameter_ids": verified_ids,
        }

    records = evidence_records()
    result_counts = Counter(str(data.get("result", "unknown")) for _, data in records)
    protocol_counts = Counter(str(data.get("protocol", "unknown")) for _, data in records)
    invalid_seals = [str(path.relative_to(ROOT)) for path, data in records if not verify_seal(data)]
    promotable = [str(path.relative_to(ROOT)) for path, data in records if data.get("promotable") is True]
    contradicted = [
        {
            "path": str(path.relative_to(ROOT)),
            "parameter_id": data.get("parameter_id"),
            "protocol": data.get("protocol"),
        }
        for path, data in records if data.get("result") == "contradicted"
    ]

    return {
        "format": "aim-editor.verification-report",
        "schema_version": 1,
        "parameter_count": len(parameters),
        "protocols": protocol_summary,
        "evidence": {
            "record_count": len(records),
            "result_counts": dict(sorted(result_counts.items())),
            "protocol_counts": dict(sorted(protocol_counts.items())),
            "promotable_record_paths": promotable,
            "invalid_seal_paths": invalid_seals,
            "contradictions": contradicted,
        },
    }


def as_markdown(report: dict[str, Any]) -> str:
    nrpn = report["protocols"]["nrpn"]
    sysex = report["protocols"]["sysex"]
    evidence = report["evidence"]
    lines = [
        "# AIM Editor protocol verification report",
        "",
        f"Semantic parameters: **{report['parameter_count']}**",
        "",
        "| Transport | Mapped | Candidate | Verified | Unmapped |",
        "| --- | ---: | ---: | ---: | ---: |",
        f"| NRPN | {nrpn['mapped']} | {nrpn['candidate']} | {nrpn['verified']} | {nrpn['unmapped']} |",
        f"| SysEx | {sysex['mapped']} | {sysex['candidate']} | {sysex['verified']} | {sysex['unmapped']} |",
        "",
        f"Sealed evidence records: **{evidence['record_count']}**",
    ]
    if evidence["result_counts"]:
        lines.append("Results: " + ", ".join(f"{k}={v}" for k, v in evidence["result_counts"].items()))
    else:
        lines.append("Results: no hardware-verification records committed yet.")
    lines.append("")

    if evidence["invalid_seal_paths"]:
        lines += ["## Invalid evidence seals", ""] + [f"- `{p}`" for p in evidence["invalid_seal_paths"]] + [""]
    if evidence["contradictions"]:
        lines += ["## Contradictions requiring review", ""]
        lines += [f"- `{item['parameter_id']}` ({item['protocol']}): `{item['path']}`" for item in evidence["contradictions"]]
        lines.append("")
    if nrpn["verified_parameter_ids"] or sysex["verified_parameter_ids"]:
        lines += ["## Verified mappings", ""]
        if nrpn["verified_parameter_ids"]:
            lines.append("NRPN: " + ", ".join(f"`{x}`" for x in nrpn["verified_parameter_ids"]))
        if sysex["verified_parameter_ids"]:
            lines.append("SysEx: " + ", ".join(f"`{x}`" for x in sysex["verified_parameter_ids"]))
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def self_test() -> int:
    report = build_report()
    if report["parameter_count"] != 217:
        raise AssertionError(f"expected 217 parameters, got {report['parameter_count']}")
    if report["protocols"]["nrpn"]["mapped"] != 199:
        raise AssertionError("candidate NRPN coverage changed unexpectedly")
    if report["protocols"]["sysex"]["mapped"] != 190:
        raise AssertionError("candidate SysEx coverage changed unexpectedly")
    if report["evidence"]["invalid_seal_paths"]:
        raise AssertionError("committed verification evidence has invalid seals")
    print("PASS: protocol verification report self-test")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", nargs="?", choices=("report", "self-test"), default="report")
    parser.add_argument("--format", choices=("markdown", "json"), default="markdown")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    if args.command == "self-test":
        return self_test()

    report = build_report()
    text = (json.dumps(report, indent=2, sort_keys=True) + "\n") if args.format == "json" else as_markdown(report)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
