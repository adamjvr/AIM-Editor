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
    bootstrap_python = (ROOT / "tools/bootstrap_python_tools.py").read_text(encoding="utf-8")
    clean_python = (ROOT / "tools/clean_python_artifacts.py").read_text(encoding="utf-8")
    python_requirements = (ROOT / "tools/requirements-tools.txt").read_text(encoding="utf-8")
    doctor = (ROOT / "tools/build_doctor.py").read_text(encoding="utf-8")
    repository_check = (ROOT / "tools/check_repository.py").read_text(encoding="utf-8")
    build_sh = (ROOT / "tools/build_and_test.sh").read_text(encoding="utf-8")
    build_ps = (ROOT / "tools/build_and_test.ps1").read_text(encoding="utf-8")
    build_ipad = (ROOT / "tools/build_ipad_simulator.sh").read_text(encoding="utf-8")
    build_apple = (ROOT / "tools/build_apple_targets.sh").read_text(encoding="utf-8")
    ci = (ROOT / ".github/workflows/ci.yml").read_text(encoding="utf-8")

    require(cmake, "cmake_minimum_required(VERSION 3.22)", "root CMake minimum")
    require(cmake, "project(AIMEditor VERSION 0.1.0 LANGUAGES C CXX)", "root CMake")
    require(cmake, 'set(AIM_EDITOR_JUCE_TAG "9.0.1"', "root CMake")
    require(cmake, PINNED_COMMIT, "root CMake exact JUCE commit pin")
    require(cmake, "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)", "root CMake compile database")
    require(cmake, "DOCUMENT_EXTENSIONS syx", "root CMake")
    require(cmake, "FILE_SHARING_ENABLED TRUE", "iPad application file sharing")
    require(cmake, "DOCUMENT_BROWSER_ENABLED TRUE", "iPad document browser")
    require(cmake, "ICLOUD_PERMISSIONS_ENABLED TRUE", "iPad native document chooser entitlement")
    require(cmake, "UIInterfaceOrientationLandscapeLeft", "iPad landscape orientation")
    require(cmake, "UIInterfaceOrientationLandscapeRight", "iPad landscape orientation")
    require(ignore, "/.deps/", ".gitignore")

    require(bootstrap_python, 'VENV = ROOT / ".deps" / "python-tools"', "Python tooling bootstrap")
    require(bootstrap_python, "venv.EnvBuilder(with_pip=True, clear=True)", "Python tooling bootstrap")
    require(bootstrap_python, "Draft202012Validator", "Python tooling bootstrap self-test")
    require(bootstrap_python, "python3-venv", "Python tooling bootstrap Debian hint")
    for requirement in ("jsonschema==4.25.1", "referencing==0.36.2", "rpds-py==0.27.1"):
        require(python_requirements, requirement, "pinned Python tooling requirements")

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
    require(doctor, "MIN_CMAKE = (3, 22, 0)", "build doctor minimum")
    require(doctor, 'PINNED_JUCE = "9.0.1"', "build doctor")
    require(doctor, '"alsa", "jack", "freetype2", "x11"', "build doctor")
    require(doctor, 'JUCE 9 requires a C compiler', "build doctor")
    require(doctor, PINNED_COMMIT, "build doctor exact JUCE commit")
    require(doctor, "--strict-platform", "build doctor strict platform mode")
    require(doctor, "--require-ios-simulator-sdk", "build doctor iPadOS SDK gate")
    require(doctor, '"iphonesimulator"', "build doctor iPadOS SDK discovery")
    require(doctor, "Draft202012Validator", "build doctor Python schema capability")
    require(doctor, "python-schema", "build doctor Python schema result")
    require(repository_check, 'IGNORED_RUNTIME_ROOTS = {".git", ".deps", ".idea", ".vscode"}', "repository local dependency exclusion")
    require(repository_check, "is_runtime_generated", "repository local dependency exclusion")
    require(repository_check, "tracked_python_cache_artifacts", "repository tracked-cache guard")
    require(clean_python, "tracked cache artifacts are treated as an error", "Python cache cleanup safety")
    require(clean_python, "git", "Python cache cleanup tracked-file check")

    for text, label in ((build_sh, "POSIX build script"), (build_ps, "Windows build script")):
        require(text, "bootstrap_python_tools.py", label)
        require(text, "clean_python_artifacts.py", label)
        require(text, "PYTHONDONTWRITEBYTECODE", label)
        require(text, "python-tools", label)
        require(text, "build_doctor.py", label)
        require(text, "check_repository.py", label)
        require(text, "AIM_EDITOR_JUCE_PATH", label)
        require(text, "AIM_EDITOR_BUILD_TESTS=ON", label)
        require(text, "--require-local-juce", label)
        require(text, "--strict-platform", label)

    require(build_sh, "AIM_EDITOR_BOOTSTRAP_JUCE", "POSIX one-command bootstrap")
    require(build_ps, "NoBootstrap", "Windows one-command bootstrap")

    require(build_ipad, "AIMEditor", "iPadOS simulator build script")

    require(build_ipad, "bootstrap_python_tools.py", "iPadOS simulator build script")
    require(build_ipad, "clean_python_artifacts.py", "iPadOS simulator build script")
    require(build_ipad, "check_repository.py", "iPadOS simulator build script")
    require(build_ipad, "--require-local-juce", "iPadOS simulator build script")
    require(build_ipad, "--require-ios-simulator-sdk", "iPadOS simulator build script")
    require(build_ipad, "-DCMAKE_SYSTEM_NAME=iOS", "iPadOS simulator build script")
    require(build_ipad, "-DCMAKE_OSX_SYSROOT=iphonesimulator", "iPadOS simulator build script")
    require(build_ipad, "CODE_SIGNING_ALLOWED=NO", "iPadOS simulator build script")
    require(build_apple, "build_and_test.sh", "Apple aggregate build script")
    require(build_apple, "build_ipad_simulator.sh", "Apple aggregate build script")
    require(build_apple, "Apple standalone-application gate", "Apple aggregate build script")
    require(ci, "Bootstrap pinned Python validation tools", "CI Python validation bootstrap")
    require(ci, "Bootstrap verified JUCE (Unix)", "CI verified JUCE bootstrap")
    require(ci, "Bootstrap verified JUCE (Windows)", "CI verified JUCE bootstrap")
    require(ci, "permissions:\n  contents: read", "CI least-privilege permissions")

    # Build/bootstrap scripts must never make hardware transmission persistent or
    # silently opt-in to experimental MIDI writes as a side effect of building.
    combined = "\n".join((bootstrap_sh, bootstrap_ps, bootstrap_python, doctor, build_sh, build_ps, build_ipad, build_apple))
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
