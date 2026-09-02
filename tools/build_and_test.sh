#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${AIM_EDITOR_BUILD_DIR:-$ROOT/build-local}"
BUILD_TYPE="${AIM_EDITOR_BUILD_TYPE:-Debug}"
BOOTSTRAP_JUCE="${AIM_EDITOR_BOOTSTRAP_JUCE:-1}"

resolve_local_juce() {
  if [[ -n "${AIM_EDITOR_JUCE_PATH:-}" && -f "${AIM_EDITOR_JUCE_PATH}/CMakeLists.txt" ]]; then
    printf '%s\n' "$AIM_EDITOR_JUCE_PATH"
    return 0
  fi
  if [[ -f "$ROOT/.deps/JUCE/CMakeLists.txt" ]]; then
    printf '%s\n' "$ROOT/.deps/JUCE"
    return 0
  fi
  if [[ -f "$HOME/GitHub/JUCE/CMakeLists.txt" ]]; then
    printf '%s\n' "$HOME/GitHub/JUCE"
    return 0
  fi
  return 1
}

LOCAL_JUCE="$(resolve_local_juce || true)"
if [[ -z "$LOCAL_JUCE" && "$BOOTSTRAP_JUCE" == "1" ]]; then
  printf '== Bootstrap pinned JUCE ==\n'
  "$ROOT/tools/bootstrap_juce.sh"
  LOCAL_JUCE="$(resolve_local_juce || true)"
fi

if [[ -z "$LOCAL_JUCE" ]]; then
  printf 'ERROR: no local JUCE %s tree is available.\n' "9.0.1" >&2
  printf 'Run ./tools/bootstrap_juce.sh, set AIM_EDITOR_JUCE_PATH, or set AIM_EDITOR_BOOTSTRAP_JUCE=1.\n' >&2
  exit 2
fi

export AIM_EDITOR_JUCE_PATH="$LOCAL_JUCE"

cmake_args=(
  -S "$ROOT"
  -B "$BUILD_DIR"
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  -DAIM_EDITOR_BUILD_TESTS=ON
  "-DAIM_EDITOR_JUCE_PATH=$LOCAL_JUCE"
)

if [[ "${AIM_EDITOR_SANITIZE:-0}" == "1" ]]; then
  cmake_args+=("-DAIM_EDITOR_ENABLE_SANITIZERS=ON")
fi

if command -v ninja >/dev/null 2>&1; then
  cmake_args+=(-G Ninja)
fi

printf '== AIM Editor build doctor ==\n'
python3 "$ROOT/tools/build_doctor.py" --require-local-juce --strict-platform

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
