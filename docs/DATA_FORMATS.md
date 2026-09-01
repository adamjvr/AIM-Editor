# Data formats

AIM Editor treats JSON as a public interface, not an implementation detail.

## Parameter database

`data/parameters.json` describes the semantic Ion parameter surface. Every entry has a stable ID, human name, section, value-domain state, display metadata, protocol mapping state, and UI placement hints.

Protocol values begin as `null`. A reverse-engineering discovery should not become a concrete number until evidence exists.

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
    "raw_min": null,
    "raw_max": null,
    "default_raw": null
  },
  "display": {
    "unit": "Hz",
    "transform": "unknown"
  },
  "protocol": {
    "status": "unmapped",
    "nrpn": null,
    "sysex": {
      "offset": null,
      "bits": null,
      "encoding": null
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

Native JSON documents will coexist with hardware-compatible `.syx` import/export.
