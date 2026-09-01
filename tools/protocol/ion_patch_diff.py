#!/usr/bin/env python3
"""Diff two Ion single-patch SysEx dumps for hardware verification.

This is intentionally a research tool: raw decoded byte differences are the
primary evidence, while field names come from the candidate JSON specification.
It can read raw .syx files, whitespace-separated hex files, or AIM Editor MIDI
capture JSON and emits a fully human-readable JSON report.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Any

from ion_sysex import decode_patch_sysex, load_spec, read_sysex


def _wire_from_capture(path: Path) -> bytes:
    doc = json.loads(path.read_text())

    if doc.get("format") == "aim-editor.midi-capture":
        for event in reversed(doc.get("events", [])):
            if event.get("kind") != "sysex":
                continue
            raw = event.get("bytes")
            if isinstance(raw, list) and len(raw) == 434 and raw[:1] == [0xF0] and raw[-1:] == [0xF7]:
                return bytes(raw)
        raise ValueError(f"{path}: no 434-byte SysEx event found in capture")

    wire_hex = doc.get("wire_hex")
    if isinstance(wire_hex, str):
        return bytes.fromhex(wire_hex)

    raise ValueError(f"{path}: unsupported JSON input format")


def read_patch_wire(path: Path) -> bytes:
    if path.suffix.lower() == ".json":
        return _wire_from_capture(path)
    return read_sysex(path)


def field_range(field: dict[str, Any]) -> range:
    return range(field["offset"], field["offset"] + field["width_bytes"])


def build_diff(before_path: Path, after_path: Path, expected: str | None = None) -> dict[str, Any]:
    spec = load_spec()
    before = decode_patch_sysex(read_patch_wire(before_path), spec)
    after = decode_patch_sysex(read_patch_wire(after_path), spec)

    before_bytes = before["decoded_bytes"]
    after_bytes = after["decoded_bytes"]
    changed_offsets = [i for i, (a, b) in enumerate(zip(before_bytes, after_bytes)) if a != b]

    byte_changes = [
        {
            "offset": offset,
            "before": before_bytes[offset],
            "after": after_bytes[offset],
            "before_hex": f"{before_bytes[offset]:02X}",
            "after_hex": f"{after_bytes[offset]:02X}",
            "xor_hex": f"{before_bytes[offset] ^ after_bytes[offset]:02X}",
        }
        for offset in changed_offsets
    ]

    field_changes: list[dict[str, Any]] = []
    covered_changed_offsets: set[int] = set()
    for field in spec["fields"]:
        field_id = field["id"]
        before_item = before["fields"][field_id]
        after_item = after["fields"][field_id]
        if before_item["raw"] == after_item["raw"]:
            continue

        offsets = sorted(set(field_range(field)).intersection(changed_offsets))
        covered_changed_offsets.update(offsets)
        field_changes.append(
            {
                "field_id": field_id,
                "offset": field["offset"],
                "width_bytes": field["width_bytes"],
                "kind": field["kind"],
                "mask": field.get("mask"),
                "shift": field.get("shift"),
                "before": before_item.get("raw"),
                "after": after_item.get("raw"),
                "before_enum": before_item.get("enum"),
                "after_enum": after_item.get("enum"),
                "changed_byte_offsets": offsets,
                "status": field.get("status", "candidate"),
            }
        )

    unmapped_offsets = [offset for offset in changed_offsets if offset not in covered_changed_offsets]

    changed_semantic_field_ids = [item["field_id"] for item in field_changes]
    expected_observed = expected in changed_semantic_field_ids if expected else None

    if len(field_changes) == 1 and not unmapped_offsets:
        confidence_hint = "single_candidate_field_changed"
    elif len(field_changes) == 0:
        confidence_hint = "raw_change_not_explained_by_candidate_fields"
    else:
        confidence_hint = "multiple_candidate_fields_or_unmapped_bytes_changed"

    return {
        "$schema": "../../schemas/patch-diff.schema.json",
        "format": "aim-editor.ion-patch-diff",
        "schema_version": 1,
        "research_status": "candidate",
        "inputs": {
            "before": str(before_path),
            "after": str(after_path),
        },
        "patch_identity": {
            "before_name": before["name"],
            "after_name": after["name"],
            "before_bank": before["bank"],
            "after_bank": after["bank"],
            "before_slot": before["slot"],
            "after_slot": after["slot"],
            "before_checksum_valid": before["checksum"]["valid"],
            "after_checksum_valid": after["checksum"]["valid"],
        },
        "expected_parameter_or_field": expected,
        "expected_observed": expected_observed,
        "summary": {
            "changed_decoded_byte_count": len(changed_offsets),
            "changed_candidate_field_count": len(field_changes),
            "unmapped_changed_byte_count": len(unmapped_offsets),
            "confidence_hint": confidence_hint,
        },
        "byte_changes": byte_changes,
        "candidate_field_changes": field_changes,
        "unmapped_changed_byte_offsets": unmapped_offsets,
    }


def self_test() -> None:
    # Re-use the protocol module's synthetic patch construction by generating
    # two structurally valid images here. This keeps the test copyright-free.
    import struct
    from ion_sysex import checksum_complement, encode_7of8
    import tempfile

    def make(raw_filter: int) -> bytes:
        decoded = bytearray(378)
        decoded[0:3] = bytes((0, 0x0E, 0x22))
        decoded[3:7] = bytes((1, 3, 0, 7))
        decoded[7:15] = b"Q01SYNTH"
        decoded[19:23] = b"1.06"
        decoded[51:55] = struct.pack(">I", 315)
        decoded[63:72] = b"DiffTest\x00"
        # filter1.frequency is candidate s16be at offsets 126..127.
        decoded[126:128] = int(raw_filter).to_bytes(2, "big", signed=True)
        decoded[15:19] = struct.pack(">I", checksum_complement(decoded))
        return b"\xF0" + encode_7of8(decoded) + b"\xF7"

    with tempfile.TemporaryDirectory() as td:
        before = Path(td) / "before.syx"
        after = Path(td) / "after.syx"
        before.write_bytes(make(100))
        after.write_bytes(make(101))
        report = build_diff(before, after, "filter1.frequency")
        assert report["expected_observed"] is True
        assert any(item["field_id"] == "filter1.frequency" for item in report["candidate_field_changes"])
    print("PASS: Ion patch-diff self-test")


def main() -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)

    diff = sub.add_parser("diff", help="compare two single-patch dumps/captures")
    diff.add_argument("before", type=Path)
    diff.add_argument("after", type=Path)
    diff.add_argument("--expected", help="expected candidate field/parameter ID")
    diff.add_argument("--output", "-o", type=Path)

    sub.add_parser("self-test")
    args = parser.parse_args()

    if args.command == "self-test":
        self_test()
        return 0

    report = build_diff(args.before, args.after, args.expected)
    text = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
