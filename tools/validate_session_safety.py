#!/usr/bin/env python3
"""Static invariants for persistent session restore and verification capture."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


def main() -> int:
    settings = (ROOT / "Source/Core/AppSettings.cpp").read_text()
    bar = (ROOT / "Source/UI/GlobalControlBar.cpp").read_text()
    inspector = (ROOT / "Source/UI/SysExInspector.cpp").read_text()
    schema = json.loads((ROOT / "schemas/midi-capture.schema.json").read_text())

    persisted_keys = {
        "session.page",
        "session.midi_channel",
        "session.bank",
        "session.program",
        "session.midi_input_identifier",
        "session.midi_output_identifier",
    }
    for key in persisted_keys:
        require(key in settings, f"missing persistent key {key}")

    lowered = settings.lower()
    require("live nrpn" not in lowered and "live_edit" not in lowered and "liveedit" not in lowered,
            "live editing must never be persisted")
    require("write arm" not in lowered and "write_arm" not in lowered and "armed" not in lowered,
            "hardware write arming must never be persisted")

    require('liveEdit.setToggleState (false, juce::dontSendNotification)' in bar,
            "session restore must explicitly force live NRPN off")
    restore_body = bar.split("void GlobalControlBar::restoreSession", 1)[1].split("void GlobalControlBar::captureSession", 1)[0]
    require("sendNow" not in restore_body and "requestCurrentPatch" not in restore_body,
            "session restore must not transmit MIDI")

    require('root->setProperty ("nrpn_transactions"' in inspector,
            "SysEx inspector must export reconstructed NRPN transactions")
    nrpn_schema = schema.get("$defs", {}).get("nrpn_transaction")
    require(isinstance(nrpn_schema, dict), "midi capture schema missing nrpn_transaction")
    required = set(nrpn_schema.get("required", []))
    for field in {"nrpn", "value14", "parameter_id", "mapping_status", "semantic_value"}:
        require(field in required, f"NRPN transaction schema missing {field}")

    print("PASS: persistent session excludes live/write enable state")
    print("PASS: restoring session is MIDI-transmit free")
    print("PASS: MIDI capture exports semantic candidate NRPN transactions")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
