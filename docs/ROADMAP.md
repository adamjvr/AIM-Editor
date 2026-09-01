# AIM Editor roadmap

## Phase 0 — foundation (current)

- [x] JUCE/CMake project skeleton
- [x] macOS/iPadOS/Windows/Linux target strategy
- [x] JSON-first parameter database
- [x] JSON Schema for parameter database and programs
- [x] typed parameter registry
- [x] semantic program model
- [x] nested human-readable program JSON export/import
- [x] preservation container for unknown raw bytes
- [x] MIDI device service
- [x] generic NRPN message builder
- [x] five-page responsive GUI shell
- [x] bottom MIDI/program control strip
- [ ] select project license

## Phase 1 — protocol archaeology

- [ ] unpack original executable completely
- [ ] recover/parse serialized SynthMaker/FlowStone project payload
- [ ] find archived Ion MIDI/SysEx documentation and preserve provenance
- [ ] identify manufacturer/model SysEx framing
- [ ] map patch request and edit-buffer update messages
- [ ] map program/bank dumps
- [ ] derive checksum/packing rules
- [ ] map NRPN IDs
- [ ] add confidence + evidence records to JSON
- [ ] build raw SysEx inspector and capture logger

## Phase 2 — verified data model

- [ ] complete parameter value domains
- [ ] complete enum tables
- [ ] complete display transforms (Hz, ms, %, semitones, etc.)
- [ ] map every SysEx field/bit
- [ ] map all modulation source/destination IDs
- [ ] decode tracking-generator points
- [ ] byte-perfect program decode -> encode tests
- [ ] bank round-trip tests

## Phase 3 — functional editor

- [ ] live NRPN editing
- [ ] request patch
- [ ] update edit buffer
- [ ] program naming/category support
- [ ] load/save `.syx`
- [ ] native `.aimprogram.json`
- [ ] native `.aimbank.json`
- [ ] program librarian
- [ ] Mod Matrix editor
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
