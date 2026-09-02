# Undo, redo, and editor workflow

AIM Editor keeps undo/redo at the **semantic program** layer rather than at the
individual GUI-widget layer. This keeps Front/Dual/Rear duplicate controls in
one history and ensures that a history operation never needs to know raw NRPN
or SysEx details.

## Safety rules

- Only deliberate `interactive` editor changes enter undo history.
- Imported JSON/SysEx and captured hardware patches become a fresh baseline.
- Incoming hardware NRPN changes also become a fresh baseline rather than
  pretending an editor undo has changed the physical synth.
- Undo/redo restores state with `ProgramChangeOrigin::internal`, so the live
  NRPN transmitter **does not echo history navigation back to hardware**.
- Rapid repeated changes to the same parameter are coalesced for 450 ms. This
  makes a knob/slider gesture behave like one practical undo step.
- History is bounded to 256 semantic snapshots by default.

## Controls and shortcuts

The bottom strip exposes explicit **Undo** and **Redo** buttons.

Desktop keyboard shortcuts:

- `Cmd/Ctrl + Z` — undo
- `Cmd/Ctrl + Shift + Z` — redo
- `Cmd/Ctrl + Y` — redo
- `Cmd/Ctrl + 1..5` — Front, Dual 1, Dual 2, Randomizer, Rear
- `Cmd/Ctrl + L` — program librarian
- `Cmd/Ctrl + I` — MIDI/SysEx inspector
- `Esc` — close an open inspector/librarian/hardware overlay

Text editors may consume their own platform-standard editing shortcuts first;
unhandled key events then bubble to the editor surface.

## Librarian slot clipboard

The native librarian now includes **Copy Slot** and **Paste Slot**. The copied
`IonProgram` includes semantic parameters, unknown-byte evidence, and the full
source patch template when present. Pasting therefore cannot accidentally
strip the raw evidence required for safe template-preserving SysEx export.
