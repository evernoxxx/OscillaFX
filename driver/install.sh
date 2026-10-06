#!/bin/bash
# Install OscillaFX Audio driver (bundle Oscilla.driver) into the system HAL dir. Needs sudo. Restarts coreaudiod
# (all audio drops for ~2s). Run: sudo ./install.sh
set -euo pipefail
cd "$(dirname "$0")"

NAME="${DRIVER_NAME:-Oscilla}"
SRC="$PWD/build/$NAME.driver"
DEST="/Library/Audio/Plug-Ins/HAL/$NAME.driver"

[ "$(id -u)" -eq 0 ] || { echo "run with sudo"; exit 1; }
[ -d "$SRC" ] || { echo "build first: ./build.sh"; exit 1; }

# Fresh copy, then re-sign IN PLACE: overwriting a loaded bundle leaves stale
# code pages and coreaudiod's helper gets killed at dlopen.
rm -rf "$DEST"
ditto --noqtn "$SRC" "$DEST"
xattr -cr "$DEST"
chown -R root:wheel "$DEST"
chmod -R go-w "$DEST"
codesign --force --sign - --timestamp=none "$DEST"
codesign --verify --strict "$DEST"

killall coreaudiod || true
echo "installed $DEST; verify: system_profiler SPAudioDataType | grep -i oscilla"
