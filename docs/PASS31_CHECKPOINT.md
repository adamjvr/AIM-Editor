# Pass 31 checkpoint — Ion/Micron family architecture

## Closed in this pass

- Added explicit **Ion** and **Micron** device profiles around the shared 378-byte Program image.
- Added a persistent, non-destructive DEVICE selector to the global hardware context.
- Ion single-Program requests continue to use product ID `0x22`.
- Micron single-Program requests now use independently corroborated product ID `0x26` while Program dumps remain shared `0x22`.
- Micron request addressing exposes 8 banks x 128 Programs from the independent implementation evidence.
- Hardware-transfer overlay follows the selected device profile.
- Ion bank requests and guarded Edit 1-4 writes remain available in Ion mode.
- Micron bank requests and full Program writes remain deliberately disabled until their storage/destination semantics are physically verified.
- Added canonical `data/device-profiles.json` and schema.
- Added pinned `micronau` evidence summary without copying GPL implementation code.
- Added static guards for the device split, clean implementation boundary, shared Program size, known Micron extensions, and unresolved conflicts.

## Known evidence holds

- NRPN 105 conflict: `lfo1.sync` vs Micron file ID.
- NRPN 250 conflict: synced FX1 interpretation vs fifth selected FX2 parameter.
- Micron bank-stream request behavior is unknown.
- Micron full Program write destination/storage semantics are unknown.

These remain candidates until controlled hardware captures resolve them.

## Next

1. Build/launch macOS + physical iPadOS concurrently.
2. UI-polish the new DEVICE selector at both target sizes.
3. Connect the physical Ion and close the first request/dump/NRPN evidence loop.
4. Add visible Micron FX2/X-Y-Z controls behind the Micron capability profile once the family data layer is stable.
