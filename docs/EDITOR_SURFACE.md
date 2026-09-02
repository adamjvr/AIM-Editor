# Purpose-built editor surface

AIM Editor's UI is not a direct bitmap clone of the legacy Windows program. It
preserves the Ion's editing topology while using responsive JUCE components that
scale cleanly across macOS, iPadOS, Windows, and Linux.

## Shared state rule

Every visible control reads/writes the same `ProgramState`. Front, Dual 1, Dual
2, Randomizer, Rear, the librarian, imported JSON, incoming NRPN, and decoded
SysEx are views or producers of one semantic model. No panel owns a private copy
of a synth parameter.

## Purpose-built blocks

### Oscillators

`OscillatorPanel` presents OSC 1/2/3 as three parallel hardware-like lanes and a
separate common strip. The controls are still generated from `parameters.json`;
only their visual composition is specialized.

### Filters

`FilterPanel` presents Filter 1 and Filter 2 as parallel lanes with an explicit
routing/common strip. This replaces the earlier anonymous combined parameter
bucket.

### Envelopes

`EnvelopePanel` contains independent Pitch, Filter, and Amp lanes. Each lane has
an interactive curve display backed directly by the canonical attack, decay,
sustain, and release parameter IDs. Mouse drag and iPad touch both use normal
JUCE pointer events, so no mobile-only state path exists.

The curve is intentionally a semantic editing aid, not an attempt to claim that
its screen-space segment lengths reproduce undocumented firmware timing. Dragging
updates the same raw parameter values as the knobs; engineering display text is
calculated only from transforms explicitly present in JSON.

### Mod Matrix and Tracking Generator

The Mod Matrix and Tracking Generator remain dedicated editors because tabular
routing and a 33-point curve cannot be represented well by generic knobs.

## Responsive page composition

At large landscape widths, Front/Dual pages use explicit signal-oriented rows:

- Front: oscillator/pre-filter/filter path; modulation/voice/post/output/effects;
  envelopes; Mod Matrix.
- Dual 1: oscillator/pre-filter/filter path; modulation/voice/post/output; Mod
  Matrix.
- Dual 2: pre-filter/filter/post/output; modulation/voice/effects; envelopes;
  Mod Matrix.
- Rear: Mod Matrix and Tracking Generator side-by-side.

At smaller widths the same components automatically fall back to a masonry
layout. This keeps the editor usable on smaller desktop windows and portrait
orientations without creating a second UI implementation.

## Display transforms

`ParameterFormatter` executes only display-transform expressions that are
explicitly recorded in `data/parameters.json`. Unknown transforms display as
`raw N`, rather than attaching a possibly false engineering unit. This keeps
reverse-engineering uncertainty visible while allowing known Hz/ms/% transforms
to be useful immediately.
