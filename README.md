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
- purpose-built oscillator, dual-filter, and three-envelope editor blocks instead of only generic control grids;
- purpose-built LFO/S&H/tempo, voice, effects, pre/post mixer, and output blocks covering the remaining main signal-path groups;
- segmented Front/Dual 1/Dual 2/Randomizer/Rear navigation on large landscape surfaces with compact-window fallback;
- touch/mouse Pitch, Filter, and Amp envelope curve editing backed by the same semantic state as the knobs;
- signal-oriented large-landscape Front/Dual/Rear layouts with automatic compact-window fallback;
- procedural high-DPI Ion-style knobs, selectors, and LED toggles suitable for desktop and iPad;
- conservative JSON-driven engineering-value formatting, with unknown transforms shown explicitly as raw values;
- safe editor-side linear/invert/zero Tracking Generator transforms that do not bulk-send candidate MIDI automatically;
- standard single/multi-message `.syx` import plus template-preserving program/bank `.syx` export;
- complete 378-byte source patch preservation inside native JSON when a program comes from hardware;
- a MIDI service boundary for device enumeration/input/output;
- opt-in candidate live NRPN editing with explicit MIDI channel selection;
- incoming NRPN decoding that updates shared state without MIDI feedback loops;
- synchronized semantic parameter state shared by every editor page;
- a live MIDI/SysEx capture inspector with JSON export and checksum-valid candidate patch summary;
- guarded hardware-transfer tools for patch/bank requests and source-template-preserving sends to Edit 1–4;
- a candidate Ion/Micron 7-of-8 patch codec, checksum verifier, and single-patch request path;
- machine-readable candidate SysEx and NRPN specifications with explicit evidence status;
- 185 of 212 semantic parameters linked to candidate raw patch fields;
- 194 of 212 semantic parameters linked to candidate NRPN addresses;
- candidate Alesis signed-14-bit NRPN conversion kept separate from generic MIDI NRPN framing;
- JSON Schemas and protocol/core round-trip tests;
- semantic unsaved-program and librarian-bank tracking with save/discard/cancel protection;
- save-aware application quit plus an explicit semantic New Program workflow that never invents SysEx bytes;
- unified guarded document Open/Save All handling with desktop Ctrl/Cmd+O, Ctrl/Cmd+S, and Ctrl/Cmd+Shift+S shortcuts;
- single-instance command-line/second-launch document forwarding through the same librarian safety path, with `.syx` association metadata declared in CMake;
- SHA-256-verified JUCE 9.0.1 bootstrap plus an exact upstream commit/version gate for repeatable local builds;
- JUCE 9 destructive confirmation dialogs use a statically guarded zero-based result contract so Save/Discard/Cancel cannot change meaning between native and custom alert implementations;
- a deterministic read-only NRPN/SysEx hardware-verification work queue;
- a controlled NRPN Start/Stop verification mode that freezes evidence captures and cross-checks the inspector assessment offline against raw MIDI.

## Build

AIM Editor is pinned to **JUCE 9.0.1** and the exact upstream commit `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8`. The preferred build is one command; when no local JUCE tree is available the helper bootstraps the SHA-256-verified official release archive into `.deps/JUCE`, runs the strict platform doctor, validates the repository, configures CMake, compiles, and runs CTest:

```bash
./tools/build_and_test.sh
```

On Windows PowerShell:

```powershell
.\tools\build_and_test.ps1
```

To use an already-verified JUCE checkout, set `AIM_EDITOR_JUCE_PATH` or pass `-DAIM_EDITOR_JUCE_PATH=/path/to/JUCE`. CMake rejects a local JUCE tree whose declared version is not exactly 9.0.1; the network FetchContent fallback is pinned to the exact commit rather than a movable tag. The bootstrap aborts on an archive checksum mismatch. See [`docs/BUILD_AND_DOCUMENT_WORKFLOW.md`](docs/BUILD_AND_DOCUMENT_WORKFLOW.md).

For macOS desktop, the normal helper configures/builds/tests through CMake. For the iPad Simulator on a Mac with Xcode installed:

```bash
./tools/build_ipad_simulator.sh
```

AddressSanitizer + UndefinedBehaviorSanitizer remain opt-in on supported Clang/GCC desktop builds:

```bash
AIM_EDITOR_SANITIZE=1 ./tools/build_and_test.sh
```

The repository checks and CMake project graph are green, but a complete build against the real JUCE tree is not considered verified until one of these commands reaches compilation/tests on the target machine.

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

Run `./tools/build_doctor.py --strict-platform` for a non-mutating strict platform preflight, or use `./tools/build_and_test.sh` (or `.ps1` on Windows) for the complete verified-JUCE bootstrap/repository-check/configure/build/CTest pipeline.

Undo/redo is semantic and shared across every editor view. It never echoes back to live MIDI. See [`docs/UNDO_AND_WORKFLOW.md`](docs/UNDO_AND_WORKFLOW.md).

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
./tools/protocol/ion_verification_report.py report
./tools/protocol/ion_verification_plan.py next --transport nrpn
```

## Live editing

Candidate NRPN live editing is deliberately disabled by default. See
[`docs/LIVE_EDITING.md`](docs/LIVE_EDITING.md) for the no-echo safety model and
hardware-verification workflow.

## Guarded hardware transfer

The **Update Edit Buffer** control now opens a dedicated candidate hardware-transfer surface rather than sending a full dump immediately. Full patch writes require a real 378-byte source template plus an explicit arm switch, and the arm resets after every send. Requests remain available without arming. See [`docs/HARDWARE_TRANSFER.md`](docs/HARDWARE_TRANSFER.md).

The offline codec mirrors destination retargeting:

```bash
./tools/protocol/ion_sysex.py retarget source.syx edit1.syx --bank edit --slot 0
```

## Unified document workflow

The librarian has one guarded dispatcher for `.aimprogram.json`, `.aimbank.json`, and `.syx`. `Ctrl/Cmd+O` opens through that path, `Ctrl/Cmd+S` saves all dirty native JSON documents, and `Ctrl/Cmd+Shift+S` performs Save Program As. A supported file passed through AIM Editor's command-line/second-instance entry point is forwarded through the same unsaved-work protection; CMake also declares `.syx` association metadata. See [`docs/BUILD_AND_DOCUMENT_WORKFLOW.md`](docs/BUILD_AND_DOCUMENT_WORKFLOW.md).

## Document safety and controlled verification

AIM Editor now tracks the current semantic program and the native librarian bank against independent clean baselines. Destructive loads/imports and application quit now offer Save, Discard, or Cancel paths before unsaved state is replaced. The MIDI/SysEx Inspector also provides a bounded **Start Test / Stop Test** workflow that freezes a controlled NRPN experiment before export; offline verification independently rebuilds the transaction stream from raw MIDI before promotion. See [`docs/DOCUMENT_SAFETY_AND_VERIFICATION_MODE.md`](docs/DOCUMENT_SAFETY_AND_VERIFICATION_MODE.md) and [`docs/SAVE_AND_EVIDENCE_WORKFLOW.md`](docs/SAVE_AND_EVIDENCE_WORKFLOW.md).

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

## Safe session restore and verification capture

AIM Editor remembers non-destructive session context (page, MIDI channel, bank/program context, and selected MIDI endpoints), but **never** restores Live NRPN or full-patch write arming. Reopening remembered MIDI devices therefore cannot transmit anything by itself.

The MIDI/SysEx Inspector also exports reconstructed `nrpn_transactions` alongside the authoritative raw MIDI events. These records attach candidate semantic IDs/status to complete CC99/CC98/CC6/CC38 sequences without automatically promoting any protocol mapping to verified. See [`docs/SESSION_AND_VERIFICATION.md`](docs/SESSION_AND_VERIFICATION.md).

## Auditable protocol promotion

Hardware testing can now promote candidate NRPN/SysEx mappings without hand-editing canonical JSON. The SysEx Inspector can tag a controlled capture with a semantic parameter ID, an isolation confirmation, and a test note. Offline verification reconstructs raw transport evidence, seals the result with SHA-256, and only promotes mappings whose evidence passes strict checks.

```bash
./tools/protocol/ion_protocol_verification.py nrpn capture.json \
  --parameter filter1.frequency --direction input --confirm-isolation \
  -o research/verification/filter1-frequency-nrpn-001.json

./tools/protocol/ion_protocol_verification.py apply \
  research/verification/filter1-frequency-nrpn-001.json
```

`verified` mappings must reference committed evidence under `research/verification/`; repository validation rejects evidence-free verification claims. See `docs/PROTOCOL_VERIFICATION.md`.
