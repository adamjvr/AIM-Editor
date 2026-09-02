# Pass 5 checkpoint — purpose-built synth surface

This checkpoint moves AIM Editor from generic JSON control grids toward the
actual editing topology visible in the reference application and Ion hardware.

## Added

- purpose-built three-lane Oscillator panel;
- purpose-built dual-lane Filter panel;
- purpose-built three-lane Pitch/Filter/Amp Envelope panel;
- touch/mouse envelope curve editing for attack, decay, sustain, and release;
- large-landscape Front, Dual 1, Dual 2, and Rear page compositions;
- automatic masonry fallback at smaller widths;
- improved procedural Ion knob rendering with scale ticks and focus ring;
- hardware-style toggle/LED rendering and dark selector rendering;
- central `ParameterFormatter` for JSON-declared engineering transforms;
- conservative raw-value fallback for unknown/unimplemented transforms;
- editor-surface JSON validator integrated into repository checks.

## Data/model invariants

No new synth protocol values were invented in this pass. All new editor widgets
bind existing semantic IDs in `data/parameters.json` through `ProgramState`.
Envelope curve manipulation therefore follows the same live-edit safety path as
any other interactive control.

The formatter currently executes documented transforms for 85 of 212 semantic
parameters. The other 127 explicitly remain `unknown` and are shown as raw
values until research supplies a trustworthy transform.

## Cross-platform intent

The new surface uses JUCE vector/procedural drawing only; no DPI-specific bitmap
knobs were introduced. Pointer-based envelope editing works through JUCE's normal
mouse/touch event abstraction, so macOS, iPadOS, Windows, and Linux all share one
implementation.

## Still pending

- first full compile/run against a real JUCE checkout on Rosie/macOS;
- screenshot-driven geometry tuning on desktop and iPad;
- more specialized LFO/effects/voice widgets;
- verified hardware display curves and enum labels;
- accessibility labels/focus order;
- verified edit-buffer writes.
