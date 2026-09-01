#!/usr/bin/env python3
"""AIM Editor candidate Alesis Ion/Micron patch SysEx tooling.

The wire format implemented here is based on the community-reverse-engineered
2008 BEE document recorded in data/protocol/ion-sysex.json. It is deliberately
labelled candidate until confirmed against independent Ion hardware captures.

This tool is stdlib-only so protocol research does not require building JUCE.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import sys
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SPEC = ROOT / "data" / "protocol" / "ion-sysex.json"

BANKS = {"red": 0, "green": 1, "blue": 2, "yellow": 3, "user": 3, "edit": 4}


def load_spec(path: Path = DEFAULT_SPEC) -> dict[str, Any]:
    return json.loads(path.read_text())


def encode_7of8(decoded: bytes) -> bytes:
    if not decoded:
        return b""
    out = bytearray()
    for start in range(0, len(decoded), 7):
        group = bytearray(decoded[start:start + 7])
        group.extend(b"\x00" * (7 - len(group)))
        msbs = 0
        for i, value in enumerate(group):
            if value & 0x80:
                msbs |= 1 << (6 - i)
        out.append(msbs)
        out.extend(value & 0x7F for value in group)
    return bytes(out)


def decode_7of8(encoded: bytes) -> bytes:
    if len(encoded) % 8:
        raise ValueError("7-of-8 payload length must be a multiple of 8")
    if any(value & 0x80 for value in encoded):
        raise ValueError("7-of-8 transport data contains a MIDI status bit")
    out = bytearray()
    for start in range(0, len(encoded), 8):
        msbs = encoded[start]
        for i in range(7):
            out.append(encoded[start + 1 + i] | (((msbs >> (6 - i)) & 1) << 7))
    return bytes(out)


def make_patch_request(bank: int, slot: int, multiple: bool = False) -> bytes:
    if not 0 <= bank <= 4:
        raise ValueError("bank must be 0..4")
    max_slot = 3 if bank == 4 else 127
    if not multiple and not 0 <= slot <= max_slot:
        raise ValueError(f"slot must be 0..{max_slot} for this bank")
    slot = 0 if multiple else slot
    return bytes((0xF0, 0x00, 0x00, 0x0E, 0x22, 0x41, bank, int(multiple), slot, 0xF7))


def read_u32be(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset:offset + 4], "big", signed=False)


def checksum_complement(decoded: bytes) -> int:
    if len(decoded) < 375:
        raise ValueError("decoded patch is too short for checksum")
    total = 0
    for offset in range(63, 375, 4):
        total = (total + read_u32be(decoded, offset)) & 0xFFFFFFFF
    return (-total) & 0xFFFFFFFF


def verify_checksum(decoded: bytes) -> bool:
    return len(decoded) >= 19 and read_u32be(decoded, 15) == checksum_complement(decoded)


def encode_patch_sysex(decoded_input: bytes) -> bytes:
    """Repack one 378-byte candidate patch image, recalculating checksum.

    The caller supplies the complete decoded image so unknown bytes are
    preserved. AIM Editor never constructs a blank hardware patch from partial
    semantic knowledge.
    """
    if len(decoded_input) != 378:
        raise ValueError(f"decoded patch must be 378 bytes, got {len(decoded_input)}")
    if decoded_input[0:3] != bytes((0x00, 0x0E, 0x22)):
        raise ValueError("decoded manufacturer/product bytes do not match candidate Ion format")
    if decoded_input[7:15] != b"Q01SYNTH":
        raise ValueError("decoded patch lacks Q01SYNTH tag")
    if read_u32be(decoded_input, 51) != 315:
        raise ValueError("decoded patch-size field is not 315")

    decoded = bytearray(decoded_input)
    decoded[15:19] = struct.pack(">I", checksum_complement(decoded))
    encoded = encode_7of8(decoded)
    if len(encoded) != 432:
        raise ValueError(f"encoded patch payload must be 432 bytes, got {len(encoded)}")
    return b"\xF0" + encoded + b"\xF7"


def _decode_scalar(field: dict[str, Any], data: bytes) -> Any:
    off = field["offset"]
    width = field["width_bytes"]
    kind = field["kind"]
    raw = data[off:off + width]
    if len(raw) != width:
        raise ValueError(f"field {field['id']} extends beyond decoded data")

    if kind == "u8":
        value: Any = raw[0]
    elif kind == "s8":
        value = int.from_bytes(raw, "big", signed=True)
    elif kind == "s16be":
        value = int.from_bytes(raw, "big", signed=True)
    elif kind == "u32be":
        value = int.from_bytes(raw, "big", signed=False)
    elif kind == "bitfield":
        value = (raw[0] & field["mask"]) >> field["shift"]
    elif kind == "ascii":
        value = raw.split(b"\x00", 1)[0].decode("ascii", errors="replace")
    elif kind in {"bytes", "opaque"}:
        value = raw.hex(" ").upper()
    else:
        raise ValueError(f"unsupported field kind {kind}")
    return value


def decode_fields(decoded: bytes, spec: dict[str, Any]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for field in spec["fields"]:
        value = _decode_scalar(field, decoded)
        item: dict[str, Any] = {"raw": value, "offset": field["offset"], "kind": field["kind"]}
        if field.get("mask") is not None:
            item["mask"] = field["mask"]
            item["shift"] = field["shift"]
        enum = field.get("enum")
        if enum is not None and str(value) in enum:
            item["enum"] = enum[str(value)]
        if field.get("scale") is not None and isinstance(value, (int, float)):
            item["scaled"] = value * field["scale"]
        if field.get("unit") is not None:
            item["unit"] = field["unit"]
        if field.get("display_transform") is not None:
            item["display_transform"] = field["display_transform"]
        result[field["id"]] = item
    return result


def decode_patch_sysex(wire: bytes, spec: dict[str, Any] | None = None) -> dict[str, Any]:
    spec = spec or load_spec()
    if len(wire) != 434:
        raise ValueError(f"single patch dump must be 434 wire bytes, got {len(wire)}")
    if wire[0] != 0xF0 or wire[-1] != 0xF7:
        raise ValueError("not a complete SysEx message")
    decoded = decode_7of8(wire[1:-1])
    if len(decoded) != 378:
        raise ValueError(f"decoded patch must be 378 bytes, got {len(decoded)}")
    if decoded[0:3] != bytes((0x00, 0x0E, 0x22)):
        raise ValueError("decoded manufacturer/product bytes do not match candidate Ion format")
    if decoded[7:15] != b"Q01SYNTH":
        raise ValueError("decoded patch lacks Q01SYNTH tag")
    if read_u32be(decoded, 51) != 315:
        raise ValueError("decoded patch-size field is not 315")

    return {
        "format": "aim-editor.decoded-ion-patch-research",
        "schema_version": 1,
        "research_status": "candidate",
        "bank": decoded[4],
        "multiple": bool(decoded[5]),
        "slot": decoded[6],
        "name": decoded[63:78].split(b"\x00", 1)[0].decode("ascii", errors="replace"),
        "firmware_version": decoded[19:23].decode("ascii", errors="replace"),
        "checksum": {
            "stored": read_u32be(decoded, 15),
            "expected": checksum_complement(decoded),
            "valid": verify_checksum(decoded),
        },
        "fields": decode_fields(decoded, spec),
        "decoded_bytes": list(decoded),
        "decoded_hex": decoded.hex(" ").upper(),
        "wire_hex": wire.hex(" ").upper(),
    }


def _parse_hex_file(path: Path) -> bytes:
    text = path.read_text().strip()
    cleaned = text.replace(",", " ").replace("0x", "").replace("0X", "")
    return bytes(int(token, 16) for token in cleaned.split())


def read_sysex(path: Path) -> bytes:
    raw = path.read_bytes()
    if raw.startswith(b"\xF0"):
        return raw
    try:
        return _parse_hex_file(path)
    except (UnicodeDecodeError, ValueError):
        return raw


def self_test() -> None:
    known = bytes((0x80, 0x01, 0x82, 0x03, 0x84, 0x05, 0x86))
    packed = encode_7of8(known)
    assert packed == bytes((0x55, 0, 1, 2, 3, 4, 5, 6))
    assert decode_7of8(packed) == known

    all_values = bytes(range(256))
    decoded = decode_7of8(encode_7of8(all_values))
    assert decoded[:256] == all_values
    assert set(decoded[256:]) <= {0}

    synthetic = bytearray(378)
    synthetic[0:3] = bytes((0, 0x0E, 0x22))
    synthetic[3:7] = bytes((1, 3, 0, 7))
    synthetic[7:15] = b"Q01SYNTH"
    synthetic[19:23] = b"1.06"
    synthetic[51:55] = struct.pack(">I", 315)
    synthetic[63:72] = b"AIM Test\x00"
    synthetic[108] = 73
    synthetic[126:128] = bytes((2, 0))
    synthetic[15:19] = struct.pack(">I", checksum_complement(synthetic))
    wire = encode_patch_sysex(synthetic)
    parsed = decode_patch_sysex(wire)
    assert encode_patch_sysex(bytes(parsed["decoded_bytes"])) == wire
    assert parsed["name"] == "AIM Test"
    assert parsed["checksum"]["valid"] is True
    assert parsed["fields"]["filter1.frequency"]["raw"] == 512
    print("PASS: candidate Ion SysEx self-test")


def main() -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)

    req = sub.add_parser("request", help="print candidate patch-request SysEx")
    req.add_argument("--bank", choices=sorted(BANKS), required=True)
    req.add_argument("--slot", type=int, default=0)
    req.add_argument("--bank-request", action="store_true")

    dec = sub.add_parser("decode", help="decode a 434-byte single-patch .syx to JSON")
    dec.add_argument("input", type=Path)
    dec.add_argument("--output", "-o", type=Path)
    dec.add_argument("--spec", type=Path, default=DEFAULT_SPEC)

    repack = sub.add_parser("repack", help="decode and losslessly repack one candidate single-patch .syx")
    repack.add_argument("input", type=Path)
    repack.add_argument("output", type=Path)

    sub.add_parser("self-test", help="run protocol codec self-tests")

    args = parser.parse_args()
    if args.command == "request":
        wire = make_patch_request(BANKS[args.bank], args.slot, args.bank_request)
        print(wire.hex(" ").upper())
        return 0
    if args.command == "decode":
        doc = decode_patch_sysex(read_sysex(args.input), load_spec(args.spec))
        text = json.dumps(doc, indent=2) + "\n"
        if args.output:
            args.output.write_text(text)
        else:
            sys.stdout.write(text)
        return 0
    if args.command == "repack":
        parsed = decode_patch_sysex(read_sysex(args.input))
        args.output.write_bytes(encode_patch_sysex(bytes(parsed["decoded_bytes"])))
        return 0
    if args.command == "self-test":
        self_test()
        return 0
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
