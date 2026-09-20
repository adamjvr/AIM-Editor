#!/usr/bin/env python3
"""Guard the shared Ion/Micron Program architecture and Micron evidence boundary."""
from __future__ import annotations

import json
from pathlib import Path

from jsonschema import Draft202012Validator

ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


def load_json(relative: str) -> dict:
    return json.loads((ROOT / relative).read_text(encoding="utf-8"))


def validate(instance_path: str, schema_path: str) -> dict:
    instance = load_json(instance_path)
    schema = load_json(schema_path)
    Draft202012Validator.check_schema(schema)
    errors = sorted(Draft202012Validator(schema).iter_errors(instance), key=lambda e: list(e.path))
    if errors:
        first = errors[0]
        location = "/".join(str(p) for p in first.path) or "<root>"
        raise SystemExit(f"FAIL: {instance_path} schema error at {location}: {first.message}")
    return instance


def main() -> int:
    profiles = validate("data/device-profiles.json", "schemas/device-profiles.schema.json")
    evidence = validate("research/external/micronau-evidence.json", "schemas/micronau-evidence.schema.json")

    require(profiles["program_format"]["program_dump_product_id"] == 0x22,
            "shared Program dump product ID must remain 0x22")
    require(profiles["program_format"]["decoded_bytes"] == 378,
            "shared Program image must remain 378 decoded bytes")
    require(profiles["program_format"]["encoded_payload_bytes"] == 432,
            "shared Program SysEx payload must remain 432 transport bytes")
    require(profiles["program_format"]["wire_bytes"] == 434,
            "shared Program SysEx wire size must remain 434 bytes")

    by_id = {device["id"]: device for device in profiles["devices"]}
    require(set(by_id) == {"ion", "micron"}, "device profiles must contain exactly Ion and Micron in Pass 31")
    require(by_id["ion"]["request_product_id"] == 0x22, "Ion request product ID drift")
    require(by_id["micron"]["request_product_id"] == 0x26, "Micron request product ID drift")
    require(by_id["micron"]["program_dump_product_id"] == 0x22, "Micron Program dump must remain shared 0x22 format")
    require(by_id["micron"]["request_bank_count"] == 8, "Micron request bank count drift")
    require(by_id["micron"]["request_program_count"] == 128, "Micron request Program count drift")
    require(by_id["micron"]["capabilities"]["bank_dump_request"] is False,
            "Micron bank request must stay disabled until verified")
    require(by_id["micron"]["capabilities"]["full_program_write_status"] == "unknown_disabled",
            "Micron full Program writes must stay disabled until verified")

    require(evidence["source"]["repository"] == "retroware/micronau", "Micronau provenance repository drift")
    require(evidence["source"]["commit"] == "0a1f14d8c5bce11663b464cef54e2035aba6ab1a",
            "Micronau evidence commit must remain pinned")
    require(evidence["source"]["archive_sha256"] == "4d127cdb6cc261d2e3f834eabe5cba3a49c578877e6783dea4a871a57233ad45",
            "Micronau source archive hash drift")
    require(evidence["program_transport"]["default_syx_sha256"] == "3316a957c026609b8afe6a3d4445382c85a95c2a1c931d31e17f9f65976cc761",
            "Micronau default.syx fixture hash drift")
    require(evidence["source"]["license"] == "GPL-2.0-or-later", "Micronau license boundary missing")
    require("not copied" in evidence["source"]["policy"], "clean implementation boundary must be explicit")
    require(evidence["device_request"]["micron_request_product_id"] == 0x26,
            "Micronau request evidence no longer matches Micron profile")

    xyz = evidence["micron_extensions"]["xyz_assignments"]
    require([item["decoded_byte_offset"] for item in xyz] == [162, 163, 164],
            "Micron X/Y/Z decoded offsets drift")
    require(evidence["micron_extensions"]["category"]["decoded_byte_offset"] == 86,
            "Micron category decoded offset drift")
    require(evidence["micron_extensions"]["fx1_fx2_balance"]["hardware_nrpn_after_micronau_offset"] == 230,
            "FX1/FX2 balance hardware NRPN drift")
    require(evidence["micron_extensions"]["fx2"]["hardware_selector_nrpn"] == 245,
            "Micron FX2 selector hardware NRPN drift")

    family_header = (ROOT / "Source/Midi/IonFamilyDevice.h").read_text(encoding="utf-8")
    sysex_codec = (ROOT / "Source/Midi/IonSysExCodec.cpp").read_text(encoding="utf-8")
    bar = (ROOT / "Source/UI/GlobalControlBar.cpp").read_text(encoding="utf-8")
    transfer = (ROOT / "Source/UI/HardwareTransferPanel.cpp").read_text(encoding="utf-8")
    settings = (ROOT / "Source/Core/AppSettings.cpp").read_text(encoding="utf-8")

    require('"micron"' in family_header and "0x26" in family_header,
            "C++ device profile must include Micron request ID 0x26")
    require("profile.requestProductId" in sysex_codec,
            "single Program request must take its product ID from the active device profile")
    require("IonFamilyDevice::micron" in sysex_codec,
            "SysEx request codec must distinguish Micron address validation")
    require("deviceSelector" in bar and '"Micron"' in bar,
            "global hardware context must expose an explicit Micron selector")
    require("supportsBankDumpRequest" in transfer and "Micron bank request not enabled" in transfer,
            "Micron bank requests must be visibly guarded")
    require("supportsIonEditBuffers" in transfer,
            "Ion-only edit-buffer write UI must be capability-gated")
    require('session.device_profile' in settings,
            "selected Ion-family hardware profile must persist as safe session context")

    sysex = load_json("data/protocol/ion-sysex.json")
    fields = {field["id"]: field for field in sysex["fields"]}
    for field_id, offset in {
        "performance_knob.x.assignment": 162,
        "performance_knob.y.assignment": 163,
        "performance_knob.z.assignment": 164,
    }.items():
        require(fields.get(field_id, {}).get("offset") == offset,
                f"existing shared Program map lost corroborated {field_id} offset {offset}")

    nrpn = load_json("data/protocol/ion-nrpn.json")
    numbers = {entry["number"]: entry for entry in nrpn["parameters"]}
    for number in (230, 245, 246, 247, 248, 249, 250):
        require(number in numbers, f"candidate NRPN table lost Micron-relevant NRPN {number}")

    print("PASS: Ion/Micron share the guarded 378-byte Program architecture")
    print("PASS: Ion request 0x22 / Micron request 0x26 device split is explicit")
    print("PASS: Micron-only bank/write behavior remains disabled until hardware verification")
    print("PASS: Micronau evidence is pinned, provenance-backed, and kept behind a clean implementation boundary")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
