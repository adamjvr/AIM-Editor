# AIM Editor

AIM Editor is an open-source, cross-platform editor/librarian for the **Alesis Ion** hardware synthesizer.

The project is a ground-up implementation. It does **not** reuse the original Windows editor's code or runtime. The goals are behavioral compatibility, portable MIDI/SysEx support, and a maintainable human-readable specification of the synth's parameter model.

## Targets

- macOS
- iPadOS
- Windows
- Linux

## Technology

- C++20
- JUCE 9
- CMake
- JSON as the canonical human-readable interchange format

## Architecture rules

1. The Ion data model is independent of the GUI.
2. UI controls never encode raw MIDI/SysEx protocol details directly.
3. Unknown reverse-engineered values remain explicit and nullable; they are never guessed.
4. Parameter definitions, patches, banks, mappings, and reverse-engineering fixtures must be exportable as readable JSON.
5. Raw SysEx must be preservable during incomplete decoding so round trips can eventually become byte-perfect.
6. Platform-specific code is kept at the edge. Core/model/protocol logic must remain portable.

## Current status

The project is now in its first functional-editor passes. It currently contains:

- a buildable JUCE application shell;
- five editor pages matching the reference application's Front, Dual 1, Dual 2, Randomizer, and Rear organization;
- a global MIDI/program control strip;
- a typed parameter registry loaded from JSON, including explicit enum domains;
- an observable semantic `ProgramState` shared across all editor views;
- JSON-driven knob/selector/toggle control construction rather than hard-coded per-widget protocol logic;
- 212 human-readable semantic parameters, including the complete 33-point candidate Tracking Generator curve;
- native `.aimprogram.json` and sparse `.aimbank.json` import/export;
- a 128-slot program librarian with name/category editing;
- a deterministic, seedable semantic patch Randomizer with section/type scopes, strength control, and one-step restore;
- a touch/mouse 33-point Tracking Generator curve editor backed directly by the JSON parameter model;
- safe editor-side linear/invert/zero Tracking Generator transforms that do not bulk-send candidate MIDI automatically;
- standard single/multi-message `.syx` import plus template-preserving program/bank `.syx` export;
- complete 378-byte source patch preservation inside native JSON when a program comes from hardware;
- a MIDI service boundary for device enumeration/input/output;
- opt-in candidate live NRPN editing with explicit MIDI channel selection;
- incoming NRPN decoding that updates shared state without MIDI feedback loops;
- synchronized semantic parameter state shared by every editor page;
- a live MIDI/SysEx capture inspector with JSON export;
- a candidate Ion/Micron 7-of-8 patch codec, checksum verifier, and single-patch request path;
- machine-readable candidate SysEx and NRPN specifications with explicit evidence status;
- 185 of 212 semantic parameters linked to candidate raw patch fields;
- 194 of 212 semantic parameters linked to candidate NRPN addresses;
- candidate Alesis signed-14-bit NRPN conversion kept separate from generic MIDI NRPN framing;
- JSON Schemas and protocol/core round-trip tests.

## Build

### Linux

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

### macOS

```bash
cmake -S . -B build-mac -G Xcode
cmake --build build-mac --config Debug
```

### iPadOS

Use an Xcode generator and the iOS toolchain supplied by Xcode:

```bash
cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0
cmake --build build-ios --config Debug
```

The generated Xcode project can then be signed and run on an iPad.

### Windows

```powershell
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64
cmake --build build-win --config Debug
ctest --test-dir build-win -C Debug --output-on-failure
```

## JUCE

By default CMake fetches the pinned JUCE release. To use a local checkout:

```bash
cmake -S . -B build -DAIM_EDITOR_JUCE_PATH=/path/to/JUCE
```

## Data

`data/parameters.json` is the canonical semantic parameter inventory. Unknown mappings remain `null`; research-backed but unverified mappings are stored as `candidate` with explicit evidence metadata rather than being mistaken for verified facts.

Transport-specific source material lives separately in `data/protocol/ion-sysex.json` and `data/protocol/ion-nrpn.json`. This is important because the same semantic parameter may use different raw domains over SysEx and NRPN.

Program exports use a nested JSON representation such as:

```json
{
  "format": "aim-editor.program",
  "schema_version": 1,
  "name": "Example",
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
  "unmapped_bytes": [],
  "source_patch": null
}
```

Native banks use `format: "aim-editor.bank"` and store only occupied slots. Programs that originate from real hardware carry a complete decoded `source_patch` byte array so unknown data is never discarded; JSON-only programs deliberately leave it `null`. See [`docs/DATA_FORMATS.md`](docs/DATA_FORMATS.md).

## Data tooling

Run all repository-level JSON/protocol/research checks:

```bash
./tools/check_repository.py
```

Validate the canonical database:

```bash
./tools/validate_parameter_database.py
```

Export it to a spreadsheet-friendly CSV without changing JSON as the source of truth:

```bash
./tools/export_parameter_database.py --format csv --output exports/parameter-database.csv
```

Research the candidate Ion SysEx format without building JUCE:

```bash
./tools/protocol/ion_sysex.py self-test
./tools/protocol/ion_sysex.py request --bank yellow --slot 42
./tools/protocol/ion_sysex.py decode patch.syx --output patch.json

./tools/protocol/ion_nrpn.py self-test
./tools/protocol/ion_nrpn.py show filter1.frequency
./tools/protocol/ion_nrpn.py encode filter1.env_amount -100 --channel 1

./tools/validate_protocol_data.py
./tools/validate_tracking_generator.py
```

## Live editing

Candidate NRPN live editing is deliberately disabled by default. See
[`docs/LIVE_EDITING.md`](docs/LIVE_EDITING.md) for the no-echo safety model and
hardware-verification workflow.

## Reverse engineering

The original editor was identified as a 32-bit Windows application exported from a SynthMaker/FlowStone-style runtime and wrapped with UPX. The application-specific behavior appears likely to be represented substantially by serialized project/module data rather than only handwritten x86.

AIM Editor's reverse-engineering work should focus on extracting a verified parameter/protocol specification and behavioral tests, not reproducing that legacy runtime architecture.

## License

Open-source release is intended, but the project license has not yet been selected. Choose the license before the first public release.

## Hardware verification tools

The current protocol maps are candidate data until tested on hardware. For controlled A/B patch-dump comparison:

```bash
./tools/protocol/ion_patch_diff.py diff before.json after.json \
  --expected filter1.frequency \
  --output research/captures/filter1-frequency-001.diff.json
```

See `docs/HARDWARE_VERIFICATION.md`. The diff report keeps raw decoded-byte changes authoritative and layers candidate field names on top.

The SysEx codec can also perform a template-preserving repack test:

```bash
./tools/protocol/ion_sysex.py repack input.syx output.syx
```

AIM Editor never synthesizes a hardware patch from only the parameters it currently understands; encoding starts from a complete source patch image so unmapped bytes/bits are preserved.
