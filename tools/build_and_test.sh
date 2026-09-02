#!/usr/bin/env bash
set -euo pipefail

# Validation tools must never litter the source tree with interpreter bytecode.
export PYTHONDONTWRITEBYTECODE=1

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

SYSTEM_PYTHON="${AIM_EDITOR_PYTHON:-python3}"
if ! command -v "$SYSTEM_PYTHON" >/dev/null 2>&1; then
  printf 'ERROR: Python 3.10+ is required for AIM Editor repository validation.\n' >&2
  exit 2
fi

"$SYSTEM_PYTHON" "$ROOT/tools/bootstrap_python_tools.py"
TOOLS_PYTHON="$ROOT/.deps/python-tools/bin/python"
if [[ ! -x "$TOOLS_PYTHON" ]]; then
  printf 'ERROR: project-local Python validation interpreter was not created.\n' >&2
  exit 2
fi

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

printf '== Clean generated Python artifacts ==\n'
"$TOOLS_PYTHON" "$ROOT/tools/clean_python_artifacts.py"

printf '== AIM Editor build doctor ==\n'
"$TOOLS_PYTHON" "$ROOT/tools/build_doctor.py" --require-local-juce --strict-platform

printf '\n== AIM Editor repository checks ==\n'
"$TOOLS_PYTHON" "$ROOT/tools/check_repository.py"

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
