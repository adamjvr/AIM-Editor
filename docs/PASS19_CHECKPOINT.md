# Pass 19 checkpoint — real-build semantic test fix

Pass 19 follows the first successful full JUCE 9.0.1 Linux compile/link on Rosie.
The executable linked successfully; CTest then exposed a ProgramState enum-domain
normalization bug.

## Fix

For complete enum tables, interactive values are now validated against the exact
enum membership before any numeric min/max clamping. Invalid raw values therefore
fall back deterministically instead of being silently transformed into a different
valid enum member. Incomplete enum tables retain the previous evidence-preserving
behavior: unknown in-range raw values are allowed and only numeric bounds are
applied.

Fallback resolution for complete enums is non-recursive: a declared `default_raw`
is used only when it is a known member; otherwise the first declared enum member
is the stable fallback.

Core tests now cover high and low invalid complete-enum values, a valid endpoint,
and the existing incomplete-enum preservation contract.
