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


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise AssertionError(f"{label}: forbidden build marker present: {needle}")


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
    build_ipad = (ROOT / "tools/build_ipad_device.sh").read_text(encoding="utf-8")
    select_ipad = (ROOT / "tools/select_ipad_device.py").read_text(encoding="utf-8")
    build_apple = (ROOT / "tools/build_apple_targets.sh").read_text(encoding="utf-8")
    ci = (ROOT / ".github/workflows/ci.yml").read_text(encoding="utf-8")
    main_window = (ROOT / "Source/App/MainWindow.cpp").read_text(encoding="utf-8")
    main_editor = (ROOT / "Source/UI/MainEditor.cpp").read_text(encoding="utf-8")
    editor_page = (ROOT / "Source/UI/Panels/EditorPage.cpp").read_text(encoding="utf-8")
    global_bar = (ROOT / "Source/UI/GlobalControlBar.cpp").read_text(encoding="utf-8")

    require(cmake, "cmake_minimum_required(VERSION 3.22)", "root CMake minimum")
    require(cmake, "project(AIMEditor VERSION 0.1.0 LANGUAGES C CXX)", "root CMake")
    require(cmake, 'set(AIM_EDITOR_JUCE_TAG "9.0.1"', "root CMake")
    require(cmake, PINNED_COMMIT, "root CMake exact JUCE commit pin")
    require(cmake, "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)", "root CMake compile database")
    require(cmake, "DOCUMENT_EXTENSIONS syx", "root CMake")
    require(cmake, "FILE_SHARING_ENABLED TRUE", "iPad application file sharing")
    require(cmake, "DOCUMENT_BROWSER_ENABLED TRUE", "iPad document browser")
    require(cmake, "STATUS_BAR_HIDDEN TRUE", "iPad full-screen status bar policy")
    require(cmake, "REQUIRES_FULL_SCREEN TRUE", "iPad landscape-only full-screen policy")
    require(cmake, "UIViewControllerBasedStatusBarAppearance", "iPad view-controller status-bar override")
    require(cmake, "<false/>", "iPad global status-bar policy")
    forbid(cmake, "ICLOUD_PERMISSIONS_ENABLED TRUE", "unnecessary iCloud provisioning dependency")
    require(cmake, 'BUNDLE_ID "com.rothamplification.aimeditor"', "physical-device bundle identity")
    require(cmake, "TARGETED_DEVICE_FAMILY 2", "iPad-only device family")
    require(cmake, "XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC NO", "JUCE iOS Objective-C++ non-ARC boundary")
    forbid(cmake, "XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES", "forcing ARC across JUCE Objective-C++ modules")
    require(cmake, "UIInterfaceOrientationLandscapeLeft", "iPad landscape orientation")
    require(cmake, "UIInterfaceOrientationLandscapeRight", "iPad landscape orientation")
    require(ignore, "/.deps/", ".gitignore")
    require(main_window, "setKioskModeComponent (this, false)", "physical iPad kiosk/full-screen status-bar policy")
    require(main_editor, '"Alesis ION/Micron Editor"', "front-page product title")
    require(editor_page, '"shared JSON state / desktop + iPad touch surface"', "ASCII-safe editor metadata banner")
    forbid(editor_page, "â€¢", "mojibake editor metadata banner")
    require(global_bar, "drawSelectorHeader", "non-overlapping global selector headers")
    forbid(global_bar, "translated (0, -15)", "selector header overlap regression")

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
    require(doctor, "--require-ios-device-sdk", "build doctor physical iPadOS SDK gate")
    require(doctor, '"iphoneos"', "build doctor physical iPadOS SDK discovery")
    require(doctor, '"devicectl"', "build doctor CoreDevice tooling")
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

    require(build_ipad, "AIMEditor", "physical iPadOS build script")
    require(build_ipad, "bootstrap_python_tools.py", "physical iPadOS build script")
    require(build_ipad, "clean_python_artifacts.py", "physical iPadOS build script")
    require(build_ipad, "check_repository.py", "physical iPadOS build script")
    require(build_ipad, 'cmake -E remove_directory "$BUILD_DIR"', "physical iPadOS clean cross-build cache")
    require(build_ipad, "--require-local-juce", "physical iPadOS build script")
    require(build_ipad, "--require-ios-device-sdk", "physical iPadOS build script")
    require(build_ipad, "select_ipad_device.py", "physical iPad selection")
    require(build_ipad, '--device "$DEVICE_SELECTOR"', "explicit physical iPad selector forwarding")
    require(build_ipad, 'if DEVICE_ENV=', "selector failure propagation")
    forbid(build_ipad, 'eval "$("$TOOLS_PYTHON"', "masked selector subprocess failure")
    require(build_ipad, "AIM_EDITOR_DEVELOPMENT_TEAM", "explicit Apple development team")
    require(build_ipad, ".aim-editor-local.env", "machine-local Apple deployment configuration")
    require(build_ipad, "-allowProvisioningDeviceRegistration", "physical-device provisioning registration")
    require(build_ipad, "-DCMAKE_SYSTEM_NAME=iOS", "physical iPadOS build script")
    require(build_ipad, "-DCMAKE_OSX_SYSROOT=iphoneos", "physical iPadOS SDK")
    require(build_ipad, "-DCMAKE_OSX_ARCHITECTURES=arm64", "physical iPadOS architecture")
    require(build_ipad, '-destination "platform=iOS,id=$AIM_IPAD_UDID"', "physical iPad destination")
    require(build_ipad, "-allowProvisioningUpdates", "automatic physical-device provisioning")
    require(build_ipad, "CODE_SIGN_STYLE=Automatic", "automatic physical-device signing")
    require(build_ipad, "aimeditor-ipad-build.log", "persistent physical-device compiler log")
    require(build_ipad, "CLANG_ENABLE_OBJC_ARC", "generated Xcode ARC preflight")
    require(build_ipad, "Objective-C ARC", "generated Xcode ARC preflight diagnostic")
    require(build_ipad, "PIPESTATUS[0]", "physical-device build failure propagation through tee")
    require(build_ipad, "Compiler diagnostics extracted from failed iPad build", "physical-device compiler diagnostic extraction")
    require(build_ipad, "TARGET_BUILD_DIR", "physical iPad generated product location")
    require(build_ipad, "FULL_PRODUCT_NAME", "physical iPad generated product name")
    require(build_ipad, "codesign --verify --deep --strict", "physical iPad code-signature verification")
    require(build_ipad, "UIStatusBarHidden", "packaged iPad status-bar verification")
    require(build_ipad, "UIRequiresFullScreen", "packaged iPad full-screen verification")
    require(build_ipad, "UIViewControllerBasedStatusBarAppearance", "packaged iPad view-controller status-bar verification")
    forbid(build_ipad, "-path '*iphoneos*'", "fragile physical iPad application path guess")
    require(build_ipad, "devicectl device install app", "physical iPad installation")
    require(build_ipad, "devicectl device process launch", "physical iPad launch")
    require(build_ipad, "com.rothamplification.aimeditor", "physical-device bundle launch")
    require(build_ipad, "-DCMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET=17.0", "iPadOS device deployment target")
    forbid(build_ipad, "iphonesimulator", "simulator SDK path")
    forbid(build_ipad, "iOS Simulator", "simulator destination path")
    forbid(build_ipad, "CODE_SIGNING_ALLOWED=NO", "unsigned simulator-style build")
    forbid(build_ipad, "-DCMAKE_OSX_DEPLOYMENT_TARGET=17.0", "JUCE host-tool deployment target isolation")

    require(select_ipad, 'device_type != "ipad"', "physical iPad device filter")
    require(select_ipad, 'reality == "simulated"', "simulator rejection")
    require(select_ipad, 'developerModeStatus', "physical iPad Developer Mode gate")
    require(select_ipad, '"online": online', "physical iPad availability state preservation")
    require(select_ipad, 'is known to CoreDevice but is not currently available', "explicit offline-device diagnostic")
    require(select_ipad, 'devicectl", "list", "devices"', "CoreDevice inventory")

    require(build_apple, "build_and_test.sh", "Apple aggregate build script")
    require(build_apple, "build_ipad_device.sh", "Apple aggregate physical iPad script")
    require(build_apple, "MAC_PID", "concurrent macOS Apple gate")
    require(build_apple, "IPAD_PID", "concurrent iPadOS Apple gate")
    require(build_apple, 'wait "$MAC_PID"', "concurrent macOS wait")
    require(build_apple, 'wait "$IPAD_PID"', "concurrent iPadOS wait")
    require(build_apple, "run_prefixed macOS", "concurrent macOS output prefix")
    require(build_apple, "run_prefixed iPadOS", "concurrent iPadOS output prefix")
    require(build_apple, "build-logs", "concurrent Apple log retention")
    require(build_apple, "bootstrap_python_tools.py", "shared Python bootstrap before concurrent Apple jobs")
    require(build_apple, "Bootstrap shared pinned JUCE before concurrent Apple builds", "shared JUCE bootstrap before concurrent Apple jobs")
    require(build_apple, "run_prefixed", "concurrent Apple status-preserving runner")
    require(build_apple, "PIPESTATUS[0]", "concurrent Apple child failure propagation")
    require(build_apple, "Launching %s", "macOS application launch after successful build")
    require(build_apple, 'open "$MAC_APP"', "macOS application launch command")
    require(build_apple, "AIM Editor.app", "macOS application product lookup")
    forbid(build_ipad, "security find-identity", "certificate display-name team inference")
    require(build_apple, "concurrent macOS + physical iPadOS application gate", "Apple aggregate build script")
    forbid(build_apple, "simulator", "Apple aggregate simulator path")

    forbid(ci, "ipad-simulator", "CI simulator job")
    forbid(ci, "iphonesimulator", "CI simulator SDK")
    require(ci, "Bootstrap pinned Python validation tools", "CI Python validation bootstrap")
    require(ci, "Bootstrap verified JUCE (Unix)", "CI verified JUCE bootstrap")
    require(ci, "Bootstrap verified JUCE (Windows)", "CI verified JUCE bootstrap")
    require(ci, "permissions:\n  contents: read", "CI least-privilege permissions")

    # Build/bootstrap scripts must never make hardware transmission persistent or
    # silently opt-in to experimental MIDI writes as a side effect of building.
    combined = "\n".join((bootstrap_sh, bootstrap_ps, bootstrap_python, doctor, build_sh, build_ps, build_ipad, select_ipad, build_apple))
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
