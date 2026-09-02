# Hardware transfer safety model

AIM Editor now has a dedicated hardware-transfer surface for candidate Ion SysEx operations.
The protocol framing remains **candidate** until verified against independent Alesis Ion captures.

## Read operations

The hardware panel can request:

- one selected program from Red, Green, Blue, Yellow/User, or Edit;
- a candidate whole-bank transfer request.

Incoming MIDI is still captured by the SysEx inspector. A single-patch response is only offered to the semantic editor when it passes the current structural checks and checksum validation.

## Full-patch writes

AIM Editor does **not** construct an Ion patch from a blank byte array. Full writes require all of the following:

1. the current program must contain a complete 378-byte decoded source patch captured/imported from hardware;
2. the user must explicitly enable **Arm candidate full-patch writes**;
3. an Edit buffer slot (1–4) must be selected;
4. the write must be triggered explicitly.

The arm switch resets after every write.

Before transmission AIM Editor:

1. copies the original source patch;
2. changes only the candidate destination header to the selected Edit slot;
3. overlays semantic fields that AIM Editor currently understands;
4. preserves all unmapped bytes and untouched bitfields from the source template;
5. recalculates the candidate checksum;
6. applies Alesis 7-of-8 packing;
7. sends one complete SysEx message.

This design keeps unknown data intact while protocol archaeology is incomplete.

## Offline verification

The Python codec mirrors the retargeting behavior and can be tested without JUCE or hardware:

```bash
./tools/protocol/ion_sysex.py self-test
./tools/protocol/ion_sysex.py retarget source.syx edit3.syx --bank edit --slot 2
```

`--slot` is zero-based at the protocol/research CLI level, so `--slot 2` means Edit 3.

## Verification status

A successful encode/decode/checksum self-test proves internal consistency only. It does **not** prove that the Ion accepts the candidate write framing. Promote the write protocol to verified only after controlled hardware captures/tests are recorded with provenance.
