# Pass 29 checkpoint — first Apple UI cleanup

Pass 29 starts visual cleanup after the first successful native macOS and physical-iPad runs.

## UI changes

- Parameter controls now use context-aware compact captions inside purpose-built sections. The semantic parameter database remains unchanged and the full parameter name remains available in the tooltip.
- Caption space is increased to reduce multi-line collision.
- Front/rear segmented page tabs use compact single-character labels where the fixed transport bar is narrow.
- The Effects explanatory footer is shortened to avoid needless truncation.

## iPad presentation

- The iPad application declares full-screen landscape presentation and hides the system status bar. This removes the first-run overlap between the system status region and AIM Editor's page header while also satisfying the landscape-orientation requirement.

## Apple gate

`./tools/build_apple_targets.sh` continues to build macOS and physical iPadOS concurrently. After the macOS build and CTest pass, the built `AIM Editor.app` is launched automatically. The iPad branch continues to sign, verify, install, and launch on the selected physical device.

No simulator target and no plugin target are introduced.
