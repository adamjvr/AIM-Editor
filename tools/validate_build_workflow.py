#!/usr/bin/env python3
"""Static guardrails for AIM Editor's reproducible cross-platform build workflow."""
from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
PINNED_JUCE = "9.0.1"
PINNED_COMMIT = "e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8"
HASHES = {
    "linux": "2389950c45a30cdddc00c8fb35ecc898d97781760fd25aa2589eaf7057ffbf44",
    "macos": "dda2aefd73796f9b8d37b8f84d7154a7881706bf8aa5a3467334277629001875",
    "windows": "de0256584416764d82ef5794d26a34b06a68bc65326f24e0d91301abd74704c1",
}


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"{label}: missing required build marker: {needle}")


def main() -> int:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    juce_cmake = (ROOT / "cmake/JUCE.cmake").read_text(encoding="utf-8")
    ignore = (ROOT / ".gitignore").read_text(encoding="utf-8")
    bootstrap_sh = (ROOT / "tools/bootstrap_juce.sh").read_text(encoding="utf-8")
    bootstrap_ps = (ROOT / "tools/bootstrap_juce.ps1").read_text(encoding="utf-8")
    doctor = (ROOT / "tools/build_doctor.py").read_text(encoding="utf-8")
    build_sh = (ROOT / "tools/build_and_test.sh").read_text(encoding="utf-8")
    build_ps = (ROOT / "tools/build_and_test.ps1").read_text(encoding="utf-8")
    ci = (ROOT / ".github/workflows/ci.yml").read_text(encoding="utf-8")

    require(cmake, "project(AIMEditor VERSION 0.1.0 LANGUAGES C CXX)", "root CMake")
    require(cmake, 'set(AIM_EDITOR_JUCE_TAG "9.0.1"', "root CMake")
    require(cmake, PINNED_COMMIT, "root CMake exact JUCE commit pin")
    require(cmake, "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)", "root CMake compile database")
    require(cmake, "DOCUMENT_EXTENSIONS syx", "root CMake")
    require(ignore, "/.deps/", ".gitignore")

    require(juce_cmake, '${CMAKE_SOURCE_DIR}/.deps/JUCE/CMakeLists.txt', "JUCE CMake resolver")
    require(juce_cmake, "AIM_EDITOR_JUCE_PATH", "JUCE CMake resolver")
    require(juce_cmake, "FetchContent_Declare", "JUCE CMake resolver")
    require(juce_cmake, "GIT_TAG ${AIM_EDITOR_JUCE_COMMIT}", "JUCE CMake exact checkout")
    require(juce_cmake, "_aim_juce_local_version STREQUAL AIM_EDITOR_JUCE_TAG", "JUCE CMake local version guard")

    require(bootstrap_sh, f'TAG="${{AIM_EDITOR_JUCE_TAG:-{PINNED_JUCE}}}"', "POSIX JUCE bootstrap")
    require(bootstrap_ps, f'[string]$Version = "{PINNED_JUCE}"', "Windows JUCE bootstrap")
    for platform_name, digest in HASHES.items():
        source = bootstrap_ps if platform_name == "windows" else bootstrap_sh
        require(source.lower(), digest, f"{platform_name} JUCE archive pin")

    require(bootstrap_sh, "JUCE archive SHA-256 mismatch", "POSIX JUCE bootstrap")
    require(bootstrap_ps, "JUCE archive SHA-256 mismatch", "Windows JUCE bootstrap")
    require(doctor, 'PINNED_JUCE = "9.0.1"', "build doctor")
    require(doctor, '"alsa", "jack", "freetype2", "x11"', "build doctor")
    require(doctor, 'JUCE 9 requires a C compiler', "build doctor")
    require(doctor, PINNED_COMMIT, "build doctor exact JUCE commit")
    require(doctor, "--strict-platform", "build doctor strict platform mode")

    for text, label in ((build_sh, "POSIX build script"), (build_ps, "Windows build script")):
        require(text, "build_doctor.py", label)
        require(text, "check_repository.py", label)
        require(text, "AIM_EDITOR_JUCE_PATH", label)
        require(text, "AIM_EDITOR_BUILD_TESTS=ON", label)
        require(text, "--require-local-juce", label)
        require(text, "--strict-platform", label)

    require(build_sh, "AIM_EDITOR_BOOTSTRAP_JUCE", "POSIX one-command bootstrap")
    require(build_ps, "NoBootstrap", "Windows one-command bootstrap")
    require(ci, "Bootstrap verified JUCE (Unix)", "CI verified JUCE bootstrap")
    require(ci, "Bootstrap verified JUCE (Windows)", "CI verified JUCE bootstrap")
    require(ci, "permissions:\n  contents: read", "CI least-privilege permissions")

    # Build/bootstrap scripts must never make hardware transmission persistent or
    # silently opt-in to experimental MIDI writes as a side effect of building.
    combined = "\n".join((bootstrap_sh, bootstrap_ps, doctor, build_sh, build_ps))
    for forbidden in ("setLiveNrpnEnabled (true)", "fullWriteArmed = true", "writeArmed = true"):
        if forbidden in combined:
            raise AssertionError(f"build tooling contains unsafe runtime enable marker: {forbidden}")

    print("PASS: pinned JUCE bootstrap/build-doctor workflow is reproducible and write-safe")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
