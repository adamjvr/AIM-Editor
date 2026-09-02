#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOG_DIR="${AIM_EDITOR_APPLE_LOG_DIR:-$ROOT/build-logs}"
MAC_LOG="$LOG_DIR/macos.log"
IPAD_LOG="$LOG_DIR/ipados-device.log"
MAC_BUILD_DIR="${AIM_EDITOR_MAC_BUILD_DIR:-$ROOT/build-macos}"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "ERROR: the Apple application gate requires macOS/Xcode and a connected physical iPad." >&2
  exit 2
fi

# Resolve/bootstrap shared dependencies before starting the two compiler jobs. This
# prevents a fresh clone from racing two JUCE downloads or two Python venv creates.
if [[ -z "${AIM_EDITOR_JUCE_PATH:-}" \
      && ! -f "$ROOT/.deps/JUCE/CMakeLists.txt" \
      && ! -f "$HOME/GitHub/JUCE/CMakeLists.txt" ]]; then
  printf '== Bootstrap shared pinned JUCE before concurrent Apple builds ==\n'
  "$ROOT/tools/bootstrap_juce.sh"
fi

SYSTEM_PYTHON="${AIM_EDITOR_PYTHON:-python3}"
if ! command -v "$SYSTEM_PYTHON" >/dev/null 2>&1; then
  echo "ERROR: Python 3.10+ is required for AIM Editor repository validation." >&2
  exit 2
fi
"$SYSTEM_PYTHON" "$ROOT/tools/bootstrap_python_tools.py"

mkdir -p "$LOG_DIR"
: > "$MAC_LOG"
: > "$IPAD_LOG"

printf '=== AIM Editor concurrent Apple hardware gate ===\n'
printf 'macOS log : %s\n' "$MAC_LOG"
printf 'iPadOS log: %s\n\n' "$IPAD_LOG"

run_prefixed() {
  local label="$1"
  local log="$2"
  shift 2

  set +e
  "$@" 2>&1 \
    | while IFS= read -r line || [[ -n "$line" ]]; do printf '[%s] %s\n' "$label" "$line"; done \
    | tee "$log"
  local rc=${PIPESTATUS[0]}
  set -e
  return "$rc"
}

# Independent build trees allow the desktop and device compilers to run at the
# same time. The function itself is backgrounded so wait receives the real child
# script status rather than tee's status.
run_prefixed macOS "$MAC_LOG" \
  env AIM_EDITOR_BUILD_DIR="$MAC_BUILD_DIR" \
  "$ROOT/tools/build_and_test.sh" &
MAC_PID=$!

run_prefixed iPadOS "$IPAD_LOG" \
  "$ROOT/tools/build_ipad_device.sh" &
IPAD_PID=$!

set +e
wait "$MAC_PID"
MAC_RC=$?

if [[ "$MAC_RC" -eq 0 ]]; then
  MAC_APP="$(find "$MAC_BUILD_DIR" -type d -name 'AIM Editor.app' -print -quit)"
  if [[ -n "$MAC_APP" && -d "$MAC_APP" ]]; then
    printf '[macOS] Launching %s\n' "$MAC_APP"
    open "$MAC_APP"
    MAC_LAUNCH_RC=$?
    if [[ "$MAC_LAUNCH_RC" -ne 0 ]]; then
      printf '[macOS] ERROR: built application could not be launched.\n' >&2
      MAC_RC="$MAC_LAUNCH_RC"
    fi
  else
    printf '[macOS] ERROR: AIM Editor.app was not found after a successful macOS build.\n' >&2
    MAC_RC=1
  fi
fi

wait "$IPAD_PID"
IPAD_RC=$?
set -e

printf '\n=== Apple gate result ===\n'
printf 'macOS : %s\n' "$([[ "$MAC_RC" -eq 0 ]] && printf PASS || printf FAIL)"
printf 'iPadOS: %s\n' "$([[ "$IPAD_RC" -eq 0 ]] && printf PASS || printf FAIL)"

if [[ "$MAC_RC" -ne 0 || "$IPAD_RC" -ne 0 ]]; then
  printf 'ERROR: one or more Apple application gates failed.\n' >&2
  printf 'macOS log : %s\n' "$MAC_LOG" >&2
  printf 'iPadOS log: %s\n' "$IPAD_LOG" >&2
  if [[ "$MAC_RC" -ne 0 ]]; then
    printf '\n--- macOS failure tail ---\n' >&2
    tail -80 "$MAC_LOG" >&2 || true
  fi
  if [[ "$IPAD_RC" -ne 0 ]]; then
    printf '\n--- iPadOS failure tail ---\n' >&2
    tail -120 "$IPAD_LOG" >&2 || true
  fi
  exit 1
fi

printf '\nPASS: AIM Editor concurrent macOS + physical iPadOS application gate\n'
