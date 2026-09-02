# Pass 8 checkpoint — workflow hardening

Pass 8 focuses on editing safety and the first serious local-build workflow.

Implemented:

- bounded semantic `ProgramHistory` with undo/redo;
- 450 ms same-parameter/history coalescing;
- imported/captured state resets history to a clean baseline;
- history navigation uses internal origin and never echoes live NRPN;
- redundant `ProgramState::setValue()` assignments no longer notify or send;
- explicit Undo/Redo controls and desktop shortcuts;
- native librarian Copy Slot / Paste Slot preserving source-patch evidence;
- MIDI device refresh now preserves still-open device selections and closes
  endpoints only when the device has actually disappeared;
- opt-in ASan+UBSan CMake build option on supported desktop Clang/GCC builds;
- `tools/build_and_test.sh` for one-command repository validation, configure,
  compile, and CTest on Linux/macOS;
- `tools/build_ipad_simulator.sh` for an unsigned Xcode iPad Simulator build.

Still required before protocol promotion:

- a real JUCE 9.0.1 compile on Rosie/macOS;
- hardware captures for patch request, NRPN edit, and guarded Edit-buffer send;
- byte-perfect real-hardware program round trips.
