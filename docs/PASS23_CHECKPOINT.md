# Pass 23 checkpoint — physical iPad only

Pass 23 replaces the temporary iPadOS Simulator verification path with the actual product-development target: a connected physical iPad.

## Policy

AIM Editor is a standalone application. iPadOS verification is performed only on Apple hardware; the repository no longer supplies a simulator build helper or simulator CI job. Historical simulator checkpoints remain as an audit trail only.

## Device gate

`tools/build_ipad_device.sh`:

1. bootstraps the pinned JUCE/Python tooling;
2. requires Xcode, the physical `iphoneos` SDK, and `devicectl`;
3. selects a connected, paired, non-simulated iPad and checks Developer Mode;
4. discovers or accepts an Apple development team;
5. cleans the iOS cross-build tree;
6. configures `iphoneos`, arm64, iPadOS 17+, automatic signing, and the iPad-only application target;
7. builds specifically for the selected device with provisioning updates allowed;
8. installs the resulting app with `devicectl`;
9. launches `com.rothamplification.aimeditor` on the device.

Expected marker:

```text
PASS: AIM Editor physical iPadOS application build/install/launch
```

The iCloud entitlement added during the simulator-preparation pass is removed from the initial hardware gate. Native document-browser and file-sharing support remain enabled without making first-run provisioning depend on an iCloud container.
