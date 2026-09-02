# Pass 27 checkpoint — explicit non-ARC iPad application target

## Evidence from the physical-device Xcode log

The physical arm64 iPadOS build reached JUCE Objective-C++ compilation. The generated
AIMEditor application target still contained `-fobjc-arc` even after the previous
explicit `CLANG_ENABLE_OBJC_ARC=YES` property had been removed. JUCE Apple module
sources use manual Objective-C ownership operations such as `retain`, `release`,
`autorelease`, and `dealloc`; Clang rejects those operations when ARC is enabled.

The static AIMEditorCore/AIMEditorMidi target arguments did not show the same ARC
flag, isolating the problem to Xcode's iOS application-target default.

## Fix

- Explicitly set `XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC NO` on `AIMEditor` for iOS.
- Keep the setting iOS-only; desktop targets are unchanged.
- Query the generated Xcode build settings before compilation and require
  `CLANG_ENABLE_OBJC_ARC = NO`.
- Abort with a direct diagnostic if the generated project ever reports ARC enabled.
- Update repository build validation to require the non-ARC boundary and reject an
  explicit `YES` regression.

AIM Editor remains a standalone JUCE application. No plugin target or simulator path
was added.
