# Candidate Ion NRPN specification

AIM Editor keeps the current community-derived Ion/Micron NRPN table in
`data/protocol/ion-nrpn.json`. It is deliberately separate from the SysEx
layout because the two transports do not always use identical raw value
representations.

## Evidence level

Primary current source:

- Alesis Ion / Micron / Miniak Wiki, **Common FAQ — MIDI implementation / NRPNs**
- https://ion-micron-miniak.fandom.com/wiki/Common_FAQ

The source itself says the table is preliminary and contains mistakes. Every
entry is therefore `candidate` with `hardware_verified: false` until AIM Editor
has independent evidence.

## NRPN transport

Candidate address sequence:

```text
CC 99 = NRPN address MSB
CC 98 = NRPN address LSB
CC  6 = Data Entry MSB
CC 38 = Data Entry LSB
```

The normal 14-bit reconstruction is:

```text
address = (CC99 << 7) | CC98
value   = (CC6  << 7) | CC38
```

## Important Alesis behavior

Community evidence documents two behaviors that generic hardware-controller
NRPN implementations commonly get wrong:

1. For small values Alesis expects the **Data Entry LSB (CC38)** to carry the
   low seven bits. Sending only CC6, as many coarse/7-bit NRPN controllers do,
   is not equivalent.
2. Signed values wrap through the 14-bit unsigned space. For example:

```text
semantic -100 -> encoded 16284
semantic   -1 -> encoded 16383
semantic    0 -> encoded     0
semantic  100 -> encoded   100
```

AIM Editor implements this candidate conversion separately from the generic
MIDI NRPN encoder as `IonProtocol::encodeIonSigned14()` and
`IonProtocol::makeIonNrpnSequence()`.

It is now wired to **opt-in** live knob writes through `IonParameterTransmitter`.
The feature starts disabled, transmits only interactive editor changes, and remains
marked candidate until hardware captures verify address/value behavior.

## Current coverage

`data/protocol/ion-nrpn.json` currently contains 234 candidate definitions,
including:

- voice and portamento;
- oscillators/FM/sync;
- pre-filter and post-filter mixers;
- both filters;
- three envelopes;
- LFOs and Sample & Hold;
- tracking generator;
- 12 modulation-matrix slots;
- effects;
- arpeggiator.

194 of 212 current semantic parameters have a candidate one-to-one NRPN link in
`data/parameters.json`. The increase includes 33 candidate Tracking Generator curve
points plus its preset field, which were intentionally absent from the initial
screenshot-only inventory.

## Known conflicts are data, not hidden assumptions

Examples currently recorded explicitly in JSON:

- FX mix has different candidate raw domains between NRPN and SysEx evidence.
- LFO reset ranges differ between the NRPN and SysEx sources.
- Sample & Hold input is implausibly listed as only `0..1` in the NRPN table,
  despite the much larger known modulation-source domain.
- Tracking input uses different candidate domains in NRPN and SysEx material.
- Program category ranges disagree.
- Modulation slot 1 source has a smaller maximum than slots 2–12 in the same
  NRPN source table.

These are not normalized away. The application must eventually use verified
transport-specific conversion rules.

## Research utility

No JUCE build is required to inspect or generate candidate NRPN messages:

```bash
./tools/protocol/ion_nrpn.py self-test
./tools/protocol/ion_nrpn.py show filter1.frequency
./tools/protocol/ion_nrpn.py encode filter1.env_amount -100 --channel 1
```

The final command emits the four MIDI CC messages as human-readable JSON and
hex bytes.
