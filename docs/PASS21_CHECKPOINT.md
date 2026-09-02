# Pass 21 checkpoint — isolate the iPadOS deployment target from JUCE host tools

> Historical note: the simulator path described below was retired in Pass 23. Current iPadOS verification is physical-device-only.


Pass 21 is a corrective Apple-build pass based on the first real Pass 20 run on an Apple Silicon Mac with Xcode 26.3.

## Real build evidence

The macOS standalone application compiled and linked successfully and `AIMEditorCoreTests` passed under CTest. The iPadOS Simulator build reached CMake cross-configuration, but failed before any AIM Editor iOS source compilation while JUCE 9.0.1 recursively configured the host-side `juceaide` helper.

The failing Pass 20 command supplied `CMAKE_OSX_DEPLOYMENT_TARGET=17.0` to the iOS cross-build. JUCE 9.0.1 intentionally reinvokes CMake to build `juceaide` for the host and forwards `CMAKE_OSX_DEPLOYMENT_TARGET` into that host configuration. This makes the iPadOS 17 value appear as a macOS deployment target during the helper build.

## Corrective boundary

The iPad simulator helper now leaves the generic `CMAKE_OSX_DEPLOYMENT_TARGET` unset and expresses the app minimum with Xcode's platform-specific setting instead:

```text
CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET=17.0
```

This preserves the intended iPadOS 17 minimum while allowing JUCE's recursive host-tool build to use the host macOS/Xcode defaults. The iPad verification helper also removes its generated `build-ios` tree before each configure so the failed Pass 20 cache cannot retain the old cross-build value. A static validator rejects reintroducing the Pass 20 generic `17.0` deployment argument.

## Verification command

On macOS:

```bash
./tools/build_apple_targets.sh
```

The macOS half is already real-build verified from Pass 20. Pass 21 is complete only when the iPadOS Simulator target reaches:

```text
PASS: AIM Editor iPadOS simulator application build
PASS: AIM Editor Apple standalone-application gate
```

No plug-in target is introduced by this pass; `AIMEditor` remains a standalone JUCE GUI application.
