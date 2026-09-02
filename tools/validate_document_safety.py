#!/usr/bin/env python3
"""Static guardrails for unsaved-document and verification-experiment safety."""
from __future__ import annotations

import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"{label}: missing required safety marker: {needle}")


def main() -> int:
    tracker = (ROOT / "Source/Core/ProgramDocumentTracker.cpp").read_text()
    librarian = (ROOT / "Source/UI/ProgramLibrarian.cpp").read_text()
    librarian_header = (ROOT / "Source/UI/ProgramLibrarian.h").read_text()
    window = (ROOT / "Source/App/MainWindow.cpp").read_text()
    inspector = (ROOT / "Source/UI/SysExInspector.cpp").read_text()
    main_editor = (ROOT / "Source/UI/MainEditor.cpp").read_text()
    main_app = (ROOT / "Source/App/Main.cpp").read_text()
    cmake = (ROOT / "CMakeLists.txt").read_text()
    schema = json.loads((ROOT / "schemas/midi-capture.schema.json").read_text())

    require(tracker, "ProgramChangeOrigin::protocolInput", "document tracker")
    require(tracker, "ProgramChangeOrigin::import", "document tracker")
    require(tracker, "state.program() != cleanBaseline", "document tracker")
    require(librarian, "confirmDiscardProgramChanges", "librarian")
    require(librarian, "confirmDiscardBankChanges", "librarian")
    require(librarian, "confirmDiscardAllChanges", "librarian")
    require(librarian, "documentTracker.markClean()", "program JSON save")
    require(librarian, "cleanBankBaseline = bank", "bank JSON save")
    require(librarian, "saveUnsavedProgram", "save-aware destructive actions")
    require(librarian, "saveUnsavedBank", "save-aware destructive actions")
    require(librarian, "saveUnsavedChanges", "save-aware destructive actions")
    require(librarian, "Save & Continue", "save-aware destructive actions")
    require(librarian, "newProgram()", "explicit new-program workflow")
    require(librarian_header, 'newProgramButton { "New Program" }', "explicit new-program workflow")
    require(librarian, "openDocumentFile", "unified document open")
    require(librarian, "saveDocuments", "save-all workflow")
    require(librarian_header, 'openDocumentButton { "Open..." }', "unified document open")
    require(librarian_header, 'saveAllButton { "Save All" }', "save-all workflow")
    require(main_editor, "programLibrarian.openDocument()", "Cmd/Ctrl+O workflow")
    require(main_editor, "programLibrarian.saveDocuments()", "Cmd/Ctrl+S workflow")
    require(main_editor, "programLibrarian.saveProgramAs()", "Cmd/Ctrl+Shift+S workflow")
    require(main_app, "moreThanOneInstanceAllowed() override { return false; }", "single-instance document dispatch")
    require(main_app, "openFirstDocumentFromCommandLine", "OS/command-line document dispatch")
    require(cmake, "DOCUMENT_EXTENSIONS syx", "OS .syx association")
    require(window, "hasUnsavedChanges()", "quit protection")
    require(window, "Save & Quit", "quit protection")
    require(window, "Quit Without Saving", "quit protection")
    require(window, "saveUnsavedChanges", "quit save chain")

    require(inspector, "verificationCaptureFrozen", "verification experiment")
    require(inspector, "startVerificationExperiment", "verification experiment")
    require(inspector, "stopVerificationExperiment", "verification experiment")
    require(inspector, "observed_expected_nrpn_transactions", "verification assessment export")

    props = schema["properties"]["verification_context"]["properties"]
    for key in (
        "experiment_started_utc_ms",
        "experiment_completed_utc_ms",
        "capture_frozen",
        "observed_expected_nrpn_transactions",
        "observed_distinct_semantic_values",
        "observed_competing_nrpn_numbers",
    ):
        if key not in props:
            raise AssertionError(f"midi-capture schema missing verification field: {key}")

    print("PASS: save-aware document safeguards and frozen verification experiments are wired")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
