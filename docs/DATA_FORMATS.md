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

## Program JSON — `.aimprogram.json`

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

A native program document is intentionally self-describing:

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
  ],
  "source_patch": null
}
```

`unmapped_bytes` holds specifically identified unknown bytes/fields.

When a program originated from a real Ion patch dump, `source_patch` additionally preserves the **complete 378-byte decoded patch image**:

```json
"source_patch": {
  "format": "ion-decoded-patch-v1",
  "bytes": [0, 14, 34, 1, 3, 0, 42, "... 371 more byte values ..."]
}
```

That deliberate redundancy matters. It means a human-readable JSON program can retain every unknown byte and unknown bit while reverse engineering is incomplete. AIM Editor can then overlay the semantic parameters it understands onto the original image and recalculate the checksum when exporting `.syx`.

A JSON-only program with `source_patch: null` remains fully useful in AIM Editor, but hardware `.syx` export is disabled until a real patch template is available. AIM Editor does not fabricate unknown hardware data.

## Bank JSON — `.aimbank.json`

Native banks are sparse 128-slot containers. Empty slots remain empty rather than being filled with invented patches.

```json
{
  "format": "aim-editor.bank",
  "schema_version": 1,
  "name": "My Ion Bank",
  "hardware_bank": "yellow",
  "programs": [
    {
      "slot": 0,
      "program": {
        "format": "aim-editor.program",
        "schema_version": 1,
        "name": "Bass One",
        "category": "Bass",
        "parameters": {},
        "unmapped_bytes": [],
        "source_patch": null
      }
    },
    {
      "slot": 127,
      "program": {
        "format": "aim-editor.program",
        "schema_version": 1,
        "name": "Last Patch",
        "category": "Pad",
        "parameters": {},
        "unmapped_bytes": [],
        "source_patch": null
      }
    }
  ]
}
```

`hardware_bank` is one of `red`, `green`, `blue`, `yellow`, `edit`, or `null`. It is metadata; native banks do not require a hardware destination.

A bank may therefore be used purely as a portable librarian collection. Hardware bank `.syx` export is only permitted when every occupied slot has a preserved `source_patch` template.

## Standard SysEx — `.syx`

AIM Editor writes standard SysEx files as:

```text
F0 <7-bit SysEx payload> F7
```

Multiple messages may be concatenated in one `.syx`. The importer accepts this form and can populate librarian slots from every checksum-valid candidate Ion single-program dump it finds, without assuming undocumented bank-stream semantics.

For research tooling, a raw status-free payload without F0/F7 is also accepted as one message.

## Planned formats

The same JSON-first design will be used for:

- `aim-editor.setup`
- `aim-editor.protocol-evidence`
- raw/captured SysEx fixture manifests
- transport-verification fixture sets

Native JSON documents coexist with hardware-compatible `.syx`; neither format is treated as disposable implementation detail.
