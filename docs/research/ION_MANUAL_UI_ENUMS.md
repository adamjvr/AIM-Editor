# Ion manual-derived editor labels

This checkpoint adds a small set of **human-facing enum labels** from the Alesis
Ion reference manual so the purpose-built editor does not show anonymous numeric
selectors where the hardware documentation gives names.

Primary reference:

- Alesis, *Ion Reference Manual*, especially Program Parameters pages 43-51.
- https://www.manualslib.com/manual/207496/Alesis-Ion.html

The manual documents:

- LFO Reset: `mono`, `poly`, `key-mono`, `key-poly`;
- Sample & Hold Reset: the same four modes;
- Portamento Type: fixed/scaled and fixed/scaled glissando variants;
- Portamento Trigger: `normal`, `legato`;
- Pitch Wheel Mode: `held`, `all`;
- Voice mode: `mono`, `poly`.

## Confidence rule

The *names and ordering shown in the manual* are useful UI evidence. The mapping
from those ordinal positions to every SysEx/NRPN raw value remains **candidate**
until captured on hardware. `data/parameters.json` therefore gains labels for the
known ordinal values without changing any `candidate` protocol status to
`verified`. Where the transport sources expose a wider numeric range than the
manual describes, the extra values remain representable as `Unknown N` rather
than being coerced.
