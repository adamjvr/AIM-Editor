# Pass 20 checkpoint — Apple standalone-application gate

Pass 20 begins only after the real Linux JUCE 9.0.1 application build and CTest gate is green. It does not add a plug-in target and does not expand protocol behavior. Its purpose is to make macOS/iPadOS verification repeatable before feature work resumes.

## Standalone application contract

`AIMEditor` remains a `juce_add_gui_app` target. No `juce_add_plugin` target, VST3, AU, AUv3, CLAP, or plug-in wrapper is introduced.

For iPadOS the same application target explicitly enables JUCE's native document-browser/file-sharing/iCloud properties and targets iPad landscape orientations. The simulator build disables signing only for build verification; real device distribution requires Apple signing/provisioning and matching iCloud capability configuration.

## Apple gate

Run on a Mac with Xcode:

```bash
./tools/build_apple_targets.sh
```

The aggregate gate:

1. runs the normal pinned macOS build/test pipeline;
2. verifies Xcode/macOS/iPhoneSimulator SDK visibility;
3. runs all repository/protocol/static safety checks through the pinned Python environment;
4. configures an iPadOS 17 Simulator Xcode build;
5. builds the `AIMEditor` standalone application with simulator code signing disabled.

Expected terminal markers:

```text
PASS: AIM Editor local build/test pipeline
PASS: AIM Editor iPadOS simulator application build
PASS: AIM Editor Apple standalone-application gate
```

Do not call macOS/iPadOS verified until those real Xcode builds complete. Any compiler/API failures discovered there belong in the next corrective pass before protocol or UI expansion resumes.
