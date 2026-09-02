#!/usr/bin/env bash
set -euo pipefail

# Keep repository validation deterministic and cache-free.
export PYTHONDONTWRITEBYTECODE=1

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${AIM_EDITOR_IOS_BUILD_DIR:-$ROOT/build-ios}"
CONFIG="${AIM_EDITOR_BUILD_TYPE:-Debug}"
BOOTSTRAP_JUCE="${AIM_EDITOR_BOOTSTRAP_JUCE:-1}"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "ERROR: iPadOS simulator builds require macOS/Xcode." >&2
  exit 2
fi

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
  echo "ERROR: no local JUCE 9.0.1 tree is available." >&2
  echo "Run ./tools/bootstrap_juce.sh, set AIM_EDITOR_JUCE_PATH, or leave AIM_EDITOR_BOOTSTRAP_JUCE=1." >&2
  exit 2
fi
export AIM_EDITOR_JUCE_PATH="$LOCAL_JUCE"

SYSTEM_PYTHON="${AIM_EDITOR_PYTHON:-python3}"
if ! command -v "$SYSTEM_PYTHON" >/dev/null 2>&1; then
  echo "ERROR: Python 3.10+ is required for AIM Editor repository validation." >&2
  exit 2
fi

"$SYSTEM_PYTHON" "$ROOT/tools/bootstrap_python_tools.py"
TOOLS_PYTHON="$ROOT/.deps/python-tools/bin/python"
if [[ ! -x "$TOOLS_PYTHON" ]]; then
  echo "ERROR: project-local Python validation interpreter was not created." >&2
  exit 2
fi

printf '== Clean generated Python artifacts ==\n'
"$TOOLS_PYTHON" "$ROOT/tools/clean_python_artifacts.py"

printf '\n== AIM Editor Apple/iPadOS build doctor ==\n'
"$TOOLS_PYTHON" "$ROOT/tools/build_doctor.py" \
  --require-local-juce \
  --strict-platform \
  --require-ios-simulator-sdk

printf '\n== AIM Editor repository checks ==\n'
"$TOOLS_PYTHON" "$ROOT/tools/check_repository.py"

cmake_args=(
  -S "$ROOT"
  -B "$BUILD_DIR"
  -G Xcode
  -DCMAKE_SYSTEM_NAME=iOS
  -DCMAKE_OSX_SYSROOT=iphonesimulator
  -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0
  -DAIM_EDITOR_BUILD_TESTS=OFF
  "-DAIM_EDITOR_JUCE_PATH=$LOCAL_JUCE"
)

printf '\n== Configure iPadOS simulator ==\n'
printf '+ cmake'
printf ' %q' "${cmake_args[@]}"
printf '\n'
cmake "${cmake_args[@]}"

printf '\n== Build AIMEditor (%s) ==\n' "$CONFIG"
cmake --build "$BUILD_DIR" --config "$CONFIG" --target AIMEditor -- \
  -sdk iphonesimulator CODE_SIGNING_ALLOWED=NO

printf '\nPASS: AIM Editor iPadOS simulator application build\n'
