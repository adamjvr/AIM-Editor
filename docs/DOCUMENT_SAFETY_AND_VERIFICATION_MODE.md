# Document safety and controlled verification mode

Pass 11 adds two independent safety layers: semantic unsaved-state tracking and a guided NRPN verification capture mode.

## Unsaved semantic state

`ProgramDocumentTracker` compares the live `ProgramState` against the last clean program baseline. A clean baseline is established when a program is imported/captured or when native program JSON is successfully saved. Interactive edits and internal undo/redo navigation are compared against that baseline, so undoing all the way back to the saved state clears the dirty flag naturally.

The librarian separately compares its 128-slot `ProgramBank` against the last loaded/saved bank baseline. Bank metadata, slot stores, clears, copy/paste operations, and other bank mutations therefore become visible as unsaved state.

Potentially destructive operations are guarded:

- loading a librarian slot over an edited program asks before discarding the program edits;
- creating/importing a bank asks before discarding unsaved bank edits;
- importing `.syx`, which may replace both program and bank state, checks both;
- application quit checks both program and bank state and requires explicit **Quit Without Saving** confirmation.

These checks are editor/document safety only. They do not transmit MIDI and do not change the existing hardware-write arming rules.

## Controlled NRPN verification test

The MIDI/SysEx Inspector now has **Start Test** and **Stop Test** controls around the existing verification parameter tag.

A controlled test works like this:

1. Enter the semantic parameter ID under test, for example `filter1.frequency`.
2. Click **Start Test**. The prior capture is cleared, the parameter ID is locked, the view switches to show controller traffic, and a start timestamp is recorded.
3. Move only the intended Ion control through several distinct positions.
4. Click **Stop Test**. The capture becomes frozen, so unrelated MIDI arriving afterward cannot contaminate the evidence file.
5. Confirm **Only this control moved** only if that statement is true.
6. Save JSON.

The inspector reports the candidate NRPN number, number of complete transactions, number of distinct semantic values, and whether competing NRPN numbers were observed. This is a structural readiness hint only; it does **not** promote a mapping.

The exported `verification_context` records the experiment timestamps and the inspector's observed transaction counts. Offline `ion_protocol_verification.py` independently reconstructs the raw CC99/CC98/CC6/CC38 stream and cross-checks those in-app counts before evidence can be promotable.

Raw MIDI events remain authoritative throughout the workflow.
