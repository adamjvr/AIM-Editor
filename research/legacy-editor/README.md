# Legacy editor reference research

This directory records evidence about the historical Windows Ion editor used as a behavioral reference for AIM Editor.

The original executable is **not** stored in this repository. Only hashes, structural observations, independently written analysis tools, and eventually behavioral fixtures belong here.

## Confirmed container structure

The reference executable is a UPX-packed PE32 application with an appended container. The final 27 bytes form a reverse-readable directory/footer:

```text
uint32_le  payload absolute file offset   = 0x001a9200
uint32_le  payload size                   = 0x00599819
byte[7]    name                           = "mod.sme"
uint32_le  name length                    = 7
uint32_le  entry count                    = 1
char[4]    magic                          = "OSSM"
```

`0x001a9200 + 0x00599819 = 0x00742a19`, exactly the first byte of this 27-byte footer. This verifies that the container describes one `mod.sme` payload occupying the complete PE overlay before the footer.

Run:

```bash
./tools/re/inspect_legacy_container.py "/path/to/ion-editor demo.exe"
```

The tool emits JSON and does not require Windows.

## Current next questions

1. Identify the encoding/compression used by the high-entropy prefix of `mod.sme`.
2. Establish the exact boundary between encoded data and the low-entropy serialized graph/object stream.
3. Recover object/module records and connections into JSON.
4. Locate MIDI/NRPN/SysEx primitives and constants in the recovered graph.
5. Cross-check every protocol conclusion against hardware captures or independent documentation.
