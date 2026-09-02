#!/usr/bin/env python3
"""Generate a deterministic hardware-verification queue from canonical AIM Editor JSON.

This tool never promotes mappings. It turns the current candidate/verified state
into a practical next-experiment list so hardware work is repeatable rather than
ad hoc.
"""
from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
PARAMETERS = ROOT / "data" / "parameters.json"

SECTION_ORDER = {
    "osc1": 0, "osc2": 1, "osc3": 2,
    "pre_filter_mix": 3,
    "filter1": 4, "filter2": 5,
    "post_filter_mix": 6,
    "output": 7,
    "env.pitch": 8, "env.filter": 9, "env.amp": 10,
    "lfo1": 11, "lfo2": 12, "sample_hold": 13,
    "tempo_arp": 14, "voice": 15, "effects": 16,
    "mod_matrix": 17, "tracking_generator": 18,
}
PAGE_ORDER = {"front": 0, "dual1": 1, "dual2": 2, "rear": 3, "randomizer": 4}


def load_parameters() -> list[dict[str, Any]]:
    root = json.loads(PARAMETERS.read_text(encoding="utf-8"))
    return root["parameters"]


def status_for(parameter: dict[str, Any], transport: str) -> str:
    protocol = parameter.get("protocol", {})
    if transport == "nrpn":
        if protocol.get("nrpn") is None:
            return "unmapped"
        return protocol.get("nrpn_evidence", {}).get("status", protocol.get("status", "candidate"))
    sysex = protocol.get("sysex")
    if not isinstance(sysex, dict) or sysex.get("offset") is None:
        return "unmapped"
    return protocol.get("evidence", {}).get("status", protocol.get("status", "candidate"))


def mapping_for(parameter: dict[str, Any], transport: str) -> dict[str, Any]:
    protocol = parameter.get("protocol", {})
    if transport == "nrpn":
        evidence = protocol.get("nrpn_evidence", {})
        return {
            "nrpn": protocol.get("nrpn"),
            "value_encoding": evidence.get("value_encoding"),
            "candidate_min": evidence.get("min"),
            "candidate_max": evidence.get("max"),
        }
    sysex = protocol.get("sysex", {})
    return {
        "offset": sysex.get("offset"),
        "bits": sysex.get("bits"),
        "encoding": sysex.get("encoding"),
        "mask": sysex.get("mask"),
        "shift": sysex.get("shift"),
        "width_bytes": sysex.get("width_bytes"),
    }


def suggested_values(parameter: dict[str, Any], transport: str) -> list[int]:
    # NRPN candidate domains occasionally use a signed semantic range while the
    # canonical stored/raw patch domain is offset (for example octave 0..6 vs
    # NRPN -3..+3). Prefer the transport evidence range when it exists so the
    # generated hardware procedure never recommends an out-of-range NRPN value.
    if transport == "nrpn":
        evidence = parameter.get("protocol", {}).get("nrpn_evidence", {})
        lo = evidence.get("min")
        hi = evidence.get("max")
        if isinstance(lo, int) and isinstance(hi, int):
            mid = lo + (hi - lo) // 2
            return list(dict.fromkeys([lo, mid, hi]))

    domain = parameter.get("domain", {})
    values = domain.get("values")
    if isinstance(values, list) and values:
        raws = sorted({int(item["raw"]) for item in values if isinstance(item, dict) and "raw" in item})
        if raws:
            picks = [raws[0], raws[len(raws) // 2], raws[-1]]
            return list(dict.fromkeys(picks))
    lo = domain.get("raw_min")
    hi = domain.get("raw_max")
    if isinstance(lo, int) and isinstance(hi, int):
        mid = lo + (hi - lo) // 2
        return list(dict.fromkeys([lo, mid, hi]))
    return []


def priority_key(parameter: dict[str, Any], transport: str) -> tuple[Any, ...]:
    pages = parameter.get("ui", {}).get("pages", []) or []
    page_rank = min((PAGE_ORDER.get(page, 99) for page in pages), default=99)
    section = parameter.get("section", "")
    section_rank = SECTION_ORDER.get(section, 90)
    # Prefer mappings present in both transports because the same physical
    # control can contribute useful evidence to both protocol layers.
    both = status_for(parameter, "nrpn") != "unmapped" and status_for(parameter, "sysex") != "unmapped"
    domain = parameter.get("domain", {})
    enum_values = domain.get("values")
    easy_enum = isinstance(enum_values, list) and len(enum_values) > 1
    return (page_rank, section_rank, 0 if both else 1, 0 if easy_enum else 1, parameter.get("id", ""), transport)


def make_item(parameter: dict[str, Any], transport: str, ordinal: int) -> dict[str, Any]:
    status = status_for(parameter, transport)
    mapping = mapping_for(parameter, transport)
    pages = parameter.get("ui", {}).get("pages", []) or []
    steps = [
        "Connect Ion MIDI input/output and select the correct editor MIDI channel.",
        f"Start a controlled {transport.upper()} test for {parameter['id']}.",
        "Move only this physical control through at least three distinct values.",
        "Stop/freeze the test before touching another control and export the capture JSON.",
    ]
    if transport == "sysex":
        steps = [
            "Request/save a checksum-valid baseline patch dump.",
            f"Change only {parameter['id']} on the hardware.",
            "Request/save the changed patch dump before touching any other control.",
            "Repeat with an independent value pair; SysEx promotion requires multiple clean A/B tests.",
        ]

    return {
        "ordinal": ordinal,
        "parameter_id": parameter["id"],
        "name": parameter.get("name", parameter["id"]),
        "section": parameter.get("section"),
        "pages": pages,
        "transport": transport,
        "current_status": status,
        "mapping": mapping,
        "raw_domain": {
            "kind": parameter.get("domain", {}).get("kind"),
            "min": parameter.get("domain", {}).get("raw_min"),
            "max": parameter.get("domain", {}).get("raw_max"),
        },
        "suggested_distinct_raw_values": suggested_values(parameter, transport),
        "procedure": steps,
    }


def build_plan(transport: str, section: str | None, page: str | None, include_verified: bool) -> dict[str, Any]:
    parameters = load_parameters()
    candidates: list[dict[str, Any]] = []
    for parameter in parameters:
        status = status_for(parameter, transport)
        if status == "unmapped":
            continue
        if not include_verified and status == "verified":
            continue
        if section and parameter.get("section") != section:
            continue
        pages = parameter.get("ui", {}).get("pages", []) or []
        if page and page not in pages:
            continue
        candidates.append(parameter)

    candidates.sort(key=lambda item: priority_key(item, transport))
    items = [make_item(parameter, transport, index + 1) for index, parameter in enumerate(candidates)]
    counts = {"candidate": 0, "verified": 0, "other": 0}
    for item in items:
        status = item["current_status"]
        if status in counts:
            counts[status] += 1
        else:
            counts["other"] += 1

    return {
        "format": "aim-editor.verification-plan",
        "schema_version": 1,
        "transport": transport,
        "filters": {"section": section, "page": page, "include_verified": include_verified},
        "counts": counts,
        "items": items,
    }


def to_markdown(plan: dict[str, Any], limit: int | None = None) -> str:
    items = plan["items"] if limit is None else plan["items"][:limit]
    title = f"# AIM Editor {plan['transport'].upper()} hardware-verification plan"
    lines = [title, "", "Generated from canonical `data/parameters.json`. This file is a work queue, not evidence and not a promotion authority.", ""]
    filters = plan["filters"]
    if filters.get("section") or filters.get("page"):
        lines += [f"Filters: section={filters.get('section') or '*'}, page={filters.get('page') or '*'}", ""]
    lines += [f"Queued experiments: **{len(plan['items'])}**", ""]
    for item in items:
        lines.append(f"## {item['ordinal']:03d} — `{item['parameter_id']}` — {item['name']}")
        lines.append("")
        lines.append(f"- Section: `{item['section']}`")
        lines.append(f"- Pages: {', '.join('`'+p+'`' for p in item['pages']) if item['pages'] else 'none'}")
        lines.append(f"- Current status: **{item['current_status']}**")
        mapping = ", ".join(f"{k}={v}" for k, v in item["mapping"].items() if v is not None)
        lines.append(f"- Candidate mapping: `{mapping or 'none'}`")
        values = item["suggested_distinct_raw_values"]
        if values:
            lines.append(f"- Suggested distinct raw test values: `{', '.join(map(str, values))}`")
        lines.append("")
        for index, step in enumerate(item["procedure"], 1):
            lines.append(f"{index}. {step}")
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def self_test() -> int:
    nrpn = build_plan("nrpn", None, None, False)
    sysex = build_plan("sysex", None, None, False)
    if len(nrpn["items"]) != 194:
        raise RuntimeError(f"expected 194 mapped NRPN verification items, got {len(nrpn['items'])}")
    if len(sysex["items"]) != 185:
        raise RuntimeError(f"expected 185 mapped SysEx verification items, got {len(sysex['items'])}")
    if not nrpn["items"] or nrpn["items"][0]["current_status"] not in {"candidate", "verified"}:
        raise RuntimeError("NRPN plan ordering/status invalid")
    if "procedure" not in sysex["items"][0] or len(sysex["items"][0]["procedure"]) < 4:
        raise RuntimeError("SysEx plan did not include controlled A/B procedure")
    octave = next(item for item in nrpn["items"] if item["parameter_id"] == "osc1.octave")
    if octave["suggested_distinct_raw_values"] != [-3, 0, 3]:
        raise RuntimeError("NRPN planner ignored the transport-specific signed octave range")
    json.dumps(nrpn)
    to_markdown(nrpn, 2)
    print("PASS: Ion hardware-verification plan self-test")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)

    plan_parser = sub.add_parser("plan")
    plan_parser.add_argument("--transport", choices=("nrpn", "sysex"), default="nrpn")
    plan_parser.add_argument("--section")
    plan_parser.add_argument("--page", choices=tuple(PAGE_ORDER))
    plan_parser.add_argument("--include-verified", action="store_true")
    plan_parser.add_argument("--format", choices=("json", "markdown"), default="markdown")
    plan_parser.add_argument("--limit", type=int)
    plan_parser.add_argument("--output", type=Path)

    next_parser = sub.add_parser("next")
    next_parser.add_argument("--transport", choices=("nrpn", "sysex"), default="nrpn")
    next_parser.add_argument("--section")
    next_parser.add_argument("--page", choices=tuple(PAGE_ORDER))
    next_parser.add_argument("--format", choices=("json", "markdown"), default="markdown")

    sub.add_parser("self-test")
    args = parser.parse_args()

    if args.command == "self-test":
        return self_test()

    if args.command == "next":
        plan = build_plan(args.transport, args.section, args.page, False)
        plan["items"] = plan["items"][:1]
        if args.format == "json":
            output = json.dumps(plan, indent=2) + "\n"
        else:
            output = to_markdown(plan, 1)
        sys.stdout.write(output)
        return 0 if plan["items"] else 1

    plan = build_plan(args.transport, args.section, args.page, args.include_verified)
    if args.limit is not None and args.limit < 0:
        parser.error("--limit must be >= 0")
    if args.format == "json":
        serializable = dict(plan)
        if args.limit is not None:
            serializable["items"] = serializable["items"][:args.limit]
        output = json.dumps(serializable, indent=2) + "\n"
    else:
        output = to_markdown(plan, args.limit)

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(output, encoding="utf-8")
    else:
        sys.stdout.write(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
