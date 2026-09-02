# Reproducible build and document workflow

The build/document workflow hardens boundaries that should not depend on developer memory: obtaining the exact JUCE source used by AIM Editor, using a repository-local Python schema-validation toolchain instead of distro packages, and opening/saving native documents through one guarded path.

## Pinned JUCE 9.0.1 bootstrap

AIM Editor is currently pinned to **JUCE 9.0.1**. The bootstrap helpers download the official platform release archive and verify its SHA-256 before the source tree is installed under `.deps/JUCE`.

Linux/macOS — the build helper bootstraps the verified dependency automatically when needed:

```bash
./tools/build_and_test.sh
```

The normal build also creates `.deps/python-tools` and installs the fully pinned repository-validation dependencies from `tools/requirements-tools.txt`. This prevents an old distro `python3-jsonschema` from silently changing which JSON Schema draft is available. The environment is local to the repository and never modifies the host/system Python.

To prepare it explicitly:

```bash
./tools/bootstrap_python_tools.py
```

Then a non-mutating strict preflight can be run with:

```bash
.deps/python-tools/bin/python tools/build_doctor.py --strict-platform
```

On Debian/Ubuntu, if `python3 -m venv` is unavailable, install `python3-venv`; the bootstrapper prints that exact hint and stops without attempting a system-wide `pip install`.

Windows PowerShell:

```powershell
.\tools\build_and_test.ps1
```

Use `-NoBootstrap` only when `AIM_EDITOR_JUCE_PATH` already points to a verified local JUCE 9.0.1 tree.

The pinned archive digests are intentionally stored in the bootstrap scripts. A checksum mismatch aborts extraction. `.deps/` is ignored by Git, so JUCE itself is never folded into the AIM Editor repository.

`build_doctor.py` is a non-mutating preflight. When run inside `.deps/python-tools`, it checks CMake 3.22+, C and C++ compilers, the pinned Python schema capability (`Draft202012Validator` plus `referencing`), the local JUCE version, and the main platform prerequisites it can detect. On Linux it also reports missing pkg-config development packages commonly required by JUCE.

CMake rejects a local tree whose declared JUCE project version is not exactly 9.0.1. Network FetchContent fallback is pinned to the exact JUCE 9.0.1 release commit `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8`.

CMake resolves JUCE in this order:

1. explicit `-DAIM_EDITOR_JUCE_PATH=...`;
2. environment variable `AIM_EDITOR_JUCE_PATH`;
3. repository-local `.deps/JUCE`;
4. pinned CMake FetchContent fallback.

The one-command build helpers obtain a verified local JUCE tree if necessary, create/update the pinned `.deps/python-tools` validation environment, run the doctor in strict platform mode through that interpreter, run the repository/protocol validators through that same interpreter, configure, compile, and CTest. The Windows helper provides the same path for Visual Studio builds. Sanitizers remain opt-in on supported Linux/macOS compilers.

## Current build-verification boundary

Rosie has now completed the real JUCE 9.0.1 Linux compile/link and CTest gate through Pass 19. That verifies the Linux standalone application/core-test path; it does **not** prove the Apple targets.

On macOS, run the aggregate Apple application gate:

```bash
./tools/build_apple_targets.sh
```

It runs the normal macOS standalone application + CTest pipeline and the **physical iPadOS device** build concurrently in independent build trees. A successful macOS branch launches the built desktop app; the physical-iPad branch installs and launches its signed app. There is no simulator gate. The iPad helper uses the same pinned JUCE/Python validation environment, requires a visible `iphoneos` SDK plus `devicectl`, discovers one connected paired physical iPad, requires Developer Mode, and configures an arm64 Xcode build for iPadOS 17+.

The iPadOS minimum is expressed with `CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET=17.0` rather than the generic `CMAKE_OSX_DEPLOYMENT_TARGET`. JUCE 9.0.1 reinvokes CMake to build the host-side `juceaide` tool, so keeping the iOS-only deployment value in the Xcode platform setting prevents the host helper from inheriting it as a macOS deployment target.

Physical-device signing uses Xcode automatic signing with `-allowProvisioningUpdates` and `-allowProvisioningDeviceRegistration`. Team IDs are never inferred from certificate display names. Copy `.aim-editor-local.env.example` to `.aim-editor-local.env` and set `AIM_EDITOR_DEVELOPMENT_TEAM` plus, when desired, `AIM_EDITOR_IPAD_DEVICE` to the exact physical-device UDID. The helper forwards an explicit UDID directly to CoreDevice selection rather than depending on shell-export state, and a selector error aborts before any build variables are consumed. The local file is ignored by Git. After a successful build, the helper resolves the real `.app` path from Xcode's generated `TARGET_BUILD_DIR` and `FULL_PRODUCT_NAME` settings, verifies the code signature, installs the `.app` with `devicectl`, and launches bundle ID `com.rothamplification.aimeditor` on the selected iPad. The aggregate Apple gate prefixes both concurrent streams and persists them under `build-logs/`.

The iPad app target keeps JUCE's `FILE_SHARING_ENABLED` and `DOCUMENT_BROWSER_ENABLED` properties, is restricted to device family 2 (iPad), hides the system status bar, requires full-screen presentation, and supports the two landscape orientations. The initial hardware gate intentionally does not require an iCloud entitlement; Files/document-picker workflows do not need an iCloud container merely to build and launch the editor.

Do not record iPadOS as verified until the physical-hardware command reaches:

```text
PASS: AIM Editor physical iPadOS application build/install/launch
```

Missing SDKs, an unpaired/locked iPad, disabled Developer Mode, signing/provisioning failures, or unavailable dependency downloads are environment/signing failures rather than evidence about AIM Editor's protocol behavior.

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
