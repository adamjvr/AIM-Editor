#!/usr/bin/env python3
"""Run AIM Editor's repository-level data/research self-checks."""
from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
IGNORED_RUNTIME_ROOTS = {".git", ".deps", ".idea", ".vscode"}


def is_runtime_generated(path: Path) -> bool:
    try:
        relative = path.relative_to(ROOT)
    except ValueError:
        return True
    if not relative.parts:
        return False
    first = relative.parts[0]
    return first in IGNORED_RUNTIME_ROOTS or first == "build" or first.startswith("build-") or first.startswith("cmake-build-")




def tracked_python_cache_artifacts() -> list[Path]:
    try:
        result = subprocess.run(
            ["git", "ls-files", "-z"],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        return []

    artifacts: list[Path] = []
    for raw in result.stdout.split(b"\0"):
        if not raw:
            continue
        relative = Path(raw.decode("utf-8", errors="surrogateescape"))
        if relative.suffix.lower() in {".pyc", ".pyo"} or "__pycache__" in relative.parts:
            artifacts.append(relative)
    return sorted(artifacts)

def run(*args: str) -> None:
    print("+", " ".join(args), flush=True)
    env = os.environ.copy()
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    subprocess.run(args, cwd=ROOT, check=True, env=env)


def main() -> int:
    run(sys.executable, "tools/validate_parameter_database.py")
    run(sys.executable, "tools/validate_editor_surface.py")
    run(sys.executable, "tools/validate_protocol_data.py")
    run(sys.executable, "tools/validate_native_formats.py")
    run(sys.executable, "tools/validate_session_safety.py")
    run(sys.executable, "tools/validate_document_safety.py")
    run(sys.executable, "tools/validate_build_workflow.py")
    run(sys.executable, "tools/validate_juce_api_contracts.py")
    run(sys.executable, "tools/protocol/ion_sysex.py", "self-test")
    run(sys.executable, "tools/protocol/ion_nrpn.py", "self-test")
    run(sys.executable, "tools/protocol/ion_patch_diff.py", "self-test")
    run(sys.executable, "tools/protocol/ion_nrpn_capture.py", "self-test")
    run(sys.executable, "tools/protocol/ion_protocol_verification.py", "self-test")
    run(sys.executable, "tools/protocol/ion_verification_report.py", "self-test")
    run(sys.executable, "tools/protocol/ion_verification_plan.py", "self-test")

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

    tracked_cache = tracked_python_cache_artifacts()
    if tracked_cache:
        print("FAIL: Python cache artifacts are tracked by git:", file=sys.stderr)
        for relative in tracked_cache:
            print(f"  {relative}", file=sys.stderr)
        return 1

    cache_artifacts = [
        p
        for p in ROOT.rglob("*")
        if p.is_file()
        and not is_runtime_generated(p)
        and (p.suffix in {".pyc", ".pyo"} or "__pycache__" in p.parts)
    ]
    if cache_artifacts:
        print("FAIL: Python cache artifacts must not be packaged/committed:", file=sys.stderr)
        for path in cache_artifacts:
            print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
        return 1

    conflict_markers = []
    text_suffixes = {".cpp", ".h", ".py", ".md", ".json", ".cmake", ".txt", ".sh", ".ps1", ".yml", ".yaml"}
    for path in ROOT.rglob("*"):
        if not path.is_file() or is_runtime_generated(path) or path.suffix.lower() not in text_suffixes:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        if any(marker in text for marker in (("<" * 7) + " ", (">" * 7) + " ")):
            conflict_markers.append(path)
    if conflict_markers:
        print("FAIL: unresolved merge-conflict markers:", file=sys.stderr)
        for path in conflict_markers:
            print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print("PASS: repository source/cache hygiene")

    print("PASS: AIM Editor repository checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
