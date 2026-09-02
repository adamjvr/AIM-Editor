# Pass 28 checkpoint — signed app discovery + concurrent Apple gate

## Physical iPad build result entering Pass 28

The real `iphoneos`/arm64 AIM Editor application now compiles and Xcode reports
`BUILD SUCCEEDED`. The remaining Pass 27 failure was post-build tooling: the app
locator required the path to contain `iphoneos`, but CMake/JUCE emitted the signed
bundle at `AIMEditor_artefacts/<configuration>/AIM Editor.app`.

## Fix

- Resolve the application product from Xcode's generated `TARGET_BUILD_DIR` and
  `FULL_PRODUCT_NAME` build settings instead of guessing a directory pattern.
- Verify the resulting bundle with `codesign --verify --deep --strict` before install.
- Preserve a diagnostic fallback that lists discovered `.app` products when lookup fails.
- Bootstrap shared JUCE/Python prerequisites before concurrency so a fresh clone cannot race dependency creation.
- Run the macOS standalone app/CTest gate and physical-iPadOS gate concurrently from
  `tools/build_apple_targets.sh` using independent build directories.
- Prefix live output as `[macOS]` and `[iPadOS]` and retain separate logs in `build-logs/`.
- Wait for both jobs and fail the aggregate gate if either platform fails.

AIM Editor remains a standalone application. There is no simulator or plugin target.
