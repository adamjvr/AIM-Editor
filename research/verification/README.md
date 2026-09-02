# Hardware verification evidence

This directory is the canonical home for **raw-to-verified protocol evidence** used to promote AIM Editor mappings.

Do not put copyrighted factory patch collections here. A verification record should reference locally captured MIDI/SysEx experiment files by path and SHA-256, and should contain only the observations needed to reproduce the protocol conclusion.

A mapping is promoted only when `tools/protocol/ion_protocol_verification.py apply` accepts a sealed `aim-editor.protocol-verification` record stored in this directory. The canonical NRPN/SysEx JSON then records the evidence file path, making every future `verified` status auditable.

Recommended names:

```text
filter1-frequency-nrpn-001.json
filter1-frequency-sysex-001.json
osc1-octave-nrpn-001.json
```

Raw captures may be stored outside the public repository when they contain complete user patches. Their SHA-256 remains in the evidence record so a local archive can still be proven to be the exact source used for verification.

Summarize current canonical coverage and all committed evidence without changing
any mapping:

```bash
./tools/protocol/ion_verification_report.py report
./tools/protocol/ion_verification_report.py report --format json --output /tmp/aim-verification.json
```

The report flags invalid seals and contradictions for review. It is deliberately
read-only; only `ion_protocol_verification.py apply` can promote a mapping.
