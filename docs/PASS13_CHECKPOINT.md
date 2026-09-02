# Pass 13 checkpoint — build/document hardening

Pass 13 prepares AIM Editor for its first real cross-platform compile and makes native document entry points converge on the same safety model.

## Added

- SHA-256-pinned JUCE 9.0.1 bootstrap scripts for Linux/macOS and Windows.
- `build_doctor.py` preflight for CMake, C/C++ toolchains, pinned JUCE and detectable platform dependencies.
- Windows configure/build/CTest helper matching the Linux/macOS workflow.
- local JUCE precedence via `AIM_EDITOR_JUCE_PATH` or `.deps/JUCE`, with FetchContent retained as fallback.
- build-workflow repository guardrails that verify the pins and reject unsafe runtime-enable markers in build scripts.
- unified librarian Open/Save All dispatch.
- `Ctrl/Cmd+O`, `Ctrl/Cmd+S`, and `Ctrl/Cmd+Shift+S` document shortcuts.
- single-instance command-line/second-launch document forwarding through the same guarded dispatcher, plus declared `.syx` association metadata.
- `.syx` application document association via JUCE CMake.
- deterministic NRPN/SysEx hardware-verification work-queue generator and JSON schema.

## Safety invariants retained

- Native JSON is still the automatic document-save format.
- `.syx` export still requires a complete real source-patch template.
- Opening/replacing documents still honors Save/Discard/Cancel guards.
- Build/session tooling cannot enable Live NRPN or arm a full patch write.
- Verification planning is read-only and cannot promote mappings.
- No protocol mapping was promoted in this pass.

## Validation

The repository-level data/protocol/schema/tooling suite passes, including the new build-workflow and verification-plan self-tests. CMake project-graph configuration is checked separately with the local JUCE API stub.

A complete real JUCE compile is **not** claimed by this checkpoint. The current analysis host cannot retrieve/use the official JUCE archive and is missing several Linux development packages. GitHub Actions also still fails before any runner starts (`steps=[]`, runner id 0), so those runs contain no compiler evidence.

## Next

1. Run the pinned bootstrap + full build/test helper on Rosie.
2. Fix every compiler/linker/runtime issue found by the real JUCE build before adding architectural debt.
3. Run the macOS desktop and iPad Simulator builds.
4. Start the deterministic hardware-verification queue on a real Ion and commit sealed evidence.
5. Continue geometry/accessibility polish from device screenshots once the build is stable.
