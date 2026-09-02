# Pass 10 checkpoint — auditable protocol verification

Pass 10 changes the research workflow rather than guessing more protocol data.

## Added

- in-app verification tags in the MIDI/SysEx Inspector;
- separate NRPN and SysEx mapping status in the runtime parameter registry;
- deterministic `aim-editor.protocol-verification` evidence documents;
- SHA-256 sealing and source-file hashes;
- strict NRPN verification from independently reconstructed raw CC streams;
- repeated A/B SysEx field verification;
- mechanical candidate -> verified promotion into canonical JSON;
- validation requiring every verified mapping to cite committed evidence;
- corrected patch-diff confidence that separates the derived checksum field from semantic changes.

## Important invariant

A parameter can now have a verified NRPN address while its SysEx mapping remains candidate. The aggregate parameter status remains `candidate` until all protocol mappings used by that parameter are verified. This prevents one verified transport path from laundering unrelated candidate data into a verified state.

## Still required

No actual Ion mapping has been promoted in this pass because no real hardware capture was supplied. The infrastructure is ready for Rosie/Mac + Ion tests; candidate data remains candidate until those experiments are performed.
