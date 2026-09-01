#!/usr/bin/env python3
"""Research utility for candidate Alesis Ion/Micron NRPN messages.

The mapping is intentionally loaded from data/protocol/ion-nrpn.json so the
human-readable data remains the source of truth.  This tool does not imply
hardware verification.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SPEC_PATH = ROOT / "data" / "protocol" / "ion-nrpn.json"


def load_spec() -> dict:
    return json.loads(SPEC_PATH.read_text())


def encode_signed14(value: int) -> int:
    if not -8192 <= value <= 8191:
        raise ValueError("signed Ion NRPN value must fit -8192..8191")
    return value if value >= 0 else 16384 + value


def decode_signed14(value: int) -> int:
    if not 0 <= value <= 16383:
        raise ValueError("encoded NRPN value must fit 0..16383")
    return value - 16384 if value >= 8192 else value


def cc_sequence(channel: int, nrpn: int, value: int, *, signed: bool) -> list[dict]:
    if not 1 <= channel <= 16:
        raise ValueError("MIDI channel must be 1..16")
    if not 0 <= nrpn <= 16383:
        raise ValueError("NRPN number must be 0..16383")
    encoded = encode_signed14(value) if signed else value
    if not 0 <= encoded <= 16383:
        raise ValueError("NRPN value must fit 0..16383")

    pairs = [
        (99, (nrpn >> 7) & 0x7F),
        (98, nrpn & 0x7F),
        (6, (encoded >> 7) & 0x7F),
        (38, encoded & 0x7F),
    ]
    status = 0xB0 | (channel - 1)
    return [{"cc": cc, "value": val, "bytes": [status, cc, val]} for cc, val in pairs]


def find_parameter(spec: dict, token: str) -> dict:
    try:
        number = int(token, 0)
    except ValueError:
        number = None
    for entry in spec["parameters"]:
        if entry["id"] == token or (number is not None and entry["number"] == number):
            return entry
    raise KeyError(f"unknown candidate Ion NRPN parameter: {token}")


def command_show(args: argparse.Namespace) -> int:
    spec = load_spec()
    entry = find_parameter(spec, args.parameter)
    print(json.dumps(entry, indent=2))
    return 0


def command_encode(args: argparse.Namespace) -> int:
    spec = load_spec()
    entry = find_parameter(spec, args.parameter)
    signed = entry["value_encoding"] == "signed_14_wrap"
    if args.value < entry["min"] or args.value > entry["max"]:
        raise SystemExit(f"value outside candidate range {entry['min']}..{entry['max']}")
    messages = cc_sequence(args.channel, entry["number"], args.value, signed=signed)
    result = {
        "format": "aim-editor.ion-nrpn-message",
        "schema_version": 1,
        "status": "candidate",
        "hardware_verified": False,
        "parameter": entry,
        "channel": args.channel,
        "semantic_value": args.value,
        "encoded_value14": encode_signed14(args.value) if signed else args.value,
        "messages": messages,
        "hex": [" ".join(f"{b:02X}" for b in m["bytes"]) for m in messages],
    }
    print(json.dumps(result, indent=2))
    return 0


def command_self_test(_: argparse.Namespace) -> int:
    spec = load_spec()
    assert len(spec["parameters"]) >= 200
    assert encode_signed14(-100) == 16284
    assert decode_signed14(16284) == -100
    f1 = find_parameter(spec, "filter1.frequency")
    assert f1["number"] == 44
    seq = cc_sequence(1, 47, -100, signed=True)
    assert [x["bytes"] for x in seq] == [
        [0xB0, 99, 0], [0xB0, 98, 47], [0xB0, 6, 127], [0xB0, 38, 28]
    ]
    print("PASS: candidate Ion NRPN self-test")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("show", help="show one candidate mapping as JSON")
    p.add_argument("parameter", help="parameter id or NRPN number")
    p.set_defaults(func=command_show)

    p = sub.add_parser("encode", help="emit a four-CC candidate NRPN write as JSON")
    p.add_argument("parameter", help="parameter id or NRPN number")
    p.add_argument("value", type=int)
    p.add_argument("--channel", type=int, default=1)
    p.set_defaults(func=command_encode)

    p = sub.add_parser("self-test")
    p.set_defaults(func=command_self_test)

    args = parser.parse_args()
    try:
        return args.func(args)
    except (KeyError, ValueError) as exc:
        parser.error(str(exc))
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
