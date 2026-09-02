#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "ERROR: the Apple application gate requires macOS/Xcode." >&2
  exit 2
fi

printf '=== AIM Editor macOS standalone application + core tests ===\n'
AIM_EDITOR_BUILD_DIR="${AIM_EDITOR_MAC_BUILD_DIR:-$ROOT/build-macos}" \
  "$ROOT/tools/build_and_test.sh"

printf '\n=== AIM Editor iPadOS simulator standalone application ===\n'
"$ROOT/tools/build_ipad_simulator.sh"

printf '\nPASS: AIM Editor Apple standalone-application gate\n'
