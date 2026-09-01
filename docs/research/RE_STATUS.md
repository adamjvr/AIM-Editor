# Reverse-engineering status

## Confirmed from the original executable

- 32-bit x86 Windows PE.
- UPX-packed.
- Visual C++/MFC-era runtime characteristics.
- Large appended overlay containing structured application data.
- Runtime imports expose MIDI/WinMM, HID/SetupAPI, GDI/GDI+, common controls, and other generic facilities.
- The exported program appears consistent with a SynthMaker/early FlowStone-style graphical-programming runtime.

## Confirmed from reference screenshots

The editor exposes five pages: Front, Dual 1, Dual 2, Randomizer, and Rear. Visible systems include oscillators, pre-filter mixing, dual filters, post-filter mixing, output, effects, envelopes, LFOs, sample-and-hold, voice/unison/portamento, a 12-slot modulation matrix, tracking generator, randomizer, program controls, and MIDI I/O.

## Not yet verified

- Exact Ion manufacturer/model SysEx framing used by this editor.
- NRPN numbers.
- SysEx offsets/bit fields.
- Value curves and display transforms.
- Patch/bank checksum rules.
- Edit-buffer request/update state machine.
- Whether HID/SetupAPI are Ion-specific or generic runtime baggage.

Unknown protocol fields remain `null` in JSON until verified.
