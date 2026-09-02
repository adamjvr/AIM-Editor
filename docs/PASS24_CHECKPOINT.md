# Pass 24 checkpoint — deterministic EVE physical-iPad signing

Pass 24 fixes the physical-device signing regression exposed on EVE.

The prior helper incorrectly treated the parenthesized identifier in an `Apple Development` certificate display name as a development-team ID. That can select the wrong team even while Xcode is correctly signed in.

The device gate now:

- requires an explicit `AIM_EDITOR_DEVELOPMENT_TEAM`;
- optionally loads machine-local values from `.aim-editor-local.env`;
- never guesses a Team ID from `security find-identity`;
- supports pinning the exact physical iPad with `AIM_EDITOR_IPAD_DEVICE`;
- passes both `-allowProvisioningUpdates` and `-allowProvisioningDeviceRegistration`;
- remains physical-device-only (`iphoneos`, arm64, no simulator path);
- installs and launches the signed standalone app with `devicectl`.

For EVE, use the already-proven local values rather than committing personal deployment identifiers into the repository.
