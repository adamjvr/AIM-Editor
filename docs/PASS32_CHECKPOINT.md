# AIM Editor Pass 32 checkpoint

## Scope

Pass 32 turns the Ion/Micron family foundation into an explicitly device-aware product shell, with the physical Micron as the current verification target.

## Device-aware UI

- The Front-page identity switches between **Alesis ION Editor** and **Alesis Micron Editor**.
- Ion mode hides Micron-only semantic controls.
- Micron mode exposes X/Y/Z performance assignments, FX1/FX2 balance, and FX2 type from provenance-backed Micronau evidence.
- These five fields remain candidate mappings until captured on the user's physical Micron.
- Ion protocol evidence remains candidate until physical Ion hardware becomes available.

## Navigation and layout

- Desktop segmented page navigation now uses `Front / Dual 1 / Dual 2 / Random / Rear` instead of cryptic initials.
- The Oscillator `COMMON` strip reflows to a 2x2 control grid at constrained widths, addressing the truncation visible around Oscillator Sync, FM Source, and Noise Color.

## Branding

- Added an original silver/red hardware-utility AIM Editor icon and editable SVG source under `assets/branding/`.
- The PNG is wired to JUCE `ICON_BIG` / `ICON_SMALL`.
- The artwork is intentionally original and only takes high-level inspiration from professional MIDI utility iconography.

## Safety

- Micron bank-stream requests remain disabled.
- Micron full Program writes remain disabled.
- Hardware verification remains per-device.

## Verification

- 217 unique semantic parameters.
- 4 reusable enum tables.
- 234 candidate NRPN definitions.
- 265 candidate SysEx fields.
- 199/217 editor parameters carry candidate NRPN mappings.
- 190/217 editor parameters carry candidate SysEx mappings.
- Full repository static/protocol/schema/safety gate passes in the Pass 32 packaging environment.
- Apple compile/sign/install remains a physical-EVE gate.
