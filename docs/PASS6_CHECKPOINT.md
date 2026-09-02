# Functional Pass 6 checkpoint

Pass 6 concentrates on the editor surface rather than adding another transport
layer.

## Purpose-built blocks added

- `ModulatorPanel`: LFO 1, LFO 2, Sample & Hold, and tempo/arpeggiator timing.
- `VoicePanel`: unison/drift, portamento, mono/poly and pitch-wheel behavior.
- `EffectsPanel`: effect tone/modulation/mode controls while keeping algorithm-
  dependent semantics visibly provisional.
- `MixerPanel`: signal-oriented pre-filter and post-filter mixer channels.
- `OutputPanel`: drive, output level/effect mix, and routing controls.

These panels bind the same JSON-backed `ProgramState` as every other surface;
there is still no MIDI knowledge in the widgets.

## Navigation / geometry

Large landscape surfaces now use a five-button segmented page selector instead
of spending a large control on a desktop-style drop-down. Compact windows retain
the drop-down. The explicit Front/Dual layout threshold was lowered so smaller
landscape tablets use the signal-oriented composition instead of unnecessarily
falling back to masonry.

The page header was also reduced to reclaim vertical space. All controls remain
procedural/high-DPI and use the same layout code on desktop and iPadOS.

## Human-readable enum refinement

A small set of names documented by the Ion reference manual was added to
`parameters.json` for LFO/S&H reset, portamento trigger, and pitch-wheel mode.
These names do not promote candidate raw protocol mappings to verified status.
See `docs/research/ION_MANUAL_UI_ENUMS.md`.

## Verification

`tools/check_repository.py` remains the pass-level gate. Hardware verification
is still required before candidate protocol mappings can be promoted.
