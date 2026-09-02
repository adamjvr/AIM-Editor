# AIM Editor roadmap

## Phase 0 — foundation (current)

- [x] JUCE/CMake project skeleton
- [x] macOS/iPadOS/Windows/Linux target strategy
- [x] JSON-first parameter database
- [x] JSON Schema for parameter database and programs
- [x] typed parameter registry
- [x] semantic program model
- [x] observable ProgramState shared by duplicate editor views
- [x] JSON enum-domain loading and control factory
- [x] nested human-readable program JSON export/import
- [x] sparse human-readable bank JSON export/import
- [x] complete 378-byte source patch preservation inside JSON when available
- [x] preservation container for unknown raw bytes
- [x] MIDI device service
- [x] generic NRPN message builder
- [x] candidate Alesis signed-14-bit NRPN value codec
- [x] five-page responsive GUI shell
- [x] bottom MIDI/program control strip
- [ ] select project license

## Phase 1 — protocol archaeology

- [ ] unpack original executable completely
- [ ] recover/parse serialized SynthMaker/FlowStone project payload
- [x] find archived Ion MIDI/SysEx documentation and preserve provenance (candidate/community sources; hardware verification pending)
- [x] identify candidate manufacturer/model SysEx framing
- [x] map candidate single-patch request message
- [ ] map edit-buffer update message
- [x] map candidate single-program dump framing
- [ ] map/verify bank-dump streaming behavior
- [x] implement candidate 7-of-8 + checksum rules
- [x] import 234 candidate NRPN IDs
- [ ] verify NRPN IDs and transport semantics on hardware
- [x] add confidence + evidence records to JSON
- [x] build raw MIDI/SysEx inspector and JSON capture logger

## Phase 2 — verified data model

- [ ] complete parameter value domains
- [ ] complete enum tables (3 reusable tables + inline domains are loaded; incomplete candidate domains remain explicit)
- [ ] complete display transforms (Hz, ms, %, semitones, etc.)
- [ ] map every SysEx field/bit
- [ ] verify all modulation source/destination IDs on hardware (115 source candidates / 79 destination candidates loaded)
- [ ] decode tracking-generator points
- [x] template-preserving candidate program decode -> encode tests
- [ ] byte-perfect hardware program decode -> encode tests
- [ ] bank round-trip tests

## Phase 3 — functional editor

- [x] opt-in live NRPN editing
- [x] incoming candidate NRPN decoding with no-echo ProgramState origin tracking
- [x] candidate request patch UI (hardware verification pending)
- [ ] update edit buffer
- [x] program naming/category support
- [x] load/save single-program `.syx` with source-template preservation
- [x] import/export concatenated source-backed bank `.syx` without assuming bank-stream protocol
- [x] native `.aimprogram.json`
- [x] native `.aimbank.json`
- [x] program librarian
- [x] Mod Matrix editor
- [ ] Tracking Generator touch/mouse editor
- [ ] envelope editors

## Phase 4 — UI parity and improvement

- [ ] full Front panel
- [ ] Dual 1
- [ ] Dual 2
- [ ] Rear
- [ ] Randomizer
- [ ] scalable vector/procedural Ion-style look
- [ ] accessibility
- [ ] high-DPI desktop polish
- [ ] iPad landscape touch optimization
- [ ] keyboard shortcuts and MIDI learn where appropriate

## Phase 5 — release engineering

- [ ] CI for macOS, Windows, Linux
- [ ] signed/notarized macOS builds
- [ ] iPadOS signing/distribution plan
- [ ] Windows installer
- [ ] Linux AppImage/Flatpak packaging evaluation
- [ ] reproducible release archives
- [ ] protocol/spec documentation generated from JSON


## Current implementation checkpoint

- Shared `ProgramState` now keeps duplicate controls synchronized across all five views.
- Candidate patch dumps can be loaded into semantic state without echoing back to hardware.
- Template-preserving candidate patch re-encoding keeps unknown bytes intact.
- A/B patch-diff tooling emits machine-readable JSON for hardware verification.
- Opt-in live NRPN transmission is implemented for mapped `unsigned_14` and `signed_14_wrap` parameters.
- Native librarian now supports 128 sparse slots, editable bank name/hardware-bank metadata, program name/category editing, `.aimprogram.json`, `.aimbank.json`, single-program `.syx`, and concatenated source-backed bank `.syx`.
- Programs imported from hardware retain the complete 378-byte decoded source patch inside native JSON so unknown bytes/bits survive future edits and bank storage.
- The 12-slot Mod Matrix now has a purpose-built responsive editor backed by reusable JSON source/destination enum tables; currently unknown source raw values are preserved rather than coerced.
- Next: compile/test on macOS + iPadOS, then use hardware captures to promote candidate mappings to verified and implement verified edit-buffer writes.
