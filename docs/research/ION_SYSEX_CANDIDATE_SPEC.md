# Candidate Ion SysEx specification

AIM Editor now has an executable, machine-readable **candidate** description of the Ion/Micron single-program SysEx format in `data/protocol/ion-sysex.json`.

## Evidence level

The primary current source is Bernard Escaillas (BEE), *ION/MICRON PATCH DUMP SYSEX FORMAT* (2008):

- https://medias.audiofanzine.com/files/midi-sysex-472872.pdf

The HyperSynth editor manual independently states that a normal Ion/Micron/Miniak program SysEx is 434 bytes, which is consistent with the candidate format:

- https://www.hypersynth.com/download/Miniak-editor_User_Manual_rev6.pdf

This is community reverse-engineering material, not treated as an authoritative vendor specification. Every imported field is marked `candidate` until it is confirmed with independent hardware captures or another trustworthy source.

## Candidate single-patch request

Wire bytes:

```text
F0 00 00 0E 22 41 0x 00 yy F7
```

`x` is the bank number and `yy` is the zero-based patch slot. Current candidate bank mapping:

| raw | bank | slots |
| ---: | --- | ---: |
| 0 | Red | 128 |
| 1 | Green | 128 |
| 2 | Blue | 128 |
| 3 | Yellow/User | 128 |
| 4 | Edit | 4 |

A bank request changes the byte after the bank from `00` to `01` and uses slot `00`.

## 7-of-8 transport

A single program response is 434 wire bytes including `F0`/`F7`. The 432-byte SysEx payload consists of 54 groups of eight 7-bit MIDI-safe bytes. Each group reconstructs seven full-width bytes. That yields a 378-byte decoded image.

The first byte of each group carries the MSBs of the following seven bytes:

```text
m1 = 0abcdefg
m2 = 0ttttttt
m3 = 0uuuuuuu
...
m8 = 0zzzzzzz

b1 = attttttt
b2 = buuuuuuu
...
b7 = gzzzzzzz
```

AIM Editor implements this in both C++ (`IonSysExCodec`) and the stdlib-only Python research tool (`tools/protocol/ion_sysex.py`).

## Decoded patch image

Important candidate anchors:

- bytes `0..1`: Alesis ID `00 0E` after 7-of-8 decoding
- byte `2`: product ID `22`
- bytes `7..14`: `Q01SYNTH`
- bytes `15..18`: 32-bit big-endian checksum complement
- bytes `19..22`: firmware version text
- bytes `51..54`: big-endian patch data size, expected `315`
- byte `63`: start of the 315-byte program data
- bytes `63..77`: patch name (14 chars + NUL)
- byte `377`: end of decoded single-program image

## Checksum

Interpret 78 big-endian 32-bit words starting at offsets `63, 67, ... 371`. Add them modulo `2^32`. The stored checksum at offset 15 is the two's-complement value that makes:

```text
sum + checksum == 0 mod 2^32
```

## Deliberate uncertainty

We do not repair contradictions by intuition. For example, the 2008 document repeats offset `94` for both Oscillator 2 and Oscillator 3 shape, then uses `95` for an Oscillator 1 bitfield. `osc3.shape` therefore remains unmapped until independent evidence resolves it.

The same rule applies to boolean polarity and any UI concept that does not map one-to-one to a raw field.

## Current machine-readable coverage

`data/protocol/ion-sysex.json` currently contains:

- request framing;
- bank definitions;
- 7-of-8 transport metadata;
- checksum metadata;
- decoded header layout;
- candidate voice/oscillator/mixer/filter/envelope/LFO/arpeggiator/matrix/tracking/effects fields;
- explicit source-conflict notes.

`data/parameters.json` is reconciled against that spec. Candidate mappings are never promoted to `verified` automatically.

## Hardware verification workflow

1. Connect Ion MIDI IN and OUT.
2. Open **SysEx Tools** in AIM Editor.
3. Clear the capture.
4. Request one known patch or change exactly one hardware parameter.
5. Save the capture JSON.
6. Compare the raw bytes and candidate decoded program.
7. Promote only the fields proven by repeatable captures.

The raw capture remains authoritative even if a candidate decoder interpretation turns out to be wrong.
