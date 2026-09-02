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
- [x] SHA-256-pinned JUCE bootstrap, build doctor, and Linux/macOS/Windows build/test helpers
- [ ] select project license

## Phase 1 — protocol archaeology

- [ ] unpack original executable completely
- [ ] recover/parse serialized SynthMaker/FlowStone project payload
- [x] find archived Ion MIDI/SysEx documentation and preserve provenance (candidate/community sources; hardware verification pending)
- [x] identify candidate manufacturer/model SysEx framing
- [x] map candidate single-patch request message
- [x] implement guarded candidate edit-buffer/full-patch destination retargeting (hardware verification pending)
- [x] map candidate single-program dump framing
- [ ] map/verify bank-dump streaming behavior
- [x] implement candidate 7-of-8 + checksum rules
- [x] import 234 candidate NRPN IDs
- [ ] verify NRPN IDs and transport semantics on hardware (promotion pipeline is implemented; real Ion evidence still required)
- [x] add confidence + evidence records to JSON
- [x] build raw MIDI/SysEx inspector and JSON capture logger
- [x] reconstruct candidate NRPN transactions inside capture JSON for repeatable hardware verification
- [x] add sealed, auditable NRPN/SysEx evidence records and mechanical candidate -> verified promotion
- [x] generate read-only Markdown/JSON verification coverage reports from canonical mappings + sealed evidence
- [x] generate deterministic read-only NRPN/SysEx hardware-verification queues from canonical mappings

## Phase 2 — verified data model

- [ ] complete parameter value domains
- [ ] complete enum tables (3 reusable tables + inline domains are loaded; incomplete candidate domains remain explicit)
- [ ] complete display transforms (85/212 currently executable from JSON; unknowns remain raw)
- [ ] map every SysEx field/bit
- [ ] verify all modulation source/destination IDs on hardware (115 source candidates / 79 destination candidates loaded)
- [x] decode candidate tracking-generator points into first-class JSON parameters (hardware verification pending)
- [x] template-preserving candidate program decode -> encode tests
- [ ] byte-perfect hardware program decode -> encode tests
- [ ] bank round-trip tests

## Phase 3 — functional editor

- [x] opt-in live NRPN editing
- [x] incoming candidate NRPN decoding with no-echo ProgramState origin tracking
- [x] candidate request patch UI (hardware verification pending)
- [x] guarded candidate update-to-Edit workflow requiring source template + explicit arming (hardware verification pending)
- [x] program naming/category support
- [x] load/save single-program `.syx` with source-template preservation
- [x] import/export concatenated source-backed bank `.syx` without assuming bank-stream protocol
- [x] native `.aimprogram.json`
- [x] native `.aimbank.json`
- [x] program librarian
- [x] Mod Matrix editor
- [x] Tracking Generator touch/mouse editor
- [x] envelope editors (touch/mouse ADSR surface backed by shared ProgramState)

## Phase 4 — UI parity and improvement

- [x] structured Front panel composition (visual tuning ongoing)
- [x] structured Dual 1 composition (visual tuning ongoing)
- [x] structured Dual 2 composition (visual tuning ongoing)
- [x] structured Rear composition (visual tuning ongoing)
- [x] Randomizer
- [x] scalable vector/procedural Ion-style control foundation
- [ ] accessibility
- [x] high-DPI procedural control/navigation foundation (device-by-device polish ongoing)
- [x] iPad landscape-first responsive composition and touch envelope/tracking surfaces (device tuning ongoing)
- [x] core keyboard shortcuts + semantic undo/redo (MIDI learn remains optional/future)
- [x] unified guarded Open/Save All + desktop document shortcuts and single-instance command-line/second-launch forwarding; `.syx` association metadata declared
- [x] safe session restore for page/MIDI context; live editing and hardware-write arming intentionally never persist
- [x] semantic unsaved-program/bank tracking and save/discard/cancel destructive-action/quit resolution
- [x] explicit semantic New Program workflow with no invented hardware template
- [x] guided controlled-NRPN capture mode with freeze + offline raw-event cross-check

## Phase 5 — release engineering

- [ ] CI for macOS, Windows, Linux (workflow exists; hosted jobs currently fail before steps start)
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
- A/B patch-diff tooling emits machine-readable JSON for hardware verification and now separates derived checksum changes from semantic field changes.
- Opt-in live NRPN transmission is implemented for mapped `unsigned_14` and `signed_14_wrap` parameters.
- Native librarian now supports 128 sparse slots, editable bank name/hardware-bank metadata, program name/category editing, `.aimprogram.json`, `.aimbank.json`, single-program `.syx`, and concatenated source-backed bank `.syx`.
- Programs imported from hardware retain the complete 378-byte decoded source patch inside native JSON so unknown bytes/bits survive future edits and bank storage.
- The 12-slot Mod Matrix now has a purpose-built responsive editor backed by reusable JSON source/destination enum tables; currently unknown source raw values are preserved rather than coerced.
- The Randomizer is now semantic/data-driven, deterministic when seeded, scoped by synth section and parameter kind, and intentionally avoids automatic bulk MIDI writes.
- The Tracking Generator now exposes all 33 candidate curve bytes (-16..+16) as canonical JSON parameters with candidate NRPN 121..153 and SysEx offsets 304..336, plus a touch/mouse graph editor.
- Purpose-built oscillator, filter, envelope, LFO/S&H/tempo, voice, effects, pre/post mixer, and output panels now replace the main generic control buckets, with large-landscape Front/Dual/Rear composition and conservative JSON-driven engineering-value formatting.
- Large landscape surfaces now use segmented five-page navigation; smaller layouts retain the compact selector.
- A small manual-derived label set improves LFO/S&H reset, portamento, and pitch-wheel selectors while protocol raw mappings remain candidate.
- Guarded hardware-transfer tools now expose patch/bank requests and source-template-preserving full sends to Edit 1–4. Full writes require explicit arming and automatically disarm after each transmission.
- Semantic undo/redo is now shared across all editor views, bounded/coalesced, and deliberately non-echoing to MIDI; imported/captured hardware state becomes a fresh history baseline.
- Librarian slot copy/paste preserves the complete `IonProgram`, including source-patch bytes, and MIDI refresh now stays synchronized with the actually-open JUCE endpoints.
- A one-command local build/test script and optional desktop ASan+UBSan configuration are available for Rosie/macOS build hardening.
- The SysEx inspector now surfaces the latest checksum-valid candidate patch metadata before loading it into semantic state.
- Session restore now remembers non-destructive editor/MIDI context while explicitly forcing Live NRPN and full-write arming off on every launch.
- MIDI capture JSON now includes statefully reconstructed candidate NRPN transactions with semantic IDs/status while preserving raw events as the authoritative evidence.
- Controlled captures can be tagged in-app with a semantic parameter ID and isolation confirmation; Start/Stop Test now freezes a bounded experiment and reports expected/distinct/competing NRPN observations before offline sealing.
- Program and bank dirty state are independent, undo-aware semantic baselines; destructive loads and quit now support Save & Continue/Save & Quit as well as explicit discard/cancel paths.
- A fresh semantic Init can be created explicitly without fabricating any source-patch bytes, and verification coverage can be summarized from the canonical JSON/evidence tree with `ion_verification_report.py`.
- JUCE 9.0.1 can now be bootstrapped from official platform archives with pinned SHA-256 verification; the build doctor checks C/C++ toolchains and detectable platform prerequisites before configuration.
- Program/bank/SysEx opening now converges on one guarded dispatcher with Save All and desktop document shortcuts; command-line/second-instance launches use the same path and CMake declares `.syx` association metadata.
- Hardware testing can be pulled from a deterministic priority queue with `ion_verification_plan.py`; the planner remains read-only and has no promotion authority.
- Next: compile/test on Rosie and macOS/iPadOS, capture real Ion request/write behavior, promote verified protocol fields, and tune the purpose-built geometry from device screenshots.
