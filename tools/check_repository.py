#!/usr/bin/env python3
"""Run AIM Editor's repository-level data/research self-checks."""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(*args: str) -> None:
    print("+", " ".join(args), flush=True)
    subprocess.run(args, cwd=ROOT, check=True)


def main() -> int:
    run(sys.executable, "tools/validate_parameter_database.py")
    run(sys.executable, "tools/validate_editor_surface.py")
    run(sys.executable, "tools/validate_protocol_data.py")
    run(sys.executable, "tools/validate_native_formats.py")
    run(sys.executable, "tools/validate_session_safety.py")
    run(sys.executable, "tools/protocol/ion_sysex.py", "self-test")
    run(sys.executable, "tools/protocol/ion_nrpn.py", "self-test")
    run(sys.executable, "tools/protocol/ion_patch_diff.py", "self-test")
    run(sys.executable, "tools/protocol/ion_nrpn_capture.py", "self-test")
    run(sys.executable, "tools/protocol/ion_protocol_verification.py", "self-test")

    schemas = sorted((ROOT / "schemas").glob("*.json"))
    for schema in schemas:
        json.loads(schema.read_text())
    print(f"PASS: {len(schemas)} JSON schema files parse")

    forbidden = {".exe", ".dll", ".sme", ".osm"}
    specimen_root = ROOT / "research" / "legacy-editor"
    bad = [p for p in specimen_root.rglob("*") if p.is_file() and p.suffix.lower() in forbidden]
    if bad:
        print("FAIL: reference binaries/payloads must not be committed:", file=sys.stderr)
        for path in bad:
            print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print("PASS: no legacy reference binaries/payloads in repository tree")

    cmake_text = (ROOT / "CMakeLists.txt").read_text()
    missing_sources = []
    for pattern in ("*.cpp", "*.h"):
        for source in sorted((ROOT / "Source").rglob(pattern)):
            relative = source.relative_to(ROOT).as_posix()
            if relative not in cmake_text:
                missing_sources.append(relative)
    if missing_sources:
        print("FAIL: Source files missing from CMakeLists.txt:", file=sys.stderr)
        for relative in missing_sources:
            print(f"  {relative}", file=sys.stderr)
        return 1
    print("PASS: every Source .cpp/.h file is listed in CMakeLists.txt")

    print("PASS: AIM Editor repository checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
