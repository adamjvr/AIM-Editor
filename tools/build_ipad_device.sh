#!/usr/bin/env bash
set -euo pipefail

# Physical-hardware-only iPadOS gate. No simulator path is provided.
export PYTHONDONTWRITEBYTECODE=1

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${AIM_EDITOR_IOS_DEVICE_BUILD_DIR:-$ROOT/build-ios-device}"
CONFIG="${AIM_EDITOR_BUILD_TYPE:-Debug}"
BOOTSTRAP_JUCE="${AIM_EDITOR_BOOTSTRAP_JUCE:-1}"
BUNDLE_ID="com.rothamplification.aimeditor"
LOCAL_ENV="$ROOT/.aim-editor-local.env"

if [[ -f "$LOCAL_ENV" ]]; then
  # shellcheck disable=SC1090
  source "$LOCAL_ENV"
fi

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "ERROR: physical iPadOS builds require macOS/Xcode and a connected iPad." >&2
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

printf '\n== AIM Editor Apple/iPadOS device build doctor ==\n'
"$TOOLS_PYTHON" "$ROOT/tools/build_doctor.py" \
  --require-local-juce \
  --strict-platform \
  --require-ios-device-sdk

printf '\n== Select connected physical iPad ==\n'
DEVICE_SELECTOR="${AIM_EDITOR_IPAD_DEVICE:-}"
if DEVICE_ENV="$("$TOOLS_PYTHON" "$ROOT/tools/select_ipad_device.py" --device "$DEVICE_SELECTOR" --shell)"; then
  eval "$DEVICE_ENV"
else
  exit $?
fi
printf 'Device : %s\n' "$AIM_IPAD_NAME"
printf 'UDID   : %s\n' "$AIM_IPAD_UDID"
printf 'Model  : %s\n' "$AIM_IPAD_PRODUCT_TYPE"
printf 'iPadOS : %s\n' "$AIM_IPAD_OS_VERSION"
printf 'Link   : %s\n' "${AIM_IPAD_TRANSPORT:-unknown}"

DEVELOPMENT_TEAM="${AIM_EDITOR_DEVELOPMENT_TEAM:-}"
if [[ -z "$DEVELOPMENT_TEAM" ]]; then
  echo "ERROR: AIM_EDITOR_DEVELOPMENT_TEAM is not configured." >&2
  echo "Set it in $LOCAL_ENV (preferred) or export it in the shell." >&2
  echo "AIM Editor deliberately does not guess Team IDs from certificate display names." >&2
  exit 2
fi
printf 'Team   : %s\n' "$DEVELOPMENT_TEAM"

printf '\n== AIM Editor repository checks ==\n'
"$TOOLS_PYTHON" "$ROOT/tools/check_repository.py"

# JUCE's host-side helper tool caches inside the cross-build tree. Keep physical
# iPad verification deterministic and free from any prior simulator/device cache.
printf '\n== Reset physical iPadOS build tree ==\n'
cmake -E remove_directory "$BUILD_DIR"

cmake_args=(
  -S "$ROOT"
  -B "$BUILD_DIR"
  -G Xcode
  -DCMAKE_SYSTEM_NAME=iOS
  -DCMAKE_OSX_SYSROOT=iphoneos
  -DCMAKE_OSX_ARCHITECTURES=arm64
  -DCMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET=17.0
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE=Automatic
  "-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=$DEVELOPMENT_TEAM"
  -DAIM_EDITOR_BUILD_TESTS=OFF
  "-DAIM_EDITOR_JUCE_PATH=$LOCAL_JUCE"
)

printf '\n== Configure physical iPadOS target ==\n'
printf '+ cmake'
printf ' %q' "${cmake_args[@]}"
printf '\n'
cmake "${cmake_args[@]}"

printf '\n== Verify generated Xcode Objective-C ARC boundary ==\n'
ARC_SETTING="$(
  xcodebuild \
    -project "$BUILD_DIR/AIMEditor.xcodeproj" \
    -target AIMEditor \
    -configuration "$CONFIG" \
    -showBuildSettings 2>/dev/null \
    | awk -F ' = ' '/^[[:space:]]*CLANG_ENABLE_OBJC_ARC = / { print $2; exit }'
)"
if [[ "$ARC_SETTING" != "NO" ]]; then
  printf 'ERROR: generated AIMEditor Xcode target reports CLANG_ENABLE_OBJC_ARC=%s; JUCE iOS module sources require non-ARC.\n' "${ARC_SETTING:-<unset>}" >&2
  printf 'Inspect: %s\n' "$BUILD_DIR/AIMEditor.xcodeproj/project.pbxproj" >&2
  exit 2
fi
printf 'Objective-C ARC : %s (expected NO for JUCE module sources)\n' "$ARC_SETTING"

printf '\n== Build signed AIMEditor for %s ==\n' "$AIM_IPAD_NAME"
BUILD_LOG="$BUILD_DIR/aimeditor-ipad-build.log"
set +e
cmake --build "$BUILD_DIR" --config "$CONFIG" --target AIMEditor -- \
  -destination "platform=iOS,id=$AIM_IPAD_UDID" \
  -allowProvisioningUpdates \
  -allowProvisioningDeviceRegistration \
  "DEVELOPMENT_TEAM=$DEVELOPMENT_TEAM" \
  CODE_SIGN_STYLE=Automatic 2>&1 | tee "$BUILD_LOG"
BUILD_RC=${PIPESTATUS[0]}
set -e
if [[ "$BUILD_RC" -ne 0 ]]; then
  printf '\n== Compiler diagnostics extracted from failed iPad build ==\n' >&2
  grep -nE '(^|[[:space:]])(fatal )?error:|CompileC .*juce_|\*\* BUILD FAILED \*\*' "$BUILD_LOG" | tail -120 >&2 || true
  printf '\nFull Xcode build log: %s\n' "$BUILD_LOG" >&2
  exit "$BUILD_RC"
fi

printf '\n== Locate signed physical-device application product ==\n'
BUILD_SETTINGS="$(
  xcodebuild \
    -project "$BUILD_DIR/AIMEditor.xcodeproj" \
    -target AIMEditor \
    -configuration "$CONFIG" \
    -showBuildSettings 2>/dev/null
)"
TARGET_BUILD_DIR="$(printf '%s\n' "$BUILD_SETTINGS" | awk -F ' = ' '/^[[:space:]]*TARGET_BUILD_DIR = / { print $2; exit }')"
FULL_PRODUCT_NAME="$(printf '%s\n' "$BUILD_SETTINGS" | awk -F ' = ' '/^[[:space:]]*FULL_PRODUCT_NAME = / { print $2; exit }')"
APP_BUNDLE="${TARGET_BUILD_DIR%/}/${FULL_PRODUCT_NAME}"

if [[ -z "$TARGET_BUILD_DIR" || -z "$FULL_PRODUCT_NAME" || ! -d "$APP_BUNDLE" ]]; then
  echo "ERROR: signed physical-device AIM Editor.app was not found after a successful build." >&2
  printf 'TARGET_BUILD_DIR=%s\n' "${TARGET_BUILD_DIR:-<unset>}" >&2
  printf 'FULL_PRODUCT_NAME=%s\n' "${FULL_PRODUCT_NAME:-<unset>}" >&2
  printf 'Candidate application products under %s:\n' "$BUILD_DIR" >&2
  find "$BUILD_DIR" -type d -name '*.app' -maxdepth 6 -print >&2 || true
  exit 2
fi
printf 'Application : %s\n' "$APP_BUNDLE"

printf '\n== Verify packaged iPad full-screen/status-bar policy ==\n'
INFO_PLIST="$APP_BUNDLE/Info.plist"
STATUS_BAR_HIDDEN="$(/usr/libexec/PlistBuddy -c 'Print :UIStatusBarHidden' "$INFO_PLIST" 2>/dev/null || true)"
REQUIRES_FULL_SCREEN="$(/usr/libexec/PlistBuddy -c 'Print :UIRequiresFullScreen' "$INFO_PLIST" 2>/dev/null || true)"
VC_STATUS_POLICY="$(/usr/libexec/PlistBuddy -c 'Print :UIViewControllerBasedStatusBarAppearance' "$INFO_PLIST" 2>/dev/null || true)"
printf 'UIStatusBarHidden                    : %s\n' "${STATUS_BAR_HIDDEN:-<missing>}"
printf 'UIRequiresFullScreen                 : %s\n' "${REQUIRES_FULL_SCREEN:-<missing>}"
printf 'UIViewControllerBasedStatusBarAppearance: %s\n' "${VC_STATUS_POLICY:-<missing>}"
if [[ "$STATUS_BAR_HIDDEN" != "true" || "$REQUIRES_FULL_SCREEN" != "true" || "$VC_STATUS_POLICY" != "false" ]]; then
  echo "ERROR: packaged iPad application does not contain the required full-screen/status-bar policy." >&2
  exit 2
fi

printf '\n== Verify physical-device code signature ==\n'
codesign --verify --deep --strict "$APP_BUNDLE"
printf 'PASS: code signature verifies\n'

printf '\n== Install AIM Editor on %s ==\n' "$AIM_IPAD_NAME"
xcrun devicectl device install app --device "$AIM_IPAD_UDID" "$APP_BUNDLE"

printf '\n== Launch AIM Editor on %s ==\n' "$AIM_IPAD_NAME"
xcrun devicectl device process launch --device "$AIM_IPAD_UDID" "$BUNDLE_ID"

printf '\nPASS: AIM Editor physical iPadOS application build/install/launch\n'
