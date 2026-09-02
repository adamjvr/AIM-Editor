# Pass 15 checkpoint — Ubuntu 22.04 build-gate correction

Rosie reached the real Pass 14 preflight with the pinned JUCE 9.0.1 tree installed successfully. Two blockers were reported before CMake configuration:

1. The project build doctor required CMake 3.24+, while JUCE 9.0.1 itself requires CMake 3.22+. Ubuntu 22.04 ships a suitable CMake 3.22.x, so the AIM Editor requirement was unnecessarily strict. Pass 15 aligns both the top-level CMake project and build doctor to 3.22.
2. The `jack` pkg-config development module was missing. On Debian/Ubuntu this is supplied by `libjack-jackd2-dev`; the build doctor continues to print the complete dependency normalization command.

This checkpoint does **not** claim a successful application compile yet. The next Rosie run should install the missing development package(s), pass the strict preflight, and proceed into the first actual JUCE 9.0.1 configure/compile diagnostics.

The repository validator now requires the CMake project and build doctor to agree on the 3.22 minimum so this gate cannot drift again.
