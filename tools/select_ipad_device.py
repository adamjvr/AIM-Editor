#!/usr/bin/env python3
"""Select one connected physical iPad from Xcode's CoreDevice inventory."""
from __future__ import annotations

import argparse
import json
import os
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path


def load_devices() -> list[dict]:
    with tempfile.NamedTemporaryFile(prefix="aim-devicectl-", suffix=".json", delete=False) as handle:
        output_path = Path(handle.name)
    try:
        completed = subprocess.run(
            ["xcrun", "devicectl", "list", "devices", "--json-output", str(output_path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
        )
        if completed.returncode != 0:
            raise RuntimeError(completed.stdout.strip() or "devicectl list devices failed")
        payload = json.loads(output_path.read_text(encoding="utf-8"))
        devices = payload.get("result", {}).get("devices", [])
        return devices if isinstance(devices, list) else []
    finally:
        output_path.unlink(missing_ok=True)


def normalized_device(device: dict) -> dict | None:
    hw = device.get("hardwareProperties") or {}
    props = device.get("deviceProperties") or {}
    conn = device.get("connectionProperties") or {}

    # Xcode 27 moves these dictionaries under `properties`; retain compatibility
    # without weakening the Xcode 26 path used by the current project gate.
    aggregate = device.get("properties") or {}
    if not hw and isinstance(aggregate, dict):
        hw = aggregate.get("hardwareProperties") or aggregate.get("hardware") or aggregate
    if not props and isinstance(aggregate, dict):
        props = aggregate.get("deviceProperties") or aggregate.get("device") or aggregate
    if not conn and isinstance(aggregate, dict):
        conn = aggregate.get("connectionProperties") or aggregate.get("connection") or aggregate

    device_type = str(hw.get("deviceType") or "").lower()
    product_type = str(hw.get("productType") or "").lower()
    reality = str(hw.get("reality") or "physical").lower()
    platform_name = str(hw.get("platform") or "").lower()

    if reality == "simulated":
        return None
    if device_type != "ipad" and not product_type.startswith("ipad"):
        return None
    if platform_name and platform_name not in {"ios", "ipados"}:
        return None

    pairing = str(conn.get("pairingState") or "").lower()
    tunnel = str(conn.get("tunnelState") or device.get("state") or "").lower()
    transport = str(conn.get("transportType") or "").lower()
    boot = str(props.get("bootState") or "").lower()
    ddi = props.get("ddiServicesAvailable") is True

    online = (
        tunnel in {"connected", "available"}
        or tunnel.startswith("available")
        or ddi
        or (pairing == "paired" and boot == "booted" and transport in {"wired", "localnetwork", "network"})
    )

    udid = str(hw.get("udid") or device.get("udid") or "")
    identifier = str(device.get("identifier") or udid)
    if not udid:
        return None

    return {
        "udid": udid,
        "identifier": identifier,
        "name": str(props.get("name") or "Connected iPad"),
        "product_type": str(hw.get("productType") or "iPad"),
        "marketing_name": str(hw.get("marketingName") or ""),
        "os_version": str(props.get("osVersionNumber") or ""),
        "transport": str(conn.get("transportType") or ""),
        "developer_mode": str(props.get("developerModeStatus") or ""),
        "pairing_state": pairing,
        "tunnel_state": tunnel,
        "boot_state": boot,
        "ddi_services_available": ddi,
        "online": online and (not pairing or pairing == "paired"),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--device", default=os.environ.get("AIM_EDITOR_IPAD_DEVICE", ""), help="iPad UDID, CoreDevice identifier, or exact device name")
    parser.add_argument("--shell", action="store_true", help="emit shell assignments for the selected device")
    args = parser.parse_args()

    try:
        inventory = [item for item in (normalized_device(d) for d in load_devices()) if item is not None]
    except Exception as exc:
        print(f"ERROR: unable to query physical Apple devices: {exc}", file=sys.stderr)
        return 2

    selector = args.device.strip()
    if selector:
        matches = [d for d in inventory if selector in {d["udid"], d["identifier"], d["name"]}]
        if not matches:
            print(f"ERROR: requested physical iPad {selector!r} is not present in the CoreDevice inventory.", file=sys.stderr)
            print("Check `xcrun devicectl list devices`, then reconnect/trust the iPad if it is absent.", file=sys.stderr)
            return 2
        candidates = matches
    else:
        candidates = [d for d in inventory if d["online"]]

    if not candidates:
        print("ERROR: no connected physical iPad is available through devicectl.", file=sys.stderr)
        print("Connect/unlock the iPad, trust this Mac, enable Developer Mode, and ensure it appears in Xcode Device Hub.", file=sys.stderr)
        return 2

    if len(candidates) > 1:
        print("ERROR: multiple physical iPads are available; select one with AIM_EDITOR_IPAD_DEVICE=<UDID-or-name>.", file=sys.stderr)
        for item in candidates:
            print(f"  {item['name']}  {item['udid']}  {item['product_type']}  iPadOS {item['os_version']}", file=sys.stderr)
        return 2

    selected = candidates[0]
    if not selected["online"]:
        print(f"ERROR: requested iPad {selected['name']!r} ({selected['udid']}) is known to CoreDevice but is not currently available for development.", file=sys.stderr)
        print(
            "State: "
            f"pairing={selected['pairing_state'] or 'unknown'}, "
            f"tunnel={selected['tunnel_state'] or 'unknown'}, "
            f"boot={selected['boot_state'] or 'unknown'}, "
            f"transport={selected['transport'] or 'unknown'}, "
            f"DDI={'available' if selected['ddi_services_available'] else 'unavailable'}",
            file=sys.stderr,
        )
        print("Unlock/reconnect the iPad and confirm it appears as available in Xcode Devices and Simulators.", file=sys.stderr)
        return 2

    dev_mode = selected["developer_mode"].lower()
    if dev_mode and dev_mode != "enabled":
        print(f"ERROR: Developer Mode is {selected['developer_mode']!r} on {selected['name']}; enable it before installing development builds.", file=sys.stderr)
        return 2

    if args.shell:
        pairs = {
            "AIM_IPAD_UDID": selected["udid"],
            "AIM_IPAD_IDENTIFIER": selected["identifier"],
            "AIM_IPAD_NAME": selected["name"],
            "AIM_IPAD_PRODUCT_TYPE": selected["product_type"],
            "AIM_IPAD_OS_VERSION": selected["os_version"],
            "AIM_IPAD_TRANSPORT": selected["transport"],
        }
        for key, value in pairs.items():
            print(f"{key}={shlex.quote(value)}")
    else:
        print(json.dumps(selected, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
