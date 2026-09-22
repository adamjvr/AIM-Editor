# Reverse-engineering status

## Confirmed from the original executable

- 32-bit x86 Windows PE.
- UPX-packed.
- Visual C++/MFC-era runtime characteristics.
- Large appended overlay containing structured application data.
- The overlay contains one `OSSM` directory entry named `mod.sme`; the payload
  occupies most of the executable and is the main target for graphical-project
  recovery.
- Runtime imports expose MIDI/WinMM, HID/SetupAPI, GDI/GDI+, common controls,
  and other generic facilities.
- The exported program appears consistent with a SynthMaker/early
  FlowStone-style graphical-programming runtime.

## Confirmed from reference screenshots

The editor exposes five pages: Front, Dual 1, Dual 2, Randomizer, and Rear.
Visible systems include oscillators, pre-filter mixing, dual filters,
post-filter mixing, output, effects, envelopes, LFOs, sample-and-hold,
voice/unison/portamento, a 12-slot modulation matrix, tracking generator,
randomizer, program controls, and MIDI I/O.

## Candidate protocol knowledge implemented

None of this section is promoted to hardware-verified yet.

- Ion/Micron manufacturer/product framing and patch request messages.
- 7-of-8 SysEx transport codec.
- 378-byte decoded single-program image from a 434-byte wire message.
- `Q01SYNTH` structural tag and 315-byte program-data size check.
- Candidate 32-bit checksum-complement algorithm.
- 263 candidate decoded SysEx fields.
- Candidate single-patch request path in the GUI.
- Live MIDI/SysEx inspector with raw JSON capture and optional candidate decode.
- 234 candidate NRPN definitions.
- Alesis candidate signed-14-bit NRPN semantics.
- 190/217 semantic parameters mapped to candidate SysEx fields.
- 199/217 semantic parameters mapped to candidate NRPN addresses.
- 33 candidate Tracking Generator curve points promoted into the canonical semantic database.
- Modulation source/destination and filter-type enum datasets.

## Deliberate unresolved items

- Hardware verification of all request/dump framing.
- Bank-number discrepancy in surviving community material.
- Several contradictory raw ranges between SysEx and NRPN sources.
- Exact edit-buffer update transaction.
- Program/bank **encoding** and byte-perfect round trips; decoding is further
  ahead than encoding.
- A few ambiguous UI concepts with no proven one-to-one raw field.
- Exact parameter-display conversions where sources disagree or are absent.
- Whether legacy HID/SetupAPI imports were application-specific or generic
  runtime baggage.
- Full recovery of the serialized `mod.sme` graphical project.

The JSON research files keep confidence/evidence metadata and preserve
contradictions rather than silently choosing convenient answers.


## Legacy container refinement

The OSSM footer parser now independently extracts the single `mod.sme` payload
from a local copy of the reference executable. The extracted payload is
5,871,641 bytes with SHA-256
`10ff27af3cd043ad106e1d71a009008d716578a71c0e19f2ce551beede2df9de`. A
4 KiB statistical profile detects a sharp entropy transition at the window
beginning `0x00201000`; this is a useful probe point, **not** yet an asserted
serialization boundary. Original binaries/payloads remain excluded from the
repository.
