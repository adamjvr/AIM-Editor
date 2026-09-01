# MIDI and SysEx capture

AIM Editor includes a protocol-neutral MIDI/SysEx inspector for reverse engineering and hardware verification.

The inspector deliberately does **not** assume any Ion manufacturer, model, command, parameter, checksum, or edit-buffer bytes until those values are verified from documentation and/or hardware captures.

## Capture behavior

- Both MIDI input and MIDI output are recorded.
- Capture continues while the inspector window is hidden.
- The on-screen view can be filtered to SysEx only.
- JSON export always preserves the complete captured stream, regardless of the display filter.
- A capture is bounded to 4096 events to avoid unbounded memory use during long sessions.
- Incoming MIDI is marshalled to the JUCE message thread before the inspector touches UI state.

## JSON format

The canonical capture format is `aim-editor.midi-capture`, schema version 1.

Example:

```json
{
  "format": "aim-editor.midi-capture",
  "schema_version": 1,
  "exported_at_utc_ms": 1788300000000,
  "events": [
    {
      "direction": "input",
      "utc_ms": 1788300000123,
      "device_identifier": "example-midi-device",
      "kind": "sysex",
      "size": 8,
      "hex": "F0 00 00 0E 00 01 02 F7",
      "bytes": [240, 0, 0, 14, 0, 1, 2, 247],
      "sysex_payload_size": 6,
      "sysex_payload_hex": "00 00 0E 00 01 02"
    }
  ]
}
```

The example bytes above are illustrative only and must not be treated as verified Ion protocol framing.

The machine-readable schema is `schemas/midi-capture.schema.json`.

## Intended RE workflow

1. Connect the Ion as AIM Editor's MIDI input and output.
2. Open **sysex tools**.
3. Clear the capture.
4. Perform exactly one controlled action on the hardware or reference editor.
5. Export the JSON capture.
6. Repeat with a single changed parameter/value.
7. Diff the captures.
8. Record verified byte/NRPN mappings in the canonical parameter database.

This lets protocol discoveries become reproducible test fixtures instead of informal notes.

## Candidate Ion decoding

A 432-byte SysEx payload that passes the candidate Ion structural checks is decoded non-destructively. Exported capture JSON gains two optional objects:

- `ion_candidate_patch`: raw decoded metadata/checksum plus the full 378-byte decoded image;
- `ion_candidate_program`: the same image projected through the current human-readable `data/parameters.json` mappings.

These are explicitly candidate interpretations. The original `bytes`, `hex`, and `sysex_payload_hex` remain the source of truth.

The control bar can now send the candidate single-patch request for Red/Green/Blue/Yellow/Edit banks so the response can be captured and checked.

## Loading a captured candidate patch

When a 434-byte candidate Ion patch dump passes the structural checks and checksum, the inspector enables **Load Patch**. Loading updates AIM Editor's shared semantic `ProgramState`, so every duplicate control view refreshes from the same decoded program. The raw capture remains preserved separately and is still the authoritative evidence.


## NRPN analysis

`tools/protocol/ion_nrpn_capture.py` reads the same capture JSON and emits
`aim-editor.nrpn-capture-analysis` JSON. Incoming and outgoing directions are
tracked independently so interleaved monitor traffic cannot combine into a
false NRPN transaction.
