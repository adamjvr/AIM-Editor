#!/usr/bin/env python3
"""Build and apply auditable hardware-verification evidence for AIM Editor.

Nothing is promoted merely because a candidate mapping exists.  A verification
record is produced only from raw/reconstructed capture evidence plus an explicit
human confirmation that the intended hardware control was isolated during the
experiment.  Applying an evidence record updates the canonical JSON datasets;
it never rewrites evidence or deletes candidate contradictions.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
from pathlib import Path
import sys
import time
from typing import Any, Iterable

from ion_nrpn_capture import analyze_capture

ROOT = Path(__file__).resolve().parents[2]
PARAMETERS_PATH = ROOT / "data" / "parameters.json"
NRPN_PATH = ROOT / "data" / "protocol" / "ion-nrpn.json"
SYSEX_PATH = ROOT / "data" / "protocol" / "ion-sysex.json"
VERIFICATION_DIR = ROOT / "research" / "verification"


def load_json(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError(f"{path}: JSON root must be an object")
    return data


def file_sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def canonical_digest(document: dict[str, Any]) -> str:
    body = copy.deepcopy(document)
    body.pop("evidence_id", None)
    encoded = json.dumps(body, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def seal(document: dict[str, Any]) -> dict[str, Any]:
    document = copy.deepcopy(document)
    document["evidence_id"] = "sha256:" + canonical_digest(document)
    return document


def verify_seal(document: dict[str, Any]) -> bool:
    evidence_id = document.get("evidence_id")
    return isinstance(evidence_id, str) and evidence_id == "sha256:" + canonical_digest(document)


def parameter_index(parameters: dict[str, Any]) -> dict[str, dict[str, Any]]:
    return {str(item["id"]): item for item in parameters.get("parameters", [])}


def nrpn_index(spec: dict[str, Any]) -> dict[str, dict[str, Any]]:
    return {str(item["id"]): item for item in spec.get("parameters", [])}


def sysex_index(spec: dict[str, Any]) -> dict[str, dict[str, Any]]:
    return {str(item["id"]): item for item in spec.get("fields", [])}


def _parameter_from_capture(capture: dict[str, Any]) -> str | None:
    context = capture.get("verification_context")
    if not isinstance(context, dict):
        return None
    value = context.get("parameter_id")
    return value if isinstance(value, str) and value else None


def _isolation_from_capture(capture: dict[str, Any]) -> bool:
    context = capture.get("verification_context")
    return bool(isinstance(context, dict) and context.get("user_confirmed_control_isolation") is True)


def _recommended_distinct_values(param: dict[str, Any]) -> int:
    kind = param.get("type")
    domain = param.get("domain") or {}
    lo, hi = domain.get("raw_min"), domain.get("raw_max")
    if kind == "boolean" or (isinstance(lo, (int, float)) and isinstance(hi, (int, float)) and hi - lo <= 1):
        return 2
    return 3


def build_nrpn_evidence(capture_path: Path,
                        parameter_id: str | None,
                        direction: str,
                        confirm_isolation: bool) -> dict[str, Any]:
    capture = load_json(capture_path)
    if capture.get("format") != "aim-editor.midi-capture":
        raise ValueError("NRPN verification requires aim-editor.midi-capture JSON")

    parameters = load_json(PARAMETERS_PATH)
    nrpn_spec = load_json(NRPN_PATH)
    params = parameter_index(parameters)
    nrpns = nrpn_index(nrpn_spec)

    tagged_parameter = _parameter_from_capture(capture)
    parameter_id = parameter_id or tagged_parameter
    if not parameter_id:
        raise ValueError("parameter id is required (use --parameter or tag the capture in AIM Editor)")
    if parameter_id not in params:
        raise ValueError(f"unknown semantic parameter id: {parameter_id}")
    if parameter_id not in nrpns:
        raise ValueError(f"no candidate NRPN entry for: {parameter_id}")

    param = params[parameter_id]
    candidate = nrpns[parameter_id]
    analysis = analyze_capture(capture, direction)
    transactions = analysis.get("transactions", [])
    expected_number = int(candidate["number"])
    matching = [item for item in transactions if int(item.get("nrpn", -1)) == expected_number]
    other_numbers = sorted({int(item["nrpn"]) for item in transactions if int(item.get("nrpn", -1)) != expected_number})
    semantic_values = [item.get("semantic_value") for item in matching if isinstance(item.get("semantic_value"), int)]
    distinct_values = sorted(set(semantic_values))
    recommended = _recommended_distinct_values(param)

    embedded_check = analysis.get("embedded_cross_check") or {}
    cross_check_ok = embedded_check.get("present") is not True or embedded_check.get("match") is True
    context_matches = tagged_parameter is None or tagged_parameter == parameter_id
    isolation = bool(confirm_isolation or _isolation_from_capture(capture))
    all_ranges_ok = bool(matching) and all(item.get("within_candidate_range") is not False for item in matching)
    mapping_ids_ok = all((item.get("parameter") or {}).get("id") == parameter_id for item in matching)
    enough_values = len(distinct_values) >= recommended
    no_competing_nrpn = not other_numbers

    checks = {
        "capture_parameter_tag_matches": context_matches,
        "candidate_nrpn_observed": bool(matching),
        "candidate_mapping_id_consistent": mapping_ids_ok,
        "embedded_decoder_cross_check": cross_check_ok,
        "values_within_candidate_range": all_ranges_ok,
        "distinct_values_observed": len(distinct_values),
        "recommended_distinct_values": recommended,
        "enough_distinct_values": enough_values,
        "competing_nrpn_numbers": other_numbers,
        "no_competing_nrpn": no_competing_nrpn,
        "user_confirmed_control_isolation": isolation,
    }
    promotable = all([
        context_matches,
        bool(matching),
        mapping_ids_ok,
        cross_check_ok,
        all_ranges_ok,
        enough_values,
        no_competing_nrpn,
        isolation,
    ])

    if promotable:
        result = "verified"
    elif matching and not mapping_ids_ok:
        result = "contradicted"
    elif matching:
        result = "supported"
    else:
        result = "inconclusive"

    evidence = {
        "$schema": "../../schemas/protocol-verification.schema.json",
        "format": "aim-editor.protocol-verification",
        "schema_version": 1,
        "generated_at_utc_ms": int(time.time() * 1000),
        "protocol": "nrpn",
        "evidence_scope": "mapping_address_and_value_encoding",
        "parameter_id": parameter_id,
        "result": result,
        "promotable": promotable,
        "user_confirmed_control_isolation": isolation,
        "candidate": {
            "nrpn": expected_number,
            "value_encoding": candidate.get("value_encoding"),
            "min": candidate.get("min"),
            "max": candidate.get("max"),
        },
        "sources": [{
            "path": str(capture_path),
            "sha256": file_sha256(capture_path),
            "format": capture.get("format"),
        }],
        "checks": checks,
        "observations": [{
            "utc_ms": item.get("utc_ms"),
            "direction": item.get("direction"),
            "device_identifier": item.get("device_identifier", ""),
            "midi_channel": item.get("channel"),
            "nrpn": item.get("nrpn"),
            "value14": item.get("encoded_value14"),
            "semantic_value": item.get("semantic_value"),
            "within_candidate_range": item.get("within_candidate_range"),
        } for item in matching],
    }
    return seal(evidence)


def build_sysex_evidence(diff_paths: list[Path],
                         parameter_id: str,
                         confirm_isolation: bool) -> dict[str, Any]:
    if len(diff_paths) < 2:
        raise ValueError("SysEx verification requires at least two independent A/B diff reports")

    parameters = load_json(PARAMETERS_PATH)
    sysex_spec = load_json(SYSEX_PATH)
    params = parameter_index(parameters)
    fields = sysex_index(sysex_spec)
    if parameter_id not in params:
        raise ValueError(f"unknown semantic parameter id: {parameter_id}")

    mapping = (params[parameter_id].get("protocol") or {}).get("sysex") or {}
    field_id = mapping.get("field_id") or parameter_id
    if field_id not in fields:
        raise ValueError(f"no candidate SysEx field for {parameter_id}: {field_id}")
    candidate = fields[field_id]

    reports = [load_json(path) for path in diff_paths]
    observations: list[dict[str, Any]] = []
    report_checks: list[dict[str, Any]] = []
    all_clean = True

    for path, report in zip(diff_paths, reports):
        if report.get("format") != "aim-editor.ion-patch-diff":
            raise ValueError(f"{path}: not an AIM Ion patch-diff report")
        field_changes = report.get("candidate_field_changes") or []
        expected_change = next((item for item in field_changes if item.get("field_id") == field_id), None)
        summary = report.get("summary") or {}
        identity = report.get("patch_identity") or {}
        checksum_ok = identity.get("before_checksum_valid") is True and identity.get("after_checksum_valid") is True
        clean_single = summary.get("confidence_hint") == "single_candidate_field_changed"
        no_unmapped = int(summary.get("unmapped_changed_byte_count", len(report.get("unmapped_changed_byte_offsets") or []))) == 0
        expected_only = len(field_changes) == 1 and expected_change is not None
        this_clean = bool(checksum_ok and clean_single and no_unmapped and expected_only)
        all_clean = all_clean and this_clean
        report_checks.append({
            "path": str(path),
            "checksum_valid": checksum_ok,
            "single_semantic_candidate_field": clean_single,
            "no_unmapped_changed_bytes": no_unmapped,
            "expected_field_is_only_semantic_change": expected_only,
        })
        if expected_change:
            observations.append({
                "source_path": str(path),
                "field_id": field_id,
                "before": expected_change.get("before"),
                "after": expected_change.get("after"),
                "changed_byte_offsets": expected_change.get("changed_byte_offsets", []),
            })

    transitions = {(json.dumps(item.get("before"), sort_keys=True), json.dumps(item.get("after"), sort_keys=True)) for item in observations}
    enough_independent = len(observations) >= 2 and len(transitions) >= 2
    isolation = bool(confirm_isolation)
    promotable = bool(all_clean and enough_independent and isolation)
    if promotable:
        result = "verified"
    elif observations and all_clean:
        result = "supported"
    elif observations:
        result = "contradicted"
    else:
        result = "inconclusive"

    evidence = {
        "$schema": "../../schemas/protocol-verification.schema.json",
        "format": "aim-editor.protocol-verification",
        "schema_version": 1,
        "generated_at_utc_ms": int(time.time() * 1000),
        "protocol": "sysex",
        "evidence_scope": "field_address_and_encoding",
        "parameter_id": parameter_id,
        "result": result,
        "promotable": promotable,
        "user_confirmed_control_isolation": isolation,
        "candidate": {
            "field_id": field_id,
            "offset": candidate.get("offset"),
            "width_bytes": candidate.get("width_bytes"),
            "kind": candidate.get("kind"),
            "mask": candidate.get("mask"),
            "shift": candidate.get("shift"),
        },
        "sources": [{
            "path": str(path),
            "sha256": file_sha256(path),
            "format": report.get("format"),
        } for path, report in zip(diff_paths, reports)],
        "checks": {
            "reports": report_checks,
            "all_reports_clean": all_clean,
            "independent_transition_count": len(transitions),
            "enough_independent_transitions": enough_independent,
            "user_confirmed_control_isolation": isolation,
        },
        "observations": observations,
    }
    return seal(evidence)


def _append_unique(container: dict[str, Any], key: str, value: str) -> None:
    items = container.setdefault(key, [])
    if not isinstance(items, list):
        raise ValueError(f"{key} is not an array")
    if value not in items:
        items.append(value)


def _overall_parameter_status(protocol: dict[str, Any]) -> str:
    nrpn_present = protocol.get("nrpn") is not None
    sysex_present = ((protocol.get("sysex") or {}).get("offset") is not None)
    if not nrpn_present and not sysex_present:
        return "unmapped"
    nrpn_ok = (not nrpn_present) or ((protocol.get("nrpn_evidence") or {}).get("status") == "verified")
    sysex_ok = (not sysex_present) or ((protocol.get("evidence") or {}).get("status") == "verified")
    return "verified" if nrpn_ok and sysex_ok else "candidate"


def apply_evidence_to_documents(evidence: dict[str, Any],
                                evidence_ref: str,
                                parameters: dict[str, Any],
                                nrpn_spec: dict[str, Any],
                                sysex_spec: dict[str, Any]) -> None:
    if evidence.get("format") != "aim-editor.protocol-verification":
        raise ValueError("unsupported evidence format")
    if evidence.get("result") != "verified" or evidence.get("promotable") is not True:
        raise ValueError("evidence is not promotable/verified")
    if evidence.get("user_confirmed_control_isolation") is not True:
        raise ValueError("evidence lacks explicit control-isolation confirmation")
    if not verify_seal(evidence):
        raise ValueError("evidence_id does not match evidence contents")

    pid = str(evidence.get("parameter_id", ""))
    params = parameter_index(parameters)
    if pid not in params:
        raise ValueError(f"parameter not found in canonical database: {pid}")
    param = params[pid]
    protocol = param["protocol"]

    if evidence.get("protocol") == "nrpn":
        entries = nrpn_index(nrpn_spec)
        if pid not in entries:
            raise ValueError(f"candidate NRPN entry missing for {pid}")
        entry = entries[pid]
        candidate = evidence.get("candidate") or {}
        if int(entry["number"]) != int(candidate.get("nrpn", -1)):
            raise ValueError("evidence NRPN no longer matches canonical candidate")
        if entry.get("value_encoding") != candidate.get("value_encoding"):
            raise ValueError("evidence value encoding no longer matches canonical candidate")
        entry["status"] = "verified"
        entry["hardware_verified"] = True
        _append_unique(entry, "verification_evidence", evidence_ref)

        nrpn_evidence = protocol.setdefault("nrpn_evidence", {})
        nrpn_evidence["status"] = "verified"
        nrpn_evidence["hardware_verified"] = True
        _append_unique(nrpn_evidence, "verification_evidence", evidence_ref)

    elif evidence.get("protocol") == "sysex":
        fields = sysex_index(sysex_spec)
        candidate = evidence.get("candidate") or {}
        field_id = str(candidate.get("field_id", ""))
        if field_id not in fields:
            raise ValueError(f"candidate SysEx field missing: {field_id}")
        field = fields[field_id]
        mapping = protocol.get("sysex") or {}
        if mapping.get("field_id") != field_id:
            raise ValueError("evidence field no longer matches parameter database")
        for key in ("offset", "width_bytes", "mask", "shift"):
            evidence_value = candidate.get(key)
            canonical_value = field.get(key)
            if evidence_value is not None and canonical_value != evidence_value:
                raise ValueError(f"evidence {key} no longer matches canonical SysEx field")
        field["status"] = "verified"
        field["hardware_verified"] = True
        _append_unique(field, "verification_evidence", evidence_ref)

        sysex_evidence = protocol.setdefault("evidence", {})
        sysex_evidence["status"] = "verified"
        sysex_evidence["hardware_verified"] = True
        _append_unique(sysex_evidence, "verification_evidence", evidence_ref)
    else:
        raise ValueError(f"unsupported protocol: {evidence.get('protocol')}")

    protocol["status"] = _overall_parameter_status(protocol)


def command_nrpn(args: argparse.Namespace) -> int:
    evidence = build_nrpn_evidence(args.capture, args.parameter, args.direction, args.confirm_isolation)
    _write_or_print(evidence, args.output)
    return 0 if evidence["result"] == "verified" else 1


def command_sysex(args: argparse.Namespace) -> int:
    evidence = build_sysex_evidence(args.diffs, args.parameter, args.confirm_isolation)
    _write_or_print(evidence, args.output)
    return 0 if evidence["result"] == "verified" else 1


def _write_or_print(document: dict[str, Any], output: Path | None) -> None:
    text = json.dumps(document, indent=2, ensure_ascii=False) + "\n"
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)


def command_apply(args: argparse.Namespace) -> int:
    evidence_path = args.evidence.resolve()
    verification_root = VERIFICATION_DIR.resolve()
    if verification_root not in evidence_path.parents:
        raise ValueError("promotion evidence must be committed under research/verification/")
    evidence = load_json(evidence_path)
    evidence_ref = evidence_path.relative_to(ROOT.resolve()).as_posix()

    parameters = load_json(PARAMETERS_PATH)
    nrpn_spec = load_json(NRPN_PATH)
    sysex_spec = load_json(SYSEX_PATH)
    apply_evidence_to_documents(evidence, evidence_ref, parameters, nrpn_spec, sysex_spec)

    PARAMETERS_PATH.write_text(json.dumps(parameters, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    NRPN_PATH.write_text(json.dumps(nrpn_spec, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    SYSEX_PATH.write_text(json.dumps(sysex_spec, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"PASS: promoted {evidence['protocol']} mapping for {evidence['parameter_id']} from {evidence_ref}")
    return 0


def command_self_test(_: argparse.Namespace) -> int:
    import tempfile

    # Build a raw capture containing three values for one isolated candidate NRPN.
    # NRPN 47 maps to filter1.env_amount and uses signed-14 wrapping.
    values = (-100, 0, 100)
    events: list[dict[str, Any]] = []
    utc = 0
    for semantic in values:
        encoded = semantic if semantic >= 0 else semantic + 16384
        for raw in ([0xB0, 99, 0], [0xB0, 98, 47], [0xB0, 6, (encoded >> 7) & 0x7F], [0xB0, 38, encoded & 0x7F]):
            events.append({
                "direction": "input", "utc_ms": utc, "device_identifier": "synthetic-ion",
                "kind": "control_change", "size": 3,
                "hex": " ".join(f"{byte:02X}" for byte in raw), "bytes": raw,
            })
            utc += 1
    capture = {
        "format": "aim-editor.midi-capture", "schema_version": 1, "exported_at_utc_ms": 0,
        "verification_context": {
            "parameter_id": "filter1.env_amount", "parameter_known": True,
            "user_confirmed_control_isolation": True, "note": "synthetic self-test",
        },
        "events": events,
    }

    with tempfile.TemporaryDirectory() as td:
        capture_path = Path(td) / "capture.json"
        capture_path.write_text(json.dumps(capture), encoding="utf-8")
        evidence = build_nrpn_evidence(capture_path, None, "input", False)
        assert evidence["result"] == "verified" and evidence["promotable"] is True
        assert verify_seal(evidence)

        parameters = load_json(PARAMETERS_PATH)
        nrpn_spec = load_json(NRPN_PATH)
        sysex_spec = load_json(SYSEX_PATH)
        apply_evidence_to_documents(evidence, "research/verification/self-test.json", parameters, nrpn_spec, sysex_spec)
        nrpn_entry = nrpn_index(nrpn_spec)["filter1.env_amount"]
        param = parameter_index(parameters)["filter1.env_amount"]
        assert nrpn_entry["status"] == "verified" and nrpn_entry["hardware_verified"] is True
        assert param["protocol"]["nrpn_evidence"]["status"] == "verified"
        # SysEx remains candidate, so the aggregate parameter status remains candidate.
        assert param["protocol"]["status"] == "candidate"

        fake_diffs: list[Path] = []
        for index, pair in enumerate(((10, 20), (20, 30))):
            path = Path(td) / f"diff-{index}.json"
            report = {
                "format": "aim-editor.ion-patch-diff", "schema_version": 1,
                "patch_identity": {"before_checksum_valid": True, "after_checksum_valid": True},
                "summary": {"confidence_hint": "single_candidate_field_changed", "unmapped_changed_byte_count": 0},
                "candidate_field_changes": [{"field_id": "filter1.env_amount", "before": pair[0], "after": pair[1], "changed_byte_offsets": [128]}],
                "derived_field_changes": [{"field_id": "header.checksum"}],
                "unmapped_changed_byte_offsets": [],
            }
            path.write_text(json.dumps(report), encoding="utf-8")
            fake_diffs.append(path)
        sysex_evidence = build_sysex_evidence(fake_diffs, "filter1.env_amount", True)
        assert sysex_evidence["result"] == "verified" and verify_seal(sysex_evidence)
        apply_evidence_to_documents(sysex_evidence, "research/verification/self-test-sysex.json", parameters, nrpn_spec, sysex_spec)
        assert parameter_index(parameters)["filter1.env_amount"]["protocol"]["status"] == "verified"

    print("PASS: protocol verification evidence/promotion self-test")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("nrpn", help="build evidence from a controlled NRPN capture")
    p.add_argument("capture", type=Path)
    p.add_argument("--parameter", help="semantic parameter id; defaults to capture verification tag")
    p.add_argument("--direction", choices=["input", "output", "all"], default="input")
    p.add_argument("--confirm-isolation", action="store_true", help="confirm that only the named hardware control was deliberately moved")
    p.add_argument("--output", "-o", type=Path)
    p.set_defaults(func=command_nrpn)

    p = sub.add_parser("sysex", help="build evidence from repeated controlled patch-diff reports")
    p.add_argument("diffs", nargs="+", type=Path)
    p.add_argument("--parameter", required=True, help="semantic parameter id")
    p.add_argument("--confirm-isolation", action="store_true", help="confirm each A/B pair changed only the named hardware control")
    p.add_argument("--output", "-o", type=Path)
    p.set_defaults(func=command_sysex)

    p = sub.add_parser("apply", help="promote canonical JSON from a sealed verified evidence record")
    p.add_argument("evidence", type=Path)
    p.set_defaults(func=command_apply)

    p = sub.add_parser("self-test")
    p.set_defaults(func=command_self_test)

    args = parser.parse_args()
    try:
        return args.func(args)
    except (OSError, json.JSONDecodeError, ValueError, KeyError, AssertionError) as exc:
        parser.error(str(exc))
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
