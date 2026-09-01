#!/usr/bin/env python3
"""Inspect the container appended to the legacy Windows AIM/Ion editor.

This tool intentionally knows nothing about the serialized SynthMaker graph itself.
It only parses the PE boundary and the reverse-readable OSSM footer discovered during
clean-room triage. Output is JSON so findings can be diffed and archived.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path
from typing import Any


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def pe_overlay_start(data: bytes) -> int:
    if data[:2] != b"MZ":
        raise ValueError("not an MZ executable")

    pe_offset = u32(data, 0x3C)
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise ValueError("PE signature not found")

    coff = pe_offset + 4
    section_count = u16(data, coff + 2)
    optional_header_size = u16(data, coff + 16)
    section_table = coff + 20 + optional_header_size

    raw_end = 0
    sections: list[dict[str, Any]] = []
    for index in range(section_count):
        off = section_table + index * 40
        name = data[off : off + 8].split(b"\0", 1)[0].decode("ascii", "replace")
        raw_size = u32(data, off + 16)
        raw_offset = u32(data, off + 20)
        raw_end = max(raw_end, raw_offset + raw_size)
        sections.append({"name": name, "raw_offset": raw_offset, "raw_size": raw_size})

    return raw_end


def parse_ossm_footer(data: bytes, overlay_start: int) -> dict[str, Any]:
    if len(data) < 12 or data[-4:] != b"OSSM":
        raise ValueError("OSSM footer magic not found at EOF")

    entry_count = u32(data, len(data) - 8)
    if entry_count > 1024:
        raise ValueError(f"implausible OSSM entry count: {entry_count}")

    cursor = len(data) - 8
    entries: list[dict[str, Any]] = []

    for _ in range(entry_count):
        if cursor < 4:
            raise ValueError("truncated OSSM directory")

        name_length = u32(data, cursor - 4)
        cursor -= 4
        if name_length > cursor or name_length > 4096:
            raise ValueError(f"implausible OSSM name length: {name_length}")

        cursor -= name_length
        name_bytes = data[cursor : cursor + name_length]
        name = name_bytes.decode("utf-8", "replace")

        if cursor < 8:
            raise ValueError("truncated OSSM entry metadata")
        cursor -= 8
        absolute_offset, size = struct.unpack_from("<II", data, cursor)

        entry = {
            "name": name,
            "name_length": name_length,
            "absolute_file_offset": absolute_offset,
            "size": size,
            "end_file_offset": absolute_offset + size,
            "starts_at_pe_overlay": absolute_offset == overlay_start,
            "ends_at_directory": absolute_offset + size == cursor,
        }
        entries.append(entry)

    entries.reverse()
    return {
        "magic": "OSSM",
        "entry_count": entry_count,
        "directory_file_offset": cursor,
        "directory_size": len(data) - cursor,
        "entries": entries,
    }


def inspect(path: Path) -> dict[str, Any]:
    data = path.read_bytes()
    overlay_start = pe_overlay_start(data)
    footer = parse_ossm_footer(data, overlay_start)

    return {
        "format": "aim-editor.legacy-container-inspection",
        "schema_version": 1,
        "file": {
            "name": path.name,
            "size": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
        },
        "pe": {
            "overlay_start": overlay_start,
            "overlay_size": len(data) - overlay_start,
        },
        "ossm_footer": footer,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--extract-dir", type=Path,
                        help="optionally extract OSSM payloads from your local reference executable")
    args = parser.parse_args()

    result = inspect(args.executable)

    if args.extract_dir:
        data = args.executable.read_bytes()
        args.extract_dir.mkdir(parents=True, exist_ok=True)
        extracted = []
        for entry in result["ossm_footer"]["entries"]:
            start = entry["absolute_file_offset"]
            end = entry["end_file_offset"]
            if start < 0 or end > len(data) or end < start:
                raise ValueError(f"invalid OSSM entry bounds for {entry['name']}")
            safe_name = Path(entry["name"]).name
            destination = args.extract_dir / safe_name
            payload = data[start:end]
            destination.write_bytes(payload)
            extracted.append({
                "name": safe_name,
                "size": len(payload),
                "sha256": hashlib.sha256(payload).hexdigest(),
                "path": str(destination),
            })
        result["extracted"] = extracted

    text = json.dumps(result, indent=2) + "\n"

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
    else:
        print(text, end="")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
