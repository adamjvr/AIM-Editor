#!/usr/bin/env python3
"""Decode complete candidate Ion NRPN transactions from AIM MIDI capture JSON."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
SPEC_PATH = ROOT / "data" / "protocol" / "ion-nrpn.json"


def load_index() -> dict[int, dict[str, Any]]:
    spec = json.loads(SPEC_PATH.read_text())
    return {int(entry["number"]): entry for entry in spec["parameters"]}


def decode_signed14(value: int) -> int:
    return value - 16384 if value >= 8192 else value


def analyze_capture(capture: dict[str, Any], direction: str = "all") -> dict[str, Any]:
    if capture.get("format") != "aim-editor.midi-capture":
        raise ValueError("input is not aim-editor.midi-capture JSON")

    index = load_index()
    state: dict[tuple[str, int], dict[str, int]] = {}
    decoded: list[dict[str, Any]] = []

    for event_index, event in enumerate(capture.get("events", [])):
        event_direction = event.get("direction")
        if direction != "all" and event_direction != direction:
            continue
        if event.get("kind") != "control_change":
            continue

        raw = event.get("bytes", [])
        if len(raw) < 3 or (int(raw[0]) & 0xF0) != 0xB0:
            continue

        channel = (int(raw[0]) & 0x0F) + 1
        cc = int(raw[1]) & 0x7F
        value = int(raw[2]) & 0x7F
        key = (str(event_direction), channel)
        current = state.setdefault(key, {"msb": -1, "lsb": -1, "data_msb": -1})

        if cc == 99:
            current.update(msb=value, lsb=-1, data_msb=-1)
        elif cc == 98:
            current["lsb"] = value
            current["data_msb"] = -1
            if current["msb"] == 127 and current["lsb"] == 127:
                current.update(msb=-1, lsb=-1, data_msb=-1)
        elif cc == 6 and current["msb"] >= 0 and current["lsb"] >= 0:
            current["data_msb"] = value
        elif cc == 38 and current["msb"] >= 0 and current["lsb"] >= 0 and current["data_msb"] >= 0:
            nrpn = (current["msb"] << 7) | current["lsb"]
            value14 = (current["data_msb"] << 7) | value
            mapping = index.get(nrpn)
            semantic = value14
            if mapping and mapping.get("value_encoding") == "signed_14_wrap":
                semantic = decode_signed14(value14)

            item: dict[str, Any] = {
                "event_index": event_index,
                "utc_ms": event.get("utc_ms"),
                "direction": event_direction,
                "device_identifier": event.get("device_identifier", ""),
                "channel": channel,
                "nrpn": nrpn,
                "encoded_value14": value14,
                "semantic_value": semantic,
                "mapping_status": "candidate" if mapping else "unknown",
            }
            if mapping:
                item["parameter"] = {
                    "id": mapping.get("id"),
                    "name": mapping.get("name"),
                    "category": mapping.get("category"),
                    "value_encoding": mapping.get("value_encoding"),
                    "candidate_min": mapping.get("min"),
                    "candidate_max": mapping.get("max"),
                }
                minimum = mapping.get("min")
                maximum = mapping.get("max")
                item["within_candidate_range"] = (
                    (minimum is None or semantic >= minimum)
                    and (maximum is None or semantic <= maximum)
                )
            decoded.append(item)

    result = {
        "format": "aim-editor.nrpn-capture-analysis",
        "schema_version": 1,
        "source_format": capture.get("format"),
        "direction_filter": direction,
        "transaction_count": len(decoded),
        "transactions": decoded,
    }
    result["embedded_cross_check"] = cross_check_embedded(capture, result)
    return result



def cross_check_embedded(capture: dict[str, Any], analysis: dict[str, Any]) -> dict[str, Any]:
    embedded = capture.get("nrpn_transactions")
    if not isinstance(embedded, list):
        return {
            "present": False,
            "match": None,
            "embedded_count": 0,
            "reconstructed_count": int(analysis.get("transaction_count", 0)),
            "discrepancies": [],
        }

    reconstructed = analysis.get("transactions", [])
    discrepancies: list[dict[str, Any]] = []
    if len(embedded) != len(reconstructed):
        discrepancies.append({
            "kind": "count_mismatch",
            "embedded": len(embedded),
            "reconstructed": len(reconstructed),
        })

    for index, (app_item, offline_item) in enumerate(zip(embedded, reconstructed)):
        expected = {
            "direction": offline_item.get("direction"),
            "utc_ms": offline_item.get("utc_ms"),
            "device_identifier": offline_item.get("device_identifier", ""),
            "midi_channel": offline_item.get("channel"),
            "nrpn": offline_item.get("nrpn"),
            "value14": offline_item.get("encoded_value14"),
            "semantic_value": offline_item.get("semantic_value"),
            "parameter_id": (offline_item.get("parameter") or {}).get("id"),
        }
        for field, value in expected.items():
            if app_item.get(field) != value:
                discrepancies.append({
                    "kind": "field_mismatch",
                    "transaction_index": index,
                    "field": field,
                    "embedded": app_item.get(field),
                    "reconstructed": value,
                })

    return {
        "present": True,
        "match": not discrepancies,
        "embedded_count": len(embedded),
        "reconstructed_count": len(reconstructed),
        "discrepancies": discrepancies,
    }


def command_analyze(args: argparse.Namespace) -> int:
    capture = json.loads(Path(args.capture).read_text())
    result = analyze_capture(capture, args.direction)
    text = json.dumps(result, indent=2) + "\n"
    if args.output:
        Path(args.output).write_text(text)
    else:
        print(text, end="")
    return 0


def command_self_test(_: argparse.Namespace) -> int:
    # NRPN 47, semantic -100 -> encoded 16284 -> 127/28.
    capture = {
        "format": "aim-editor.midi-capture",
        "schema_version": 1,
        "exported_at_utc_ms": 0,
        "events": [
            {"direction": "input", "utc_ms": i, "device_identifier": "test", "kind": "control_change",
             "size": 3, "hex": "", "bytes": b}
            for i, b in enumerate([
                [0xB0, 99, 0], [0xB0, 98, 47], [0xB0, 6, 127], [0xB0, 38, 28]
            ])
        ],
        "nrpn_transactions": [{
            "direction": "input",
            "utc_ms": 3,
            "device_identifier": "test",
            "midi_channel": 1,
            "nrpn": 47,
            "value14": 16284,
            "semantic_value": -100,
            "parameter_id": "filter1.env_amount",
            "parameter_name": "Filter 1 Env Amount",
            "mapping_status": "candidate",
            "value_encoding": "signed_14_wrap",
        }],
    }
    result = analyze_capture(capture, "input")
    assert result["transaction_count"] == 1
    item = result["transactions"][0]
    assert item["nrpn"] == 47
    assert item["encoded_value14"] == 16284
    assert item["semantic_value"] == -100
    assert item["parameter"]["id"] == "filter1.env_amount"
    assert result["embedded_cross_check"]["present"] is True
    assert result["embedded_cross_check"]["match"] is True
    print("PASS: Ion NRPN capture analyzer self-test")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("analyze", help="decode NRPN transactions from a MIDI capture")
    p.add_argument("capture", help="aim-editor.midi-capture JSON")
    p.add_argument("--direction", choices=["all", "input", "output"], default="all")
    p.add_argument("--output", help="write analysis JSON instead of stdout")
    p.set_defaults(func=command_analyze)

    p = sub.add_parser("self-test")
    p.set_defaults(func=command_self_test)

    args = parser.parse_args()
    try:
        return args.func(args)
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        parser.error(str(exc))
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
