#!/usr/bin/env bash
#
# build_edfbrowser.sh — clone and build EDFbrowser from source on macOS
#
# Copyright (C) 2026 Nikolai Zhivotenko
#
# This build script is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# EDFbrowser itself is Copyright (C) 2007-2026 Teunis van Beelen
# and remains under the GNU GPL v3. This script does not replace that notice.
#
# Usage:
#   ./build_edfbrowser.sh              # build and open the app
#   ./build_edfbrowser.sh --no-open    # build only
#   ./build_edfbrowser.sh --install    # build and copy to /Applications
#   ./build_edfbrowser.sh --dmg        # build and create distributable DMG
#   EDF_TAG=v2.14 ./build_edfbrowser.sh
#   QMAKE=/path/to/qmake ./build_edfbrowser.sh
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_URL="https://gitlab.com/Teuniz/EDFbrowser.git"
APP_BUNDLE="EDFbrowser.app"

# The script lives either next to the sources (this repository)
# or one directory above a clone named EDFbrowser.
if [[ -f "${SCRIPT_DIR}/edfbrowser.pro" ]]; then
  SOURCE_DIR="${SCRIPT_DIR}"
else
  SOURCE_DIR="${SCRIPT_DIR}/EDFbrowser"
fi

OPEN_APP=true
INSTALL_APP=false
CREATE_DMG=false

for arg in "$@"; do
  case "$arg" in
    --no-open)
      OPEN_APP=false
      ;;
    --install)
      INSTALL_APP=true
      ;;
    --dmg)
      CREATE_DMG=true
      OPEN_APP=false
      ;;
    -h|--help)
      sed -n '15,21p' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *)
      echo "Unknown argument: $arg" >&2
      echo "Usage: $0 [--no-open] [--install] [--dmg]" >&2
      exit 1
      ;;
  esac
done

log() {
  echo "==> $*"
}

die() {
  echo "Error: $*" >&2
  exit 1
}

require_command() {
  local cmd="$1"
  local hint="$2"
  if ! command -v "$cmd" >/dev/null 2>&1; then
    die "Command '$cmd' not found. $hint"
  fi
}

check_xcode_tools() {
  if ! xcode-select -p >/dev/null 2>&1; then
    die "Xcode Command Line Tools are not installed. Run: xcode-select --install"
  fi
}

find_qmake() {
  local candidates=()

  if [[ -n "${QMAKE:-}" ]]; then
    candidates+=("$QMAKE")
  fi

  candidates+=(
    "/opt/homebrew/opt/qt@5/bin/qmake"
    "/usr/local/opt/qt@5/bin/qmake"
  )

  if command -v brew >/dev/null 2>&1; then
    local brew_qt5
    brew_qt5="$(brew --prefix qt@5 2>/dev/null || true)"
    if [[ -n "$brew_qt5" ]]; then
      candidates+=("$brew_qt5/bin/qmake")
    fi

    local brew_qt5_alt
    brew_qt5_alt="$(brew --prefix qt5 2>/dev/null || true)"
    if [[ -n "$brew_qt5_alt" ]]; then
      candidates+=("$brew_qt5_alt/bin/qmake")
    fi
  fi

  candidates+=(
    "/opt/local/libexec/qt5/bin/qmake"
    "/usr/local/libexec/qt5/bin/qmake"
  )

  if command -v brew >/dev/null 2>&1; then
    local brew_qt
    brew_qt="$(brew --prefix qt 2>/dev/null || true)"
    if [[ -n "$brew_qt" ]]; then
      candidates+=("$brew_qt/bin/qmake")
    fi
  fi

  local candidate
  for candidate in "${candidates[@]}"; do
    if [[ -x "$candidate" ]]; then
      echo "$candidate"
      return 0
    fi
  done

  return 1
}

maybe_patch_global_h() {
  local global_h="${SOURCE_DIR}/global.h"
  if [[ ! -f "$global_h" ]]; then
    return 0
  fi

  if grep -q '^[[:space:]]*#error[[:space:]]*"configuration error"' "$global_h"; then
    log "Applying safety patch to global.h (commenting out #error \"configuration error\")"
    sed -i '' 's/^[[:space:]]*#error[[:space:]]*"configuration error"/\/\/ #error "configuration error"/' "$global_h"
  fi
}

clone_or_update_sources() {
  if [[ "$SOURCE_DIR" == "$SCRIPT_DIR" ]]; then
    log "Using sources in $SOURCE_DIR"
  elif [[ ! -d "$SOURCE_DIR/.git" ]]; then
    log "Cloning sources from $REPO_URL"
    git clone "$REPO_URL" "$SOURCE_DIR"
  else
    log "Updating sources in $SOURCE_DIR"
    git -C "$SOURCE_DIR" pull --ff-only
  fi

  if [[ -n "${EDF_TAG:-}" ]]; then
    log "Checking out tag $EDF_TAG"
    git -C "$SOURCE_DIR" fetch --tags
    git -C "$SOURCE_DIR" checkout "$EDF_TAG"
  fi
}

find_macdeployqt() {
  local qmake_bin="$1"
  local macdeployqt_bin

  macdeployqt_bin="$(dirname "$qmake_bin")/macdeployqt"
  if [[ -x "$macdeployqt_bin" ]]; then
    echo "$macdeployqt_bin"
    return 0
  fi

  return 1
}

bundle_qt_libraries() {
  local qmake_bin="$1"
  local app_path="${SOURCE_DIR}/${APP_BUNDLE}"
  local macdeployqt_bin

  if ! macdeployqt_bin="$(find_macdeployqt "$qmake_bin")"; then
    log "macdeployqt not found, skipping Qt library bundling"
    return 0
  fi

  log "Bundling Qt dependencies with macdeployqt"
  "$macdeployqt_bin" "$app_path" -no-strip
  bundle_qtdbus "$app_path"
}

# Qt 6 macdeployqt leaves QtDBus out. QtGui loads it at startup.
bundle_qtdbus() {
  local app_path="$1"
  local frameworks="${app_path}/Contents/Frameworks"
  local dest="${frameworks}/QtDBus.framework"
  local bin="${dest}/Versions/A/QtDBus"
  local qt_prefix=""
  local qt_lib=""
  local dbus_ref=""
  local dbus_name=""

  if [[ -f "$bin" ]]; then
    return 0
  fi

  if ! command -v brew >/dev/null 2>&1; then
    log "QtDBus.framework is missing and Homebrew is not available"
    return 0
  fi

  qt_prefix="$(brew --prefix qt 2>/dev/null || true)"
  qt_lib="${qt_prefix}/lib/QtDBus.framework"
  if [[ ! -d "$qt_lib" ]]; then
    log "QtDBus.framework was not found, skipping"
    return 0
  fi

  log "Copying QtDBus.framework into the app"
  ditto "$qt_lib" "$dest"

  dbus_ref="$(otool -L "$bin" | awk '/libdbus-/{print $1; exit}')"
  if [[ -n "$dbus_ref" && -f "$dbus_ref" ]]; then
    dbus_name="$(basename "$dbus_ref")"
    cp -f "$dbus_ref" "${frameworks}/${dbus_name}"
    chmod u+w "${frameworks}/${dbus_name}" "$bin"
    install_name_tool -id "@executable_path/../Frameworks/${dbus_name}" "${frameworks}/${dbus_name}"
    install_name_tool -change "$dbus_ref" "@executable_path/../Frameworks/${dbus_name}" "$bin"
  fi

  chmod u+w "$bin"
  install_name_tool -id "@executable_path/../Frameworks/QtDBus.framework/Versions/A/QtDBus" "$bin"
  if otool -L "$bin" | grep -q '@rpath/QtCore.framework/Versions/A/QtCore'; then
    install_name_tool \
      -change "@rpath/QtCore.framework/Versions/A/QtCore" \
      "@executable_path/../Frameworks/QtCore.framework/Versions/A/QtCore" \
      "$bin"
  fi
}

sign_app_bundle() {
  local app_path="${1:-${SOURCE_DIR}/${APP_BUNDLE}}"
  local item

  # macOS 27 rejects linker signatures and 4 KB page signatures:
  # dyld kills the process with SIGKILL (Code Signature Invalid).
  # --deep signs nested code in the wrong order and can leave the main
  # binary with a "resources must be present" signature.
  log "Signing the app (ad-hoc, inside out)"

  while IFS= read -r item; do
    [[ "$item" == "$app_path/Contents/MacOS/EDFbrowser" ]] && continue
    if file -b "$item" | grep -q 'Mach-O'; then
      codesign --force --sign - "$item"
    fi
  done < <(find "$app_path" -type f)

  while IFS= read -r item; do
    codesign --force --sign - "$item"
  done < <(find "$app_path/Contents" -depth -type d -name '*.framework')

  codesign --force --sign - "$app_path"

  if ! codesign --verify --deep --strict "$app_path"; then
    die "Signature verification failed for $app_path"
  fi
}

fix_agl_linking() {
  local makefile="${SOURCE_DIR}/Makefile"
  if [[ ! -f "$makefile" ]]; then
    return 0
  fi

  if grep -q 'AGL' "$makefile"; then
    log "Removing obsolete AGL framework from the Makefile (not supported by current macOS SDKs)"
    sed -i '' \
      -e 's/-framework AGL//g' \
      -e 's|-I[^ ]*AGL\.framework/Headers/||g' \
      "$makefile"
  fi
}

build_edfbrowser() {
  local qmake_bin="$1"
  local jobs

  jobs="$(sysctl -n hw.ncpu)"

  log "Building with qmake: $qmake_bin"
  log "Parallel build: -j$jobs"

  (
    cd "$SOURCE_DIR"
    # AGL was removed from current macOS SDKs; Qt5 still tries to link it.
    # Clear QMAKE_POST_LINK: its built-in script runs strip, which breaks the signature.
    "$qmake_bin" \
      'QMAKE_LIBS_OPENGL=-framework OpenGL' \
      'CONFIG+=sdk_no_version_check' \
      'QMAKE_POST_LINK='
    fix_agl_linking
    make -j"$jobs"
  )

  bundle_qt_libraries "$qmake_bin"
  sign_app_bundle
}

verify_app_bundle() {
  local app_path="${SOURCE_DIR}/${APP_BUNDLE}"
  if [[ ! -d "$app_path" ]]; then
    die "Build finished, but $app_path was not found"
  fi
  echo "$app_path"
}

install_app() {
  local app_path="$1"
  local target="/Applications/${APP_BUNDLE}"

  log "Copying the app to $target"
  if [[ -d "$target" ]]; then
    rm -rf "$target"
  fi
  ditto "$app_path" "$target"
  # macOS 27 xattr has no -r. Clear quarantine before signing; provenance may remain.
  find "$target" -exec xattr -c {} \; >/dev/null 2>&1 || true
  sign_app_bundle "$target"
  log "Installed: $target"
}

open_app() {
  local app_path="$1"
  log "Launching $app_path"
  open "$app_path"
}

get_app_version() {
  sed -n 's/.*PROGRAM_VERSION[[:space:]]*"\([^"]*\)".*/\1/p' "${SOURCE_DIR}/global.h"
}

detach_dmg() {
  local device="$1"
  hdiutil detach "$device" -quiet 2>/dev/null || \
    hdiutil detach "$device" -force -quiet 2>/dev/null || \
    die "Failed to unmount DMG ($device). Close the Finder window for the image and try again."
}

create_dmg() {
  local app_path="$1"
  local version
  local vol_name
  local dmg_temp
  local dmg_final
  local background_img="${SOURCE_DIR}/install/macos-dmg-background.png"
  local size_mb
  local device=""
  local mount_point=""

  require_command hdiutil "It ships with macOS"
  require_command osascript "It ships with macOS"

  version="$(get_app_version)"
  vol_name="EDFbrowser ${version}"
  dmg_temp="${SCRIPT_DIR}/EDFbrowser-${version}-temp.dmg"
  dmg_final="${SCRIPT_DIR}/EDFbrowser-${version}.dmg"

  log "Creating a distributable DMG"

  rm -f "$dmg_temp" "$dmg_final"

  size_mb="$(du -sm "$app_path" | awk '{print $1}')"
  size_mb=$((size_mb + 50))

  hdiutil create \
    -srcfolder "$app_path" \
    -volname "$vol_name" \
    -fs HFS+ \
    -fsargs "-c c=64,a=16,e=16" \
    -format UDRW \
    -size "${size_mb}m" \
    "$dmg_temp"

  device="$(hdiutil attach -readwrite -noverify "$dmg_temp" | awk '/^\/dev\// {print $1; exit}')"
  [[ -n "$device" ]] || die "Failed to mount the temporary DMG"

  mount_point="/Volumes/${vol_name}"
  sleep 2

  ln -s /Applications "${mount_point}/Applications"

  if [[ -f "$background_img" ]]; then
    mkdir -p "${mount_point}/.background"
    cp "$background_img" "${mount_point}/.background/"

    osascript <<EOF
tell application "Finder"
  tell disk "${vol_name}"
    open
    set current view of container window to icon view
    set toolbar visible of container window to false
    set statusbar visible of container window to false
    set the bounds of container window to {200, 120, 694, 362}
    set viewOptions to the icon view options of container window
    set arrangement of viewOptions to not arranged
    set icon size of viewOptions to 80
    set background picture of viewOptions to file ".background:macos-dmg-background.png"
    set position of item "${APP_BUNDLE}" of container window to {120, 104}
    set position of item "Applications" of container window to {377, 104}
    close
    open
    update without registering applications
    delay 2
  end tell
end tell
EOF
  fi

  sync
  detach_dmg "$device"

  log "Compressing the DMG"
  hdiutil convert "$dmg_temp" -format UDZO -imagekey zlib-level=9 -o "$dmg_final"
  rm -f "$dmg_temp"

  log "DMG ready: $dmg_final"
  echo "$dmg_final"
}

main() {
  local qmake_bin
  local app_path

  log "Checking dependencies"
  require_command git "Install git, for example: brew install git"
  require_command make "Install make via Xcode Command Line Tools"
  check_xcode_tools

  if ! qmake_bin="$(find_qmake)"; then
    die "qmake not found. Install Qt5: brew install qt@5"
  fi

  log "Using qmake: $qmake_bin"

  clone_or_update_sources
  maybe_patch_global_h
  build_edfbrowser "$qmake_bin"

  app_path="$(verify_app_bundle)"

  log "Build completed successfully"
  echo "App: $app_path"

  if [[ "$INSTALL_APP" == true ]]; then
    install_app "$app_path"
  elif [[ "$CREATE_DMG" != true ]]; then
    echo "To install into /Applications, run: $0 --install"
    echo "To create a DMG, run: $0 --dmg"
  fi

  if [[ "$CREATE_DMG" == true ]]; then
    create_dmg "$app_path"
    echo ""
    echo "Copy the DMG to another Mac. On first launch you may need to:"
    echo "  Right-click EDFbrowser.app → Open (bypass Gatekeeper for the ad-hoc signature)"
  fi

  if [[ "$OPEN_APP" == true ]]; then
    open_app "$app_path"
  fi
}

main "$@"
