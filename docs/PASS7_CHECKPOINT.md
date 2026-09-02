# Pass 7 checkpoint — guarded hardware transfer

Pass 7 turns the existing protocol research into a safer hardware-testing workflow.

## Added

- dedicated Ion hardware-transfer overlay;
- single-patch and candidate bank request controls;
- explicit Red/Green/Blue/Yellow/Edit destination context;
- guarded full-program send to Edit 1–4;
- mandatory real 378-byte source template for full writes;
- explicit write-arm gate that resets after every transmission;
- candidate patch-header retargeting in both C++ and the stdlib Python codec;
- SysEx inspector summary for the latest checksum-valid candidate patch;
- MIDI device rescan button in the global strip;
- candidate edit-buffer retarget/encode/decode tests.

## Deliberately not claimed

- hardware acceptance of the candidate edit-buffer/full-patch write;
- verified whole-bank streaming semantics;
- verified NRPN/SysEx parameter maps.

Those still require real Ion captures and A/B tests.
