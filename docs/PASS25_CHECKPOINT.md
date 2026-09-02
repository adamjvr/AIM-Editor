# Pass 25 checkpoint — deterministic explicit iPad selection

Pass 25 fixes two device-selection bugs exposed by the first Pass 24 run on EVE.

- Machine-local `.aim-editor-local.env` values are passed explicitly to the CoreDevice selector; the helper no longer depends on shell variables being exported implicitly.
- A failing selector subprocess now terminates the build gate immediately; it cannot fall through into unset `AIM_IPAD_*` variables.
- Explicit-UDID mode matches the configured physical iPad before applying online-state checks, so a known-but-unavailable device produces its real pairing/tunnel/boot/transport/DDI state instead of the misleading “no iPad” message.
- Auto-selection still considers only connected physical iPads, rejects simulators, and requires Developer Mode.
- The canonical EVE deployment remains physical `iphoneos`/arm64 with automatic signing, install, and launch.
