# Pass 18 checkpoint — first real JUCE compiler wave

Pass 18 is the first checkpoint driven by an actual JUCE 9.0.1 / GCC build on Rosie rather than CMake graph validation.

## What Rosie proved

The strict prerequisite gate, repository checks, real JUCE 9.0.1 CMake configure, `juceaide` setup, and Ninja generation all completed successfully. Compilation then reached AIM Editor source and exposed the first real source/API errors.

## Compiler fixes in this pass

- `juce::StringArray`: replace nonexistent `getLast()` usage with indexed access.
- JUCE BinaryData: use the generated symbol spelling for hyphenated filenames (`modulationsources_json`, `modulationdestinations_json`, `filtertypes_json`).
- JUCE component registration: give heterogeneous pointer initializer lists the explicit `std::initializer_list<juce::Component*>` element type.
- Oscillator paint geometry: make the local lane rectangle mutable before calling `removeFromLeft()`.
- `juce::File`: replace ambiguous brace assignment with explicit `juce::File{}` resets.
- File chooser locals: rename `flags` to `chooserFlags` to avoid JUCE `Component::flags` shadow warnings.

`tools/validate_juce_api_contracts.py` now statically guards these exact real-build contracts so they cannot silently return during later UI work.

## Build status

Pass 18 fixes every error reported by the first Ninja compiler wave. It must still be rebuilt on Rosie because later translation units may expose a second wave once these failures are cleared. Do not describe the application as fully compiled until Ninja and CTest both finish successfully.
