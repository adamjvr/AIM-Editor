# Pass 26 checkpoint — JUCE iOS Objective-C++ ARC boundary

Pass 26 fixes the first real physical-iPad compile failure reached after device selection, provisioning, and arm64/iphoneos configuration were made deterministic.

## Failure boundary

The physical-device build reached compilation of JUCE `juce_events.mm` for `Debug-iphoneos/arm64` and failed there. The AIM Editor CMake target had explicitly set `XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES` for the whole application target. JUCE Apple module sources are not an ARC-only codebase and include manual Objective-C lifetime management, so forcing ARC at the target level also forces it onto JUCE module translation units.

## Fix

- Removed the target-wide `CLANG_ENABLE_OBJC_ARC=YES` override.
- The build now leaves Objective-C memory-management mode under JUCE/Xcode defaults instead of overriding every Objective-C++ source in the app target.
- Added a repository-build contract that rejects reintroduction of a target-wide ARC override.
- Physical-iPad builds now retain the full Xcode output in `build-ios-device/aimeditor-ipad-build.log` and extract compiler error lines automatically on failure.
- No application architecture, MIDI, data-model, or plugin targets were changed. AIM Editor remains a standalone app.

## Verification target

On EVE with the configured physical iPad connected:

```bash
./tools/build_ipad_device.sh
```

The gate must compile the real `iphoneos` arm64 application, sign it, install it to the configured iPad, and launch `com.rothamplification.aimeditor`.
