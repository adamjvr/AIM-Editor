#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${AIM_EDITOR_BUILD_DIR:-$ROOT/build-local}"
BUILD_TYPE="${AIM_EDITOR_BUILD_TYPE:-Debug}"

cmake_args=(
  -S "$ROOT"
  -B "$BUILD_DIR"
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  -DAIM_EDITOR_BUILD_TESTS=ON
)

if [[ -n "${AIM_EDITOR_JUCE_PATH:-}" ]]; then
  cmake_args+=("-DAIM_EDITOR_JUCE_PATH=$AIM_EDITOR_JUCE_PATH")
elif [[ -d "$ROOT/.deps/JUCE" ]]; then
  cmake_args+=("-DAIM_EDITOR_JUCE_PATH=$ROOT/.deps/JUCE")
elif [[ -d "$HOME/GitHub/JUCE" ]]; then
  cmake_args+=("-DAIM_EDITOR_JUCE_PATH=$HOME/GitHub/JUCE")
fi

if [[ "${AIM_EDITOR_SANITIZE:-0}" == "1" ]]; then
  cmake_args+=("-DAIM_EDITOR_ENABLE_SANITIZERS=ON")
fi

if command -v ninja >/dev/null 2>&1; then
  cmake_args+=(-G Ninja)
fi

printf '== AIM Editor build doctor ==\n'
python3 "$ROOT/tools/build_doctor.py"

printf '\n== AIM Editor repository checks ==\n'
python3 "$ROOT/tools/check_repository.py"

printf '\n== Configure ==\n'
printf '+ cmake'
printf ' %q' "${cmake_args[@]}"
printf '\n'
cmake "${cmake_args[@]}"

printf '\n== Build ==\n'
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel

printf '\n== Tests ==\n'
ctest --test-dir "$BUILD_DIR" -C "$BUILD_TYPE" --output-on-failure

printf '\nPASS: AIM Editor local build/test pipeline\n'
