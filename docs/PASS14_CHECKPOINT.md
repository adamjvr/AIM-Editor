# Pass 14 checkpoint — JUCE 9 build gate and dialog correctness

Pass 14 deliberately pauses feature expansion and tightens the first real build boundary.

## Real JUCE 9 API bug fixed

The save/discard/cancel and quit-confirmation flows were built with `AlertWindow::showAsync(MessageBoxOptions, ...)` but interpreted the callback value as a zero-based button index.

That assumption is not portable in JUCE 9.0.1:

- `NativeMessageBox::showAsync(MessageBoxOptions, ...)` reports the selected button as zero-based `0, 1, 2...`;
- the non-native `AlertWindow` path preserves the historical AlertWindow return mapping, so a three-button box returns `1, 2, 0`.

A custom look-and-feel/platform choice could therefore make **Save**, **Discard**, and **Cancel** select the wrong branch.

Pass 14 moves all destructive three-button workflows to `NativeMessageBox::showAsync`, whose zero-based result contract matches AIM Editor's callbacks, and adds `validate_juce_api_contracts.py` to reject a regression back to the ambiguous APIs.

## Reproducible JUCE source gate

The project remains version-pinned to JUCE **9.0.1**, but the network fallback is now additionally pinned to the exact release commit:

`e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8`

Local JUCE trees are rejected by CMake if their declared project version is not exactly `9.0.1`. The preferred bootstrap path still downloads the official platform release archive and validates the already-pinned SHA-256 before use.

The one-command build scripts now bootstrap a local verified JUCE tree when necessary, then require a local tree for the actual build. This prevents an unnoticed CMake network fallback from changing the dependency during a build/test run.

## Build doctor is now a gate

`build_doctor.py --require-local-juce --strict-platform` turns missing platform SDK/development libraries into hard failures before configuration. On Debian/Ubuntu it prints the exact package-install hint used by the project's Linux CI.

The POSIX/Windows build helpers run that strict preflight automatically. `compile_commands.json` generation is enabled on supporting generators for future clang tooling and compiler diagnostics.

## CI hardening

When GitHub-hosted runners are available, desktop and iPad jobs now bootstrap the SHA-256-verified JUCE release archive before configuring. Workflow permissions are explicitly read-only.

The current GitHub Actions account/repository condition is still outside the source tree: Pass 13's five jobs all terminated with no steps and runner id `0`, so those failures contain no compiler evidence.

## Validation performed in this environment

- repository/data/protocol/schema self-tests: PASS;
- JUCE 9 API-contract guard: PASS;
- CMake project graph using a versioned local JUCE API stub: PASS;
- CMake local-JUCE version enforcement: PASS;
- POSIX shell syntax: PASS;
- strict build doctor: correctly FAILS before configuration because this sandbox lacks the Linux JUCE development packages;
- full real JUCE compile: **not claimed**.

The sandbox also has no direct GitHub DNS access for the official JUCE archive. That is treated separately from source correctness.

## Next gate

Run on Rosie:

```bash
cd ~/GitHub/AIM-Editor
./tools/build_and_test.sh
```

The helper will bootstrap verified JUCE 9.0.1 if necessary, stop immediately if Linux development packages are missing, then run repository checks, configure, compile, and CTest. Any compiler failure from that run becomes the next task before new features are added.
