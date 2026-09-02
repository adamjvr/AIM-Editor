#!/usr/bin/env python3
"""Static guards for JUCE 9 API contracts AIM Editor intentionally relies on.

This is not a substitute for compiling against JUCE. It prevents a few known
cross-platform footguns from being reintroduced while the project is still in
protocol/data-heavy development.
"""
from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    sources = list((ROOT / "Source").rglob("*.cpp")) + list((ROOT / "Source").rglob("*.h"))
    combined = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in sources)

    # JUCE 9 has two different historical result conventions for async message
    # boxes. NativeMessageBox::showAsync(MessageBoxOptions, ...) reports a
    # zero-based button index. AlertWindow's non-native implementation preserves
    # the old AlertWindow result codes (three buttons => 1, 2, 0). AIM Editor's
    # destructive-action callbacks are intentionally written for zero-based
    # indexes, so they must use NativeMessageBox::showAsync.
    forbidden = (
        "juce::AlertWindow::showAsync (options",
        "juce::AlertWindow::showScopedAsync (options",
        "juce::NativeMessageBox::showScopedAsync (options",
    )
    for needle in forbidden:
        if needle in combined:
            raise AssertionError(
                f"ambiguous JUCE async message-box result mapping reintroduced: {needle}"
            )

    main_window = (ROOT / "Source/App/MainWindow.cpp").read_text(encoding="utf-8")
    librarian = (ROOT / "Source/UI/ProgramLibrarian.cpp").read_text(encoding="utf-8")

    if "juce::NativeMessageBox::showAsync (options" not in main_window:
        raise AssertionError("quit confirmation must use zero-based NativeMessageBox::showAsync")
    if librarian.count("juce::NativeMessageBox::showAsync (options") < 3:
        raise AssertionError("all three save/discard/cancel workflows must use NativeMessageBox::showAsync")

    for text, label in ((main_window, "quit confirmation"), (librarian, "document confirmation")):
        if "buttonIndex == 0" not in text or "buttonIndex == 1" not in text:
            raise AssertionError(f"{label} no longer handles the expected zero-based first/second buttons")

    # First real JUCE 9.0.1/GCC build wave exposed several source patterns that
    # our earlier CMake-only checks could not catch. Keep the repaired forms
    # mechanically guarded so future UI passes do not reintroduce them.
    forbidden_compile_patterns = {
        ".getLast()": "juce::StringArray has no getLast() member in JUCE 9.0.1",
        "for (auto* component : {": "heterogeneous JUCE component pointer list needs an explicit Component* element type",
        "programFile = {};": "juce::File reset is ambiguous; use juce::File{}",
        "bankFile = {};": "juce::File reset is ambiguous; use juce::File{}",
        "AIMBinaryData::modulation_sources_json": "JUCE BinaryData removes filename hyphens",
        "AIMBinaryData::modulation_destinations_json": "JUCE BinaryData removes filename hyphens",
        "AIMBinaryData::filter_types_json": "JUCE BinaryData removes filename hyphens",
    }
    for pattern, reason in forbidden_compile_patterns.items():
        if pattern in combined:
            raise AssertionError(f"real-build compile regression: {reason}: {pattern}")

    app_main = (ROOT / "Source/App/Main.cpp").read_text(encoding="utf-8")
    core_tests = (ROOT / "Tests/CoreTests.cpp").read_text(encoding="utf-8")
    expected_binary_data = (
        "AIMBinaryData::modulationsources_json",
        "AIMBinaryData::modulationdestinations_json",
        "AIMBinaryData::filtertypes_json",
    )
    for symbol in expected_binary_data:
        if symbol not in app_main or symbol not in core_tests:
            raise AssertionError(f"generated BinaryData symbol contract missing: {symbol}")

    oscillator = (ROOT / "Source/UI/Components/OscillatorPanel.cpp").read_text(encoding="utf-8")
    if "const auto lane = juce::Rectangle<int>" in oscillator:
        raise AssertionError("OscillatorPanel mutates lane geometry; lane must not be const")

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    juce_cmake = (ROOT / "cmake/JUCE.cmake").read_text(encoding="utf-8")
    if 'AIM_EDITOR_JUCE_COMMIT "e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8"' not in cmake:
        raise AssertionError("exact JUCE 9.0.1 commit pin is missing")
    if "GIT_TAG ${AIM_EDITOR_JUCE_COMMIT}" not in juce_cmake:
        raise AssertionError("FetchContent must use the exact JUCE commit pin")
    if "_aim_juce_local_version STREQUAL AIM_EDITOR_JUCE_TAG" not in juce_cmake:
        raise AssertionError("local JUCE version guard is missing")

    print("PASS: JUCE 9 async-dialog and dependency-pin contracts are guarded")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
