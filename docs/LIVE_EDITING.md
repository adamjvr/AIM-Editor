# Live parameter editing

AIM Editor can send candidate Ion NRPN messages while a user changes controls.
This path is intentionally **opt-in** while the protocol database is still being
verified against real hardware.

## Safety model

- `Live NRPN` starts **off** every time the app launches.
- Only changes marked `ProgramChangeOrigin::interactive` may transmit.
- Loading a captured patch, importing JSON, resetting state, or later receiving
  protocol updates does **not** echo parameter changes back to MIDI.
- Parameters without a mapped NRPN and recognized value encoding are silent.
- The MIDI channel is explicit in the persistent control bar.
- Complete incoming NRPN sequences on that channel update the shared editor
  state but are tagged `protocolInput`, so they are never echoed back out.
- The current candidate database supports `unsigned_14` and the documented
  Alesis-style `signed_14_wrap` representation.

The current understanding is that NRPN parameter edits affect the synth edit
buffer rather than directly overwriting a stored program. That behavior still
needs hardware verification, so the UI keeps the candidate status visible.

## Data flow

```text
JUCE control
    |
    v
ProgramState::setValue(... interactive)
    |
    +----> all duplicate UI views update
    |
    v
IonParameterTransmitter   [only when Live NRPN is enabled]
    |
    +----> ParameterRegistry JSON mapping
    |
    +----> unsigned_14 / signed_14_wrap encoder
    |
    v
4-message NRPN sequence
CC 99 -> CC 98 -> CC 6 -> CC 38
    |
    v
IonMidiService -> selected MIDI output

selected MIDI input
    |
    v
IonNrpnDecoder -> JSON NRPN lookup -> ProgramState(... protocolInput)
    |
    v
all duplicate UI views update, with no MIDI echo
```

## Verification workflow

Open **SysEx Tools** before changing a control. Outgoing NRPN controller messages
are captured by the same MIDI monitor used for SysEx. Compare the outgoing
parameter number/value against the hardware response and the reference editor.

When a mapping is proven, change its JSON protocol status/evidence rather than
hard-coding the result in the UI.
