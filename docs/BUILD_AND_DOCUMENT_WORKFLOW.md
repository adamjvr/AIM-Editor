# Reproducible build and document workflow

Pass 13 hardens two boundaries that should not depend on developer memory: obtaining the exact JUCE source used by AIM Editor, and opening/saving native documents through one guarded path.

## Pinned JUCE 9.0.1 bootstrap

AIM Editor is currently pinned to **JUCE 9.0.1**. The bootstrap helpers download the official platform release archive and verify its SHA-256 before the source tree is installed under `.deps/JUCE`.

Linux/macOS — the build helper bootstraps the verified dependency automatically when needed:

```bash
./tools/build_and_test.sh
```

For a non-mutating preflight only:

```bash
./tools/build_doctor.py
```

Windows PowerShell:

```powershell
.\tools\build_and_test.ps1
```

Use `-NoBootstrap` only when `AIM_EDITOR_JUCE_PATH` already points to a verified local JUCE 9.0.1 tree.

The pinned archive digests are intentionally stored in the bootstrap scripts. A checksum mismatch aborts extraction. `.deps/` is ignored by Git, so JUCE itself is never folded into the AIM Editor repository.

`build_doctor.py` is a non-mutating preflight. It checks CMake 3.24+, C and C++ compilers, the local JUCE version, and the main platform prerequisites it can detect. On Linux it also reports missing pkg-config development packages commonly required by JUCE.

CMake rejects a local tree whose declared JUCE project version is not exactly 9.0.1. Network FetchContent fallback is pinned to the exact JUCE 9.0.1 release commit `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8`.

CMake resolves JUCE in this order:

1. explicit `-DAIM_EDITOR_JUCE_PATH=...`;
2. environment variable `AIM_EDITOR_JUCE_PATH`;
3. repository-local `.deps/JUCE`;
4. pinned CMake FetchContent fallback.

The one-command build helpers obtain a verified local JUCE tree if necessary, run the doctor in strict platform mode, run the repository/protocol validators, configure, compile, and CTest. The Windows helper provides the same path for Visual Studio builds. Sanitizers remain opt-in on supported Linux/macOS compilers.

## Current build-verification boundary

The repository has passed data/protocol validation, a JUCE 9 API-contract audit, and CMake project-graph configuration in the analysis environment. A complete compilation against the real JUCE 9.0.1 tree still needs to be executed on a machine with the platform development dependencies installed. GitHub Actions is presently failing before jobs acquire a runner, so its current failures are not compiler/test results.

Do not record a platform as build-verified until the corresponding real build command reaches compilation and CTest/Xcode completion. Missing development libraries, unavailable dependency downloads, and GitHub jobs that never acquire a runner are environment failures, not compiler evidence.

## Unified document open/save

The librarian now owns one document-dispatch path instead of duplicating parsing logic across buttons, shortcuts, and future OS file-open events.

**Open** accepts:

- `.aimprogram.json` — recognized by `format: "aim-editor.program"`;
- `.aimbank.json` — recognized by `format: "aim-editor.bank"`;
- `.syx` — decoded through the candidate Ion SysEx path.

Generic `.json` may be selected, but it must identify one of the supported AIM formats in its root `format` field. Unsupported or malformed files are rejected without replacing the current document.

Opening a document still passes through the existing unsaved-work guard. Cancelled/failed saves abort the destructive action.

**Save All** saves every dirty native JSON document. An existing document path is reused; the first save asks for a path. `.syx` is deliberately not an automatic save target because safe SysEx export requires a real source patch template.

## Keyboard and OS entry points

Desktop shortcuts:

- `Ctrl/Cmd+O` — unified Open;
- `Ctrl/Cmd+S` — Save All dirty native documents;
- `Ctrl/Cmd+Shift+S` — Save Program As;
- existing semantic Undo/Redo shortcuts remain unchanged.

AIM Editor runs as a single application instance and routes a file supplied on the command line/second-instance launch into the same guarded librarian dispatcher. CMake advertises `.syx` as a document extension. Native AIM documents retain their explicit double extensions (`.aimprogram.json` and `.aimbank.json`); they are identified from JSON content rather than pretending the operating system can distinguish them from every other `.json` file.

## Hardware-verification work queue

The verification pipeline now has a deterministic read-only planner:

```bash
./tools/protocol/ion_verification_plan.py next --transport nrpn
./tools/protocol/ion_verification_plan.py plan --transport nrpn --limit 10
./tools/protocol/ion_verification_plan.py plan --transport sysex --page front
```

The planner reads canonical parameter JSON, prioritizes visible signal-path controls, and proposes distinct test values/procedures. It does **not** create evidence and cannot promote mappings. Completed captures still go through `ion_protocol_verification.py`, and only sealed evidence can change `candidate` to `verified`.
