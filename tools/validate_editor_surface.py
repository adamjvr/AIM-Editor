#!/usr/bin/env python3
"""Validate the JSON parameter surface required by purpose-built editor panels."""
from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DB = ROOT / "data" / "parameters.json"

REQUIRED_SECTION_MINIMUMS = {
    "osc1": 5,
    "osc2": 5,
    "osc3": 5,
    "filter1": 5,
    "filter2": 5,
    "env_pitch": 13,
    "env_filter": 13,
    "env_amp": 13,
    "mod_matrix": 48,
    "tracking_generator": 38,
}

REQUIRED_ENVELOPE_IDS = {
    f"env.{name}.{stage}"
    for name in ("pitch", "filter", "amp")
    for stage in ("attack", "decay", "sustain", "release")
}

EXECUTABLE_DISPLAY_TRANSFORMS = {
    "raw",
    "enum",
    "raw * 0.1",
    "raw * 0.01",
    "raw-3",
    "raw-7",
    "exp(x/23.177415)/2 ms",
    "x==1023 ? 1000Hz : exp(x/88.85677)/100Hz",
    "x==1023 ? 20000Hz : exp(x/147.933647)*20Hz",
    "0..254: exp(x/25.5188668) ms; 255:30000ms; 256:held",
    "x==127 ? 10000ms : exp(x/18.38514)*10ms",
}


def main() -> int:
    root = json.loads(DB.read_text())
    parameters = root["parameters"]
    ids = {item["id"] for item in parameters}
    counts = Counter(item["section"] for item in parameters)

    for section, minimum in REQUIRED_SECTION_MINIMUMS.items():
        actual = counts[section]
        if actual < minimum:
            raise SystemExit(f"FAIL: {section} has {actual} parameters; expected at least {minimum}")

    missing = sorted(REQUIRED_ENVELOPE_IDS - ids)
    if missing:
        raise SystemExit("FAIL: envelope editor IDs missing: " + ", ".join(missing))

    transforms = Counter(item.get("display", {}).get("transform", "unknown") for item in parameters)
    executable = sum(count for transform, count in transforms.items() if transform in EXECUTABLE_DISPLAY_TRANSFORMS)
    unknown = transforms.get("unknown", 0)
    unhandled_documented = sorted(
        transform for transform in transforms
        if transform not in EXECUTABLE_DISPLAY_TRANSFORMS and transform != "unknown"
    )
    if unhandled_documented:
        raise SystemExit("FAIL: documented display transforms not handled by ParameterFormatter: "
                         + ", ".join(unhandled_documented))

    print("PASS: purpose-built editor JSON surface")
    print("PASS: oscillator lanes 3, filter lanes 2, envelope lanes 3")
    print(f"PASS: {executable}/{len(parameters)} parameters have executable display transforms")
    print(f"INFO: {unknown}/{len(parameters)} parameters intentionally remain raw/unknown display transforms")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
