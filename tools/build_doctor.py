#!/usr/bin/env python3
"""Preflight AIM Editor's local build environment without modifying the machine."""
from __future__ import annotations

import argparse
import json
import importlib.metadata
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MIN_CMAKE = (3, 22, 0)
PINNED_JUCE = "9.0.1"
PINNED_JUCE_COMMIT = "e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8"
LINUX_APT_PACKAGES = [
    "ninja-build",
    "libasound2-dev",
    "libjack-jackd2-dev",
    "libfreetype6-dev",
    "libx11-dev",
    "libxcomposite-dev",
    "libxcursor-dev",
    "libxext-dev",
    "libxinerama-dev",
    "libxrandr-dev",
    "libxrender-dev",
    "libglu1-mesa-dev",
    "mesa-common-dev",
]


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
    parser.add_argument("--strict-platform", action="store_true", help="treat missing platform SDK/development libraries as hard failures")
    parser.add_argument("--require-ios-device-sdk", action="store_true", help="require macOS/Xcode plus a visible physical iPhoneOS SDK and devicectl")
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
        add("cmake", "fail", "CMake 3.22+ is required")

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

    if sys.version_info >= (3, 10):
        try:
            from jsonschema import Draft202012Validator  # noqa: F401
            from referencing import Registry, Resource  # noqa: F401

            jsonschema_version = importlib.metadata.version("jsonschema")
            referencing_version = importlib.metadata.version("referencing")
            add(
                "python-schema",
                "pass",
                f"Python {sys.version.split()[0]}, jsonschema {jsonschema_version}, referencing {referencing_version}; Draft 2020-12 available",
            )
        except Exception as exc:
            add(
                "python-schema",
                "fail",
                "Pinned repository-validation environment is incomplete: " + str(exc)
                + "; run tools/bootstrap_python_tools.py",
            )
    else:
        add("python-schema", "fail", f"Python 3.10+ is required; found {sys.version.split()[0]}")

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
            add("linux-pkg-config", "fail" if args.strict_platform else "warn", "pkg-config not found; Linux JUCE dependency check skipped")
        else:
            # These are the libraries AIM Editor/JUCE currently needs in CI.
            modules = ["alsa", "jack", "freetype2", "x11", "xext", "xinerama", "xrandr", "xcursor", "xcomposite", "xrender"]
            missing = [module for module in modules if subprocess.call([pkg, "--exists", module], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) != 0]
            if missing:
                status = "fail" if args.strict_platform else "warn"
                hint = "sudo apt-get install " + " ".join(LINUX_APT_PACKAGES)
                add("linux-libs", status, "Missing pkg-config modules: " + ", ".join(missing) + "; Debian/Ubuntu hint: " + hint)
            else:
                add("linux-libs", "pass", "Core JUCE Linux development libraries are visible to pkg-config")
    elif platform.system() == "Darwin":
        xcodebuild = shutil.which("xcodebuild")
        xcrun = shutil.which("xcrun")
        xcode_select = shutil.which("xcode-select")

        if xcode_select:
            selected = run_text([xcode_select, "-p"])
            add("xcode-select", "pass" if selected else ("fail" if args.strict_platform else "warn"), selected or "No active Xcode developer directory")
        else:
            add("xcode-select", "fail" if args.strict_platform else "warn", "xcode-select not found")

        if xcodebuild:
            detail = run_text([xcodebuild, "-version"]).replace("\n", "; ")
            add("xcode", "pass", detail)
        else:
            add("xcode", "fail" if args.strict_platform else "warn", "xcodebuild not found; macOS/iPadOS targets cannot be built here")

        if xcrun:
            macos_sdk = run_text([xcrun, "--sdk", "macosx", "--show-sdk-path"])
            add("macos-sdk", "pass" if macos_sdk else ("fail" if args.strict_platform else "warn"), macos_sdk or "macOS SDK not visible through xcrun")

            ios_device_sdk = run_text([xcrun, "--sdk", "iphoneos", "--show-sdk-path"])
            ios_status = "pass" if ios_device_sdk else ("fail" if args.require_ios_device_sdk else "warn")
            add("ios-device-sdk", ios_status, ios_device_sdk or "iPhoneOS SDK not visible through xcrun")

            devicectl = run_text([xcrun, "--find", "devicectl"])
            device_tool_status = "pass" if devicectl else ("fail" if args.require_ios_device_sdk else "warn")
            add("devicectl", device_tool_status, devicectl or "devicectl not visible through xcrun")
        else:
            add("xcrun", "fail" if (args.strict_platform or args.require_ios_device_sdk) else "warn", "xcrun not found")

    if args.require_ios_device_sdk and platform.system() != "Darwin":
        add("ios-device-sdk", "fail", "physical iPadOS builds require macOS/Xcode")

    result = {
        "format": "aim-editor.build-doctor",
        "schema_version": 1,
        "platform": platform.platform(),
        "pinned_juce": PINNED_JUCE,
        "pinned_juce_commit": PINNED_JUCE_COMMIT,
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
