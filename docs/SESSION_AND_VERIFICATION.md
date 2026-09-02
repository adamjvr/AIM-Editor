# Session restore and verification capture

AIM Editor now persists only non-destructive editor context between launches:

- selected page;
- MIDI channel;
- hardware bank/program selector context;
- selected MIDI input/output identifiers.

The session store intentionally does **not** persist live NRPN enable state,
full-patch write arming, transient undo history, or any other setting that could
cause a launch to transmit MIDI. Restoring a session may reopen still-present
MIDI endpoints, but sends no request, parameter change, or patch write.

## Candidate NRPN verification records

The MIDI/SysEx Inspector still preserves every raw MIDI event as authoritative
capture data. In addition, it now reconstructs complete candidate NRPN
transactions from CC99/CC98/CC6/CC38 sequences using separate input and output
state machines.

Each exported `nrpn_transactions` entry records:

- direction and timestamp;
- MIDI endpoint identifier and channel;
- raw NRPN number and 14-bit value;
- candidate semantic parameter ID/name when one exists;
- current mapping status (`unmapped`, `candidate`, or `verified`);
- value encoding and decoded semantic value when known.

This is evidence, not automatic verification. Capturing a transaction never
changes `data/parameters.json` or promotes a candidate mapping. Promotion to
`verified` remains an explicit research decision backed by repeatable hardware
observations.

A useful hardware verification loop is:

1. Clear the Inspector capture.
2. Move exactly one Ion hardware control through a few known positions.
3. Export the capture JSON.
4. Compare the reconstructed NRPN transactions with the raw CC events.
5. Repeat in the opposite direction from AIM Editor with Live NRPN explicitly enabled.
6. Only then update the parameter evidence/status in the canonical JSON database.
