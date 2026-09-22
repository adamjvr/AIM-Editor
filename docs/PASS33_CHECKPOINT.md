# AIM Editor Pass 33 checkpoint

## Scope

Pass 33 is the first post-architecture product-polish pass that changes the actual editor source, not just the research model or mockups.

## Device-aware editor

- The active device still selects the shared Ion-family hardware profile.
- **Ion mode** keeps the common Program editor and hides Micron-only controls.
- **Micron mode** exposes a dedicated `MICRON CONTROLS` surface instead of a generic parameter bucket.
- The Micron surface explicitly groups:
  - X / Y / Z parameter assignments
  - FX1 / FX2 balance
  - FX2 delay/reverb algorithm selection
- The shared Effects panel visibly changes to `FX1 / MOD FX` in Micron mode and points to the dedicated FX2 surface.
- Front-page profile text now describes the selected hardware shell and the shared 378-byte Program model.

## GUI repairs

- The highlighted OSCILLATORS / COMMON row now switches to a taller 2x2 layout below 700 px panel width.
- Parameter controls use an adaptive compact layout so text no longer consumes the entire ComboBox interaction area.
- Oscillator Sync is shortened to `Sync` inside the already-labelled COMMON group.
- Desktop page navigation is explicitly labelled `VIEW` and uses `Front / Dual 1 / Dual 2 / Random / Rear`.
- Full segmented view buttons are only used when there is enough room; narrower windows fall back to the page selector instead of ellipsizing labels.
- Each page button has a descriptive tooltip and keyboard shortcut.

## Branding

- The silver/red AIM Editor application icon remains wired into the JUCE target.
- The live editor banner now includes a matching procedural silver utility-device mark with a dark display and red controls.

## Verification policy

- The physical Micron is the active hardware-verification target.
- Ion behavior remains candidate where it has not been exercised on physical Ion hardware.
- Micron bank streaming and full Program writes remain disabled until verified.
