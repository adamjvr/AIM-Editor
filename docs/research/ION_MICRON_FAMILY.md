# Ion / Micron Program-family architecture

## Conclusion

AIM Editor treats the Alesis Ion and Micron as two hardware shells around one shared Program format, not as two independent editors.

The shared Program transport remains:

```text
434 bytes on the MIDI wire (F0 ... F7)
432-byte SysEx payload
        |
        | Alesis 7-of-8 unpack
        v
378 decoded bytes
        |
        +-- product/dump ID 0x22
        +-- Q01SYNTH
        +-- 315-byte Program-data body beginning at decoded offset 63
```

This matches AIM Editor's existing template-preserving Program codec and is independently corroborated by the user-supplied `micronau` source/specimen.

## Device-shell split

| Property | Ion | Micron |
| --- | --- | --- |
| Program dump format | shared 0x22 / 378 bytes | shared 0x22 / 378 bytes |
| single-Program request product ID | `0x22` | `0x26` |
| request opcode | `0x41` | `0x41` |
| request banks exposed by current evidence | 5 Ion banks | 8 numeric banks |
| programs per normal request bank | 128 | 128 |
| Ion Edit 1-4 write target | candidate-supported | not assumed |
| bank-dump request | candidate-supported | unknown; disabled |
| FX2 delay/reverb | no | yes |
| X/Y/Z assignments | no | yes |

The critical rule is that request framing and storage policy are device-specific even when Program bytes are shared.

## Micronau corroboration

Pinned research source:

- repository: `retroware/micronau`
- commit: `0a1f14d8c5bce11663b464cef54e2035aba6ab1a`
- license: GPL-2.0-or-later
- AIM evidence summary: `research/external/micronau-evidence.json`

Useful independent facts include:

- Micron request template `00 00 0E 26 41 ...`.
- `default.syx` is 434 bytes and decodes into the same 378-byte `0x22`/`Q01SYNTH` Program image AIM already understands.
- Micron X/Y/Z assignment bytes resolve to decoded offsets 162/163/164.
- Program category resolves to decoded offset 86 and has 11 values.
- FX1/FX2 balance resolves to decoded offset 352 and hardware NRPN 230.
- FX2 type resolves to decoded offset 353 and hardware NRPN 245; the five selected-algorithm parameter controls map onto hardware NRPNs 246-250.

## Evidence discipline

Micronau is corroboration, not automatic verification. Its source is GPL, so AIM does not transplant its implementation. We extract protocol facts into provenance-backed JSON and implement our own transport/model code.

Physical verification remains per device. A mapping proven on an Ion can be marked Ion-verified without falsely claiming Micron verification, and vice versa.

Two conflicts are intentionally held:

1. NRPN 105: current AIM candidate source says LFO 1 tempo sync; Micronau labels it Micron file ID.
2. NRPN 250: current AIM candidate source labels a synced FX1 parameter; Micronau's selected-FX2 mapping reaches the same number.

Neither conflict is resolved by guesswork. They go to controlled hardware capture.

## Pass 31 implementation boundary

Pass 31 adds an explicit device profile to the editor session and request path:

```text
IonFamily Program model
        |
        +-- Ion profile
        |    request product 0x22
        |    named Ion banks
        |    bank request enabled
        |    Edit 1-4 guarded writes enabled
        |
        +-- Micron profile
             request product 0x26
             8 request banks / 128 programs
             bank request disabled pending proof
             full Program writes disabled pending proof
```

The UI selector changes the hardware request shell only. It does not duplicate Program state or create divergent parameter models.
