# Save-aware document workflow and verification reporting

Pass 12 closes two workflow gaps: unsaved work can now be saved *inside* a destructive-action/quit flow, and the current protocol-verification state can be summarized without reading the raw JSON databases by hand.

## Save-aware destructive actions

Program and bank dirty state remain independent. When a destructive action would replace either document, AIM Editor now offers three choices:

- **Save & Continue** — writes the dirty native JSON document first, asking for a path when it has never been saved.
- **Discard & Continue** — performs the requested action without writing the dirty document.
- **Cancel** — leaves the current editor state untouched.

Application quit follows the same model with **Save & Quit**, **Quit Without Saving**, and **Cancel**. If a save dialog is cancelled or a write fails, the destructive action/quit is aborted.

A new **New Program** action creates the deterministic semantic Init program from the JSON registry. It deliberately has no SysEx source template; AIM Editor does not invent unknown hardware bytes. The untouched Init becomes the new clean baseline and becomes dirty only after semantic edits.

Native JSON remains the safe save format. `.syx` export still requires a real source patch template and is not used as an automatic document-save target.

## Verification coverage report

Generate a human-readable report:

```bash
./tools/protocol/ion_verification_report.py report
```

Write Markdown or JSON to a file:

```bash
./tools/protocol/ion_verification_report.py report \
  --format markdown \
  --output research/verification/STATUS.md

./tools/protocol/ion_verification_report.py report \
  --format json \
  --output research/verification/status.json
```

The report reads the canonical parameter database and sealed records under `research/verification/`. It reports NRPN/SysEx mapped, candidate, verified, and unmapped counts; evidence result counts; invalid evidence seals; and contradictions requiring review.

The report is read-only. It never promotes mappings. `ion_protocol_verification.py apply` remains the only mechanical promotion path.
