# Protocol verification and promotion

AIM Editor now has a deliberate boundary between **candidate reverse-engineering data** and **hardware-verified data**. A candidate is never promoted because it looks plausible or because the old editor appears to use it. Promotion requires a controlled hardware experiment, raw evidence, an explicit human statement that the tested control was isolated, and a sealed JSON verification record.

## Capture tagging in AIM Editor

The MIDI / SysEx Inspector has a **VERIFY** row. Enter the semantic parameter ID being tested, for example `filter1.frequency`. The inspector checks the ID against the loaded parameter registry and shows its current NRPN/SysEx mapping status. Enable **Only this control moved** only when that is actually true for the experiment. Add a short note describing the sweep or A/B values.

Those fields are written to `verification_context` in the exported `aim-editor.midi-capture` JSON. They are annotations only; the raw MIDI bytes remain authoritative and are not modified.

## Verify an NRPN mapping

For a continuous control, move only that control through at least three distinct values while recording incoming MIDI. Then export the capture and run:

```bash
./tools/protocol/ion_protocol_verification.py nrpn \
  capture.json \
  --parameter filter1.frequency \
  --direction input \
  --confirm-isolation \
  --output research/verification/filter1-frequency-nrpn-001.json
```

If the capture itself already carries the parameter tag, `--parameter` may be omitted. The verifier reconstructs NRPN transactions independently from the raw CC99/CC98/CC6/CC38 stream and checks:

- the candidate NRPN number is actually observed;
- the semantic parameter ID remains consistent;
- the app-generated NRPN interpretation agrees with the offline reconstruction when embedded transactions are present;
- values fit the candidate encoding/range;
- enough distinct values were observed;
- no competing NRPN number appeared in the isolated experiment;
- the human isolation confirmation is present.

Boolean/two-state parameters require two distinct values; larger domains require at least three for address/encoding promotion.

## Verify a SysEx field

First make at least two independent A/B experiments. For each experiment request/capture a patch before and after changing only one control, then create diff reports:

```bash
./tools/protocol/ion_patch_diff.py diff before-1.json after-1.json \
  --expected filter1.frequency \
  -o research/captures/filter1-frequency-001.diff.json

./tools/protocol/ion_patch_diff.py diff before-2.json after-2.json \
  --expected filter1.frequency \
  -o research/captures/filter1-frequency-002.diff.json
```

Then seal the repeated evidence:

```bash
./tools/protocol/ion_protocol_verification.py sysex \
  research/captures/filter1-frequency-001.diff.json \
  research/captures/filter1-frequency-002.diff.json \
  --parameter filter1.frequency \
  --confirm-isolation \
  -o research/verification/filter1-frequency-sysex-001.json
```

The patch-diff tool now treats `header.checksum` as a **derived field**. It remains visible in the report but no longer falsely makes every one-parameter edit look like a two-field change.

## Promote canonical data

Review the evidence JSON. If `result` is `verified`, `promotable` is `true`, and the record is stored under `research/verification/`, apply it:

```bash
./tools/protocol/ion_protocol_verification.py apply \
  research/verification/filter1-frequency-nrpn-001.json
```

Promotion is idempotent and updates all relevant human-readable sources of truth:

- `data/protocol/ion-nrpn.json` or `data/protocol/ion-sysex.json`;
- the matching evidence block in `data/parameters.json`;
- the aggregate parameter status, but **only** when every protocol mapping used by that parameter has been verified.

For example, verifying NRPN while SysEx remains candidate leaves the aggregate parameter status `candidate`. This prevents a verified live-edit address from accidentally implying that the patch-dump byte mapping is also verified.

## Evidence integrity

Every verification document has a deterministic `evidence_id` of the form `sha256:…`. `apply` recomputes the digest and refuses edited or corrupted evidence. Each source capture/diff also carries its own SHA-256 in the evidence record.

The promotion validator additionally requires every canonical `verified` mapping to reference an existing evidence file under `research/verification/`. A bare `"status": "verified"` with no evidence will fail repository checks.
