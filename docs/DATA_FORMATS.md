# Data formats

AIM Editor treats JSON as a public interface, not an implementation detail.

## Parameter database

`data/parameters.json` describes the semantic Ion parameter surface. Every entry has a stable ID, human name, section, value-domain state, display metadata, protocol mapping state, and UI placement hints.

Protocol values begin as `null`. Once evidence exists they may become `candidate`; only repeatable independent/hardware confirmation promotes them to `verified`. SysEx and NRPN evidence are kept transport-specific because their raw value domains can differ.

Mapping confidence progresses through:

- `unmapped`
- `candidate`
- `verified`

Example:

```json
{
  "id": "filter1.frequency",
  "name": "Filter 1 Frequency",
  "section": "filter1",
  "type": "continuous",
  "domain": {
    "kind": "continuous",
    "raw_min": 0,
    "raw_max": 1023,
    "default_raw": null
  },
  "display": {
    "unit": "Hz",
    "transform": "x==1023 ? 20000Hz : exp(x/147.933647)*20Hz"
  },
  "protocol": {
    "status": "candidate",
    "nrpn": 44,
    "nrpn_evidence": {
      "source": "data/protocol/ion-nrpn.json",
      "status": "candidate",
      "hardware_verified": false,
      "min": 0,
      "max": 1023,
      "value_encoding": "unsigned_14",
      "source_id": "filter1.frequency"
    },
    "sysex": {
      "offset": 126,
      "bits": 16,
      "encoding": "s16be",
      "field_id": "filter1.frequency"
    }
  },
  "ui": {
    "pages": ["front", "dual1", "dual2"],
    "control": "knob"
  }
}
```

## Program JSON

Runtime parameter IDs are dotted, but exported program JSON nests them to remain pleasant to read and diff.

Internal ID:

```text
filter1.frequency
```

Export:

```json
{
  "filter1": {
    "frequency": 101
  }
}
```

A full document:

```json
{
  "format": "aim-editor.program",
  "schema_version": 1,
  "name": "Example Program",
  "category": "Lead",
  "parameters": {
    "filter1": {
      "frequency": 101,
      "resonance": 54
    },
    "voice": {
      "unison": 2
    }
  },
  "unmapped_bytes": [
    {
      "offset": 206,
      "value": 37
    }
  ]
}
```

`unmapped_bytes` is deliberately part of the format so incomplete reverse engineering never forces lossy round trips.

## Future formats

Planned formats use the same design principles:

- `aim-editor.bank`
- `aim-editor.setup`
- `aim-editor.protocol-evidence`
- raw/captured SysEx fixture manifests
- transport-verification fixture sets

Native JSON documents will coexist with hardware-compatible `.syx` import/export.
