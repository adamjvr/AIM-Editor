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

This is the initial implementation scaffold. It currently contains:

- a buildable JUCE application shell;
- five editor pages matching the reference application's Front, Dual 1, Dual 2, Randomizer, and Rear organization;
- a global MIDI/program control strip;
- a typed parameter registry loaded from JSON;
- an initial screenshot-derived inventory of 178 visible/semantic parameters;
- JSON program import/export infrastructure;
- a MIDI service boundary for device enumeration/input/output;
- protocol placeholders that deliberately avoid inventing unverified Ion addresses;
- JSON Schemas and core round-trip tests.

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

`data/parameters.json` is the first canonical parameter inventory. Protocol addresses are intentionally `null` until verified.

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
  "unmapped_bytes": []
}
```

## Data tooling

Validate the canonical database:

```bash
./tools/validate_parameter_database.py
```

Export it to a spreadsheet-friendly CSV without changing JSON as the source of truth:

```bash
./tools/export_parameter_database.py --format csv --output exports/parameter-database.csv
```

## Reverse engineering

The original editor was identified as a 32-bit Windows application exported from a SynthMaker/FlowStone-style runtime and wrapped with UPX. The application-specific behavior appears likely to be represented substantially by serialized project/module data rather than only handwritten x86.

AIM Editor's reverse-engineering work should focus on extracting a verified parameter/protocol specification and behavioral tests, not reproducing that legacy runtime architecture.

## License

Open-source release is intended, but the project license has not yet been selected. Choose the license before the first public release.
