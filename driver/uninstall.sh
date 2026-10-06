#!/bin/bash
# Remove the driver (Oscilla.driver, or OscillaFX.driver if one was ever installed) and restart coreaudiod.
# Run: sudo ./uninstall.sh
set -euo pipefail
DEST_DIR="/Library/Audio/Plug-Ins/HAL"

[ "$(id -u)" -eq 0 ] || { echo "run with sudo"; exit 1; }
rm -rf "$DEST_DIR/Oscilla.driver" "$DEST_DIR/OscillaFX.driver"
killall coreaudiod || true
echo "removed OscillaFX Audio driver"
