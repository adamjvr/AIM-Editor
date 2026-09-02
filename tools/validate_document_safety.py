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
    window = (ROOT / "Source/App/MainWindow.cpp").read_text()
    inspector = (ROOT / "Source/UI/SysExInspector.cpp").read_text()
    schema = json.loads((ROOT / "schemas/midi-capture.schema.json").read_text())

    require(tracker, "ProgramChangeOrigin::protocolInput", "document tracker")
    require(tracker, "ProgramChangeOrigin::import", "document tracker")
    require(tracker, "state.program() != cleanBaseline", "document tracker")
    require(librarian, "confirmDiscardProgramChanges", "librarian")
    require(librarian, "confirmDiscardBankChanges", "librarian")
    require(librarian, "confirmDiscardAllChanges", "librarian")
    require(librarian, "documentTracker.markClean()", "program JSON save")
    require(librarian, "cleanBankBaseline = bank", "bank JSON save")
    require(window, "hasUnsavedChanges()", "quit protection")
    require(window, "Quit Without Saving", "quit protection")

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

    print("PASS: unsaved-document safeguards and frozen verification experiments are wired")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
