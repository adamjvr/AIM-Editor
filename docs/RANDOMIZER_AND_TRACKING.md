# Randomizer and Tracking Generator

## Randomizer

The legacy editor exposes a patch Randomizer. AIM Editor implements this as a
protocol-independent transformation of the semantic `IonProgram` rather than as
GUI automation or a burst of MIDI messages.

`RandomizerEngine` reads all domains from `data/parameters.json` and refuses to
invent ranges. Parameters whose protocol state is still `unmapped` are skipped.
The UI lets the user choose synth sections, choose whether values/types/switches
may change, select a strength, and optionally enter a deterministic 64-bit seed.
The seed actually used is shown after every randomization so a result can be
reproduced.

Strength is applied as interpolation for numeric raw values and as replacement
probability for enum/switch values. Source-patch bytes and unknown bytes are
never regenerated or discarded; the resulting semantic program retains the
original source template exactly.

Randomizer changes do **not** automatically bulk-send NRPN data. That is
intentional while the hardware mappings remain candidate data. The result can
be inspected, saved as JSON, or later sent through a verified edit-buffer path.

## Tracking Generator

Pass 4 promotes the candidate Tracking Generator curve from transport research
to first-class semantic data.

The candidate protocol model contains 33 signed points:

- point -16 through point +16;
- raw domain `-100..100`;
- candidate SysEx offsets `304..336`;
- candidate NRPN numbers `121..153`;
- signed 14-bit NRPN transport;
- signed 8-bit patch-image storage.

These are represented canonically as IDs such as:

```text
tracking_generator.point_minus_16
tracking_generator.point_plus_0
tracking_generator.point_plus_16
```

The source protocol field spelling is retained separately in each parameter's
`protocol.sysex.field_id`, so normalized application IDs do not destroy
reverse-engineering provenance.

The Rear page now draws those 33 semantic values as an interactive curve. A
mouse or touch drag changes one point through `ProgramState`, meaning JSON
export, incoming MIDI, SysEx decoding, and the graph all see the same data.

The `linear`, `invert`, and `zero` convenience transforms are local semantic
operations. They intentionally use an internal change origin so enabling live
NRPN editing cannot accidentally spray 33 unverified writes at hardware.
Single-point graph edits remain normal interactive edits and can participate in
opt-in live NRPN testing.

`tracking_generator.preset` is also represented as a candidate enum field, but
selecting a hardware preset does not synthesize undocumented point values in the
GUI. Until hardware captures prove the device's exact preset-generated curve,
AIM Editor preserves that distinction rather than pretending an approximation
is authoritative.

## Verification

Run:

```bash
./tools/validate_tracking_generator.py
```

The validator checks that all 33 canonical points remain contiguous and aligned
with the candidate NRPN and SysEx datasets.
