#!/usr/bin/env python3
"""Preflight AIM Editor's local build environment without modifying the machine."""
from __future__ import annotations

import argparse
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MIN_CMAKE = (3, 24, 0)
PINNED_JUCE = "9.0.1"


def run_text(args: list[str]) -> str:
    try:
        return subprocess.check_output(args, stderr=subprocess.STDOUT, text=True).strip()
    except Exception:
        return ""


def version_tuple(text: str) -> tuple[int, int, int] | None:
    match = re.search(r"(\d+)\.(\d+)(?:\.(\d+))?", text)
    if not match:
        return None
    return tuple(int(part or 0) for part in match.groups())


def candidate_juce_paths() -> list[Path]:
    items: list[Path] = []
    if os.environ.get("AIM_EDITOR_JUCE_PATH"):
        items.append(Path(os.environ["AIM_EDITOR_JUCE_PATH"]).expanduser())
    items.append(ROOT / ".deps" / "JUCE")
    items.append(Path.home() / "GitHub" / "JUCE")
    unique: list[Path] = []
    seen: set[str] = set()
    for item in items:
        key = str(item.resolve()) if item.exists() else str(item)
        if key not in seen:
            seen.add(key)
            unique.append(item)
    return unique


def juce_version(path: Path) -> str | None:
    cmake = path / "CMakeLists.txt"
    if not cmake.is_file():
        return None
    text = cmake.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"project\s*\(\s*JUCE\s+VERSION\s+([0-9.]+)", text, re.I)
    return match.group(1) if match else None


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    parser.add_argument("--require-local-juce", action="store_true", help="fail if no local pinned JUCE tree exists")
    args = parser.parse_args()

    checks: list[dict[str, object]] = []

    def add(name: str, status: str, detail: str) -> None:
        checks.append({"name": name, "status": status, "detail": detail})

    cmake_cmd = shutil.which("cmake")
    cmake_text = run_text([cmake_cmd, "--version"]) if cmake_cmd else ""
    cmake_ver = version_tuple(cmake_text)
    if cmake_cmd and cmake_ver and cmake_ver >= MIN_CMAKE:
        add("cmake", "pass", cmake_text.splitlines()[0])
    else:
        add("cmake", "fail", "CMake 3.24+ is required")

    cxx = next((shutil.which(name) for name in (os.environ.get("CXX", ""), "c++", "clang++", "g++") if name), None)
    if cxx:
        first = run_text([cxx, "--version"]).splitlines()
        add("cxx", "pass", first[0] if first else cxx)
    elif platform.system() == "Windows" and shutil.which("cl"):
        add("cxx", "pass", "MSVC cl.exe")
    else:
        add("cxx", "fail", "No C++ compiler found")

    c_compiler = next((shutil.which(name) for name in (os.environ.get("CC", ""), "cc", "clang", "gcc") if name), None)
    if c_compiler:
        add("c", "pass", c_compiler)
    elif platform.system() == "Windows" and shutil.which("cl"):
        add("c", "pass", "MSVC cl.exe")
    else:
        add("c", "fail", "JUCE 9 requires a C compiler as well as C++")

    ninja = shutil.which("ninja")
    add("ninja", "pass" if ninja else "warn", ninja or "Ninja not found; CMake may use another generator")

    local_juce: Path | None = None
    local_version: str | None = None
    for candidate in candidate_juce_paths():
        version = juce_version(candidate)
        if version:
            local_juce, local_version = candidate, version
            break
    if local_juce and local_version == PINNED_JUCE:
        add("juce", "pass", f"JUCE {local_version} at {local_juce}")
    elif local_juce:
        add("juce", "fail", f"JUCE {local_version or 'unknown'} at {local_juce}; project is pinned to {PINNED_JUCE}")
    else:
        status = "fail" if args.require_local_juce else "warn"
        add("juce", status, "No local JUCE tree found; run tools/bootstrap_juce.sh (or .ps1) or allow CMake FetchContent")

    if platform.system() == "Linux":
        pkg = shutil.which("pkg-config")
        if not pkg:
            add("linux-pkg-config", "warn", "pkg-config not found; Linux JUCE dependency check skipped")
        else:
            # These are the libraries AIM Editor/JUCE currently needs in CI.
            modules = ["alsa", "freetype2", "x11", "xext", "xinerama", "xrandr", "xcursor", "xcomposite", "xrender"]
            missing = [module for module in modules if subprocess.call([pkg, "--exists", module], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) != 0]
            if missing:
                add("linux-libs", "warn", "Missing pkg-config modules: " + ", ".join(missing))
            else:
                add("linux-libs", "pass", "Core JUCE Linux development libraries are visible to pkg-config")
    elif platform.system() == "Darwin":
        xcodebuild = shutil.which("xcodebuild")
        if xcodebuild:
            detail = run_text([xcodebuild, "-version"]).replace("\n", "; ")
            add("xcode", "pass", detail)
        else:
            add("xcode", "warn", "xcodebuild not found; macOS/iPadOS targets cannot be built here")

    result = {
        "format": "aim-editor.build-doctor",
        "schema_version": 1,
        "platform": platform.platform(),
        "pinned_juce": PINNED_JUCE,
        "checks": checks,
    }
    failures = [check for check in checks if check["status"] == "fail"]

    if args.json:
        print(json.dumps(result, indent=2))
    else:
        for check in checks:
            print(f"{str(check['status']).upper():4}  {check['name']}: {check['detail']}")
        if failures:
            print(f"\nFAIL: {len(failures)} hard build prerequisite(s) missing")
        else:
            print("\nPASS: hard build prerequisites are present")

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
