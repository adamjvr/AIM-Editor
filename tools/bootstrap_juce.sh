#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TAG="${AIM_EDITOR_JUCE_TAG:-9.0.1}"
DEST="${AIM_EDITOR_JUCE_DEST:-$ROOT/.deps/JUCE}"

if [[ "$TAG" != "9.0.1" ]]; then
  printf 'ERROR: this bootstrap script has pinned hashes only for JUCE 9.0.1 (requested %s).\n' "$TAG" >&2
  printf 'Set AIM_EDITOR_JUCE_PATH to an already-verified checkout for other versions.\n' >&2
  exit 2
fi

case "$(uname -s)" in
  Linux)
    ASSET="juce-9.0.1-linux.zip"
    SHA256="2389950c45a30cdddc00c8fb35ecc898d97781760fd25aa2589eaf7057ffbf44"
    ;;
  Darwin)
    ASSET="juce-9.0.1-osx.zip"
    SHA256="dda2aefd73796f9b8d37b8f84d7154a7881706bf8aa5a3467334277629001875"
    ;;
  *)
    printf 'ERROR: unsupported platform for bootstrap_juce.sh. Use tools/bootstrap_juce.ps1 on Windows.\n' >&2
    exit 2
    ;;
esac

URL="https://github.com/juce-framework/JUCE/releases/download/${TAG}/${ASSET}"
TMP="$(mktemp -d "${TMPDIR:-/tmp}/aim-juce.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT
ARCHIVE="$TMP/$ASSET"
EXTRACT="$TMP/extract"
mkdir -p "$EXTRACT"

printf '== AIM Editor JUCE bootstrap ==\n'
printf 'Version : %s\n' "$TAG"
printf 'Asset   : %s\n' "$ASSET"
printf 'Target  : %s\n' "$DEST"

if command -v curl >/dev/null 2>&1; then
  curl --fail --location --retry 3 --retry-delay 2 --output "$ARCHIVE" "$URL"
elif command -v wget >/dev/null 2>&1; then
  wget --tries=3 --output-document="$ARCHIVE" "$URL"
else
  printf 'ERROR: curl or wget is required.\n' >&2
  exit 2
fi

if command -v sha256sum >/dev/null 2>&1; then
  ACTUAL="$(sha256sum "$ARCHIVE" | awk '{print $1}')"
elif command -v shasum >/dev/null 2>&1; then
  ACTUAL="$(shasum -a 256 "$ARCHIVE" | awk '{print $1}')"
else
  printf 'ERROR: sha256sum or shasum is required to verify JUCE.\n' >&2
  exit 2
fi

if [[ "$ACTUAL" != "$SHA256" ]]; then
  printf 'ERROR: JUCE archive SHA-256 mismatch.\nExpected: %s\nActual:   %s\n' "$SHA256" "$ACTUAL" >&2
  exit 3
fi
printf 'PASS: JUCE archive SHA-256 verified\n'

if ! command -v unzip >/dev/null 2>&1; then
  printf 'ERROR: unzip is required.\n' >&2
  exit 2
fi
unzip -q "$ARCHIVE" -d "$EXTRACT"

JUCE_MODULE_DIR="$(find "$EXTRACT" -maxdepth 5 -type d -path '*/modules/juce_core' -print -quit)"
if [[ -z "$JUCE_MODULE_DIR" ]]; then
  printf 'ERROR: downloaded archive does not contain modules/juce_core.\n' >&2
  exit 3
fi
JUCE_ROOT="${JUCE_MODULE_DIR%/modules/juce_core}"
if [[ ! -f "$JUCE_ROOT/CMakeLists.txt" ]]; then
  printf 'ERROR: detected JUCE root has no CMakeLists.txt: %s\n' "$JUCE_ROOT" >&2
  exit 3
fi

mkdir -p "$(dirname "$DEST")"
rm -rf "$DEST"
mv "$JUCE_ROOT" "$DEST"

printf 'PASS: JUCE %s installed at %s\n' "$TAG" "$DEST"
printf '\nBuild AIM Editor with:\n'
printf '  AIM_EDITOR_JUCE_PATH=%q ./tools/build_and_test.sh\n' "$DEST"
