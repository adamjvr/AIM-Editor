# Hardware verification workflow

AIM Editor deliberately distinguishes **candidate** protocol knowledge from hardware-verified knowledge. The fastest way to turn the current candidate maps into evidence is controlled A/B capture.

## One-parameter A/B test

1. Connect both MIDI directions between AIM Editor and the Ion.
2. Open **SysEx Tools**.
3. Request a known patch and save the capture as `before.json`.
4. On the Ion, change exactly one parameter by a known amount.
5. Request the edit-buffer patch again and save as `after.json`.
6. Run:

```bash
./tools/protocol/ion_patch_diff.py diff before.json after.json \
  --expected filter1.frequency \
  --output research/captures/filter1-frequency-001.diff.json
```

The report preserves raw decoded byte changes first, then overlays the current candidate field interpretation. A clean test where one expected candidate field changes and no unexplained bytes move is strong evidence, but it should still be repeated at multiple values before the mapping is marked verified.

## What to capture for each parameter

For continuous values, capture at least three points when practical: minimum, a middle value, and maximum. For signed values include negative, zero, and positive. For enumerations visit every option if the list is small. For packed bitfields, test neighboring controls independently so shared bytes can be separated by XOR masks.

## Evidence rules

- Never discard the raw `.syx` or MIDI-capture JSON.
- Do not call a field verified from a single unexplained observation.
- If multiple fields move, record that fact rather than choosing the most convenient one.
- If the candidate document disagrees with hardware, hardware wins and the contradiction remains documented.
- Checksums must validate before a dump is used to promote protocol data.

## Why the diff tool matters

Alesis packs seven full-width bytes into eight MIDI-safe bytes, and several controls share bitfields. Comparing the wire-level SysEx directly is therefore noisy. `ion_patch_diff.py` first restores the 378-byte candidate image and then reports both byte-level XORs and field-level changes. This turns hardware testing into repeatable JSON evidence instead of handwritten notes.


## Analyze live NRPN captures

The inspector JSON can be converted into semantic candidate transactions:

```bash
./tools/protocol/ion_nrpn_capture.py analyze capture.json --direction input --output nrpn-input.json
./tools/protocol/ion_nrpn_capture.py analyze capture.json --direction output --output nrpn-output.json
```

Each decoded transaction records the NRPN number, raw 14-bit value, signed
conversion when applicable, candidate parameter ID, and whether the observed
value falls inside the current candidate range. Unknown NRPNs remain visible
rather than being discarded.
