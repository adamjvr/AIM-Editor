# Pass 11 checkpoint

Pass 11 focuses on destructive-action safety and making hardware verification usable from inside AIM Editor.

## Added

- semantic program dirty-state tracking against a clean baseline;
- bank dirty-state tracking through complete `ProgramBank` equality;
- dirty indicators in the librarian;
- discard confirmation before replacing edited programs/banks;
- quit confirmation when either current program or librarian bank is unsaved;
- native program/bank JSON saves establish clean baselines only after successful writes;
- controlled NRPN **Start Test / Stop Test** workflow in the MIDI/SysEx Inspector;
- capture freeze after a controlled test so later MIDI cannot contaminate evidence;
- live structural assessment of expected NRPN transactions, distinct values, and competing NRPNs;
- experiment timestamps/assessment metadata in capture JSON;
- offline cross-check of the in-app assessment against freshly reconstructed raw MIDI;
- repository-level document/verification safety validator;
- standalone compile smoke test for `ProgramDocumentTracker`.

## Still deliberately not claimed

- no candidate protocol mapping is promoted without real Ion evidence;
- no full JUCE compile is claimed from the sandbox; the actual target build still belongs on Rosie/macOS/iPadOS;
- a clean-looking controlled capture is not itself "verified" until the offline sealed-evidence checks pass.
