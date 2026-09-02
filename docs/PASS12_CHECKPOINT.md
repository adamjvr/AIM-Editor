# Pass 12 checkpoint — save-aware workflows and verification reporting

## Added

- Save-before-continue handling for program, bank, and combined destructive operations.
- Save-before-quit handling for the application window and OS quit path.
- Cancelled/failed saves abort the destructive action instead of silently continuing.
- Explicit **New Program** action using registry defaults without fabricating a SysEx template.
- Read-only protocol verification coverage report in Markdown or JSON.
- Repository self-test now validates verification-report coverage and evidence seals.
- Document-safety validator now requires the save-aware paths.

## Safety invariants

1. Native JSON is the automatic save target; candidate `.syx` is never silently emitted as a document save.
2. Save cancellation is treated as cancellation of the destructive action.
3. New semantic programs contain no invented source-patch bytes.
4. Verification reporting cannot mutate or promote protocol data.
5. Candidate mappings stay candidate until sealed hardware evidence is applied.

## Next

- Real JUCE 9.0.1 Linux/macOS compile and fix pass on a machine with the full framework tree.
- iPad Simulator/device compile and geometry screenshots.
- Real Ion controlled NRPN/SysEx captures and evidence promotion.
- Bank-stream behavior research/verification.
