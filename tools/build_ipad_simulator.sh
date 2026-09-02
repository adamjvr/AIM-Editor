#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${AIM_EDITOR_IOS_BUILD_DIR:-$ROOT/build-ios}"
CONFIG="${AIM_EDITOR_BUILD_TYPE:-Debug}"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "ERROR: iPadOS simulator builds require macOS/Xcode." >&2
  exit 2
fi

cmake_args=(
  -S "$ROOT"
  -B "$BUILD_DIR"
  -G Xcode
  -DCMAKE_SYSTEM_NAME=iOS
  -DCMAKE_OSX_SYSROOT=iphonesimulator
  -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0
  -DAIM_EDITOR_BUILD_TESTS=OFF
)

if [[ -n "${AIM_EDITOR_JUCE_PATH:-}" ]]; then
  cmake_args+=("-DAIM_EDITOR_JUCE_PATH=$AIM_EDITOR_JUCE_PATH")
elif [[ -d "$HOME/GitHub/JUCE" ]]; then
  cmake_args+=("-DAIM_EDITOR_JUCE_PATH=$HOME/GitHub/JUCE")
fi

printf '== Configure iPadOS simulator ==\n'
cmake "${cmake_args[@]}"

printf '\n== Build AIMEditor (%s) ==\n' "$CONFIG"
cmake --build "$BUILD_DIR" --config "$CONFIG" --target AIMEditor -- \
  -sdk iphonesimulator CODE_SIGNING_ALLOWED=NO

printf '\nPASS: AIM Editor iPadOS simulator build\n'
