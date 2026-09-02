# Program librarian

AIM Editor's librarian is intentionally split between a **portable semantic format** and a **hardware transport format**.

## Native files

### `.aimprogram.json`

A program is ordinary, diffable JSON.  Parameter IDs are semantic names rather than anonymous byte offsets.  When a program came from a real Ion patch dump, the JSON also retains the complete 378-byte decoded source image in `source_patch`.

### `.aimbank.json`

A native bank contains 128 logical slots but serializes only occupied slots.  It stores:

- a human-readable bank name;
- optional target hardware bank metadata (`red`, `green`, `blue`, `yellow`, `edit`);
- each occupied slot's complete native program JSON.

The native format is the safe archival/editing format even when protocol reverse engineering is incomplete.

## SysEx import

AIM Editor accepts standard `.syx` framing (`F0 ... F7`) and concatenated SysEx messages.  Checksum-valid candidate Ion single-program dumps are decoded into semantic state while retaining the full decoded source image.

A multi-message file can therefore populate the librarian without AIM Editor pretending that the Ion's still-unverified bank-stream protocol is understood.

## SysEx export safety

AIM Editor does **not** construct an Ion hardware patch from a blank byte array.  Hardware export requires a source-backed program:

1. start from the exact 378-byte decoded source patch;
2. overlay only fields currently mapped by the JSON parameter database;
3. retain unknown bytes and unknown bits in shared bitfields;
4. recalculate the candidate checksum;
5. apply 7-of-8 transport packing;
6. write standard SysEx framing.

This makes incomplete reverse engineering non-destructive.

For bank `.syx` export, every occupied native slot must be source-backed.  If a native `hardware_bank` is selected, the bank/slot header bytes are adjusted before re-encoding.  The Edit bank is constrained to its four candidate program slots.

## JSON-first rule

`.syx` is a hardware compatibility format.  JSON remains AIM Editor's canonical open interchange format so patches/banks can be inspected, diffed, generated, transformed, and validated independently of JUCE.
