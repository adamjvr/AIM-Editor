#!/usr/bin/env python3
"""Emit a JSON statistical profile of a locally extracted SynthMaker SME payload."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from collections import Counter
from pathlib import Path


def entropy(data: bytes) -> float:
    if not data:
        return 0.0
    counts = Counter(data)
    total = len(data)
    return -sum((count / total) * math.log2(count / total) for count in counts.values())


def printable_ratio(data: bytes) -> float:
    if not data:
        return 0.0
    printable = sum(1 for value in data if value in (9, 10, 13) or 32 <= value <= 126)
    return printable / len(data)


def profile(path: Path, window: int) -> dict:
    data = path.read_bytes()
    windows = []
    for offset in range(0, len(data), window):
        chunk = data[offset : offset + window]
        windows.append({
            "offset": offset,
            "size": len(chunk),
            "entropy_bits_per_byte": round(entropy(chunk), 6),
            "printable_ratio": round(printable_ratio(chunk), 6),
        })

    # This is intentionally only a statistical transition hint, not a claimed
    # serialization boundary. Require several consecutive low-entropy windows.
    transition = None
    for index in range(1, max(1, len(windows) - 3)):
        previous = windows[index - 1]["entropy_bits_per_byte"]
        run = windows[index : index + 4]
        if previous >= 7.5 and len(run) == 4 and all(item["entropy_bits_per_byte"] < 6.0 for item in run):
            transition = {
                "window_offset": windows[index]["offset"],
                "window_offset_hex": f"0x{windows[index]['offset']:08x}",
                "criterion": "previous window >= 7.5 bits/byte and next four windows < 6.0",
                "interpretation": "statistical transition only; exact codec/object boundary unverified",
            }
            break

    return {
        "format": "aim-editor.legacy-sme-profile",
        "schema_version": 1,
        "file": {
            "name": path.name,
            "size": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
        },
        "window_size": window,
        "overall_entropy_bits_per_byte": round(entropy(data), 6),
        "overall_printable_ratio": round(printable_ratio(data), 6),
        "candidate_entropy_transition": transition,
        "windows": windows,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sme", type=Path)
    parser.add_argument("--window", type=int, default=4096)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.window < 256:
        parser.error("window must be at least 256 bytes")

    result = profile(args.sme, args.window)
    text = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text)
    else:
        print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
