# Pass 16 checkpoint — pinned Python validation environment

## Why this pass exists

The first real Rosie build preflight reached repository validation and exposed an
environment mismatch rather than a C++ failure: Pop!_OS 22.04's distro
`jsonschema` package did not export `Draft202012Validator`, while AIM Editor's
native interchange schemas are explicitly Draft 2020-12.

The project must not require developers to replace or `pip install` over their
system Python packages. Pass 16 therefore makes the validation runtime as
reproducible as the JUCE runtime.

## New build boundary

`tools/bootstrap_python_tools.py` owns `.deps/python-tools` and installs the
fully pinned packages listed in `tools/requirements-tools.txt`. The bootstrap
self-tests the exact capabilities needed by `validate_native_formats.py`:

- `jsonschema.Draft202012Validator`;
- `referencing.Registry`;
- `referencing.Resource`.

A SHA-256 digest of the requirements file is stored inside the ignored local
environment. Changing the requirements automatically forces a rebuild of that
environment on the next build.

Both `build_and_test.sh` and `build_and_test.ps1` bootstrap this environment
before the build doctor, then use its Python interpreter for the doctor and the
entire repository check suite. CI data validation uses the same path.

## Host safety

The bootstrap never installs packages into the system interpreter. If Python's
`venv` support is missing on Debian/Ubuntu it stops and reports:

```text
sudo apt-get install python3-venv
```

This is an environment prerequisite, not a source-code failure.

## Real-build status

Rosie has now passed:

- JUCE 9.0.1 download + SHA-256 verification;
- CMake 3.22.1 prerequisite;
- C/C++ compiler prerequisite;
- Ninja prerequisite;
- Linux JUCE pkg-config dependency prerequisite after JACK headers were added;
- parameter/editor/protocol validators up to the point where the old system
  `jsonschema` import stopped the run.

No AIM Editor C++ compiler diagnostic has been observed yet. The next real run
must continue from the pinned Python repository checks into CMake configure and
compilation.
