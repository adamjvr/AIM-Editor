# Pass 4 checkpoint — Randomizer + Tracking Generator

This checkpoint turns two legacy-editor features into first-class AIM Editor
systems rather than generic placeholder controls.

## Added

- `RandomizerEngine` in Core, independent of JUCE UI and MIDI transport.
- Deterministic 64-bit seeds for reproducible random patches.
- Randomizer scopes for oscillator, filter, envelope, modulation, effects, and
  voice/output sections.
- Separate inclusion of numeric values, enum/routing values, and switches.
- Strength control and one-step restore in the Randomizer page.
- No automatic bulk MIDI output from Randomizer replacements.
- 33 canonical Tracking Generator point parameters (`-16..+16`).
- Candidate Tracking Generator preset parameter.
- Candidate point mappings aligned to NRPN `121..153` and decoded patch offsets
  `304..336`.
- Touch/mouse curve editing on the Rear page.
- Local linear, invert, and zero curve transforms.
- Dedicated Tracking Generator integrity validator.
- Parameter CSV regenerated from the expanded JSON database.

## Validation

Repository data/protocol checks pass with:

- 212 unique semantic parameters;
- 234 candidate NRPN definitions;
- 263 candidate SysEx fields;
- 194/212 semantic parameters with candidate NRPN mappings;
- 185/212 semantic parameters with candidate SysEx mappings;
- 33 contiguous Tracking Generator points verified internally against both
  candidate transport datasets.

CMake configuration is also checked against the local JUCE API-target stub. A
full JUCE C++ compile still needs a machine with the JUCE source/dependencies
available; the sandbox cannot resolve GitHub directly for FetchContent.

## Safety / research posture

Everything introduced here remains candidate protocol knowledge until measured
against real hardware. Tracking Generator single-point edits may participate in
opt-in live NRPN testing. Bulk curve transforms intentionally use an internal
change origin to prevent accidental bursts of unverified MIDI writes.
