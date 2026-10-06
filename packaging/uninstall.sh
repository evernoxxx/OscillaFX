#!/bin/bash
# Remove OscillaFX.app (and the pre-rename Oscilla.app) plus the driver (Oscilla.driver, or OscillaFX.driver),
# restart coreaudiod (audio drops ~2s).
# Profiles in ~/Library/Application Support/Oscilla are kept unless --purge.
# Usage: packaging/uninstall.sh [--purge]   (asks for your password via sudo)
set -euo pipefail

APPS=(/Applications/OscillaFX.app /Applications/Oscilla.app)
APP_NAMES=(OscillaFX Oscilla) # executable names, matched exactly
DRIVERS=(/Library/Audio/Plug-Ins/HAL/Oscilla.driver /Library/Audio/Plug-Ins/HAL/OscillaFX.driver)
# Package identifiers are stable, so one set of ids forgets the receipts of every release.
PACKAGE_IDS=(com.oscilla.app.pkg com.oscilla.driver.pkg)
QUIT_TIMEOUT_SECONDS=10

is_purge=false
[ "${1:-}" = "--purge" ] && is_purge=true

if [ "$(id -u)" -ne 0 ]; then
    exec sudo "$0" "$@"
fi
user="${SUDO_USER:-$(stat -f %Su /dev/console)}"
home="$(dscl . -read "/Users/$user" NFSHomeDirectory | sed 's/^NFSHomeDirectory: //')"
UNREGISTER_TIMEOUT_SECONDS=10

any_running() {
    local name
    for name in "${APP_NAMES[@]}"; do
        pgrep -xq "$name" && return 0
    done
    return 1
}

quit_app() {
    any_running || return 0
    sudo -u "$user" osascript -e 'tell application id "com.oscilla.app" to quit' >/dev/null 2>&1 || true
    for _ in $(seq "$QUIT_TIMEOUT_SECONDS"); do
        any_running || return 0
        sleep 1
    done
    local name
    for name in "${APP_NAMES[@]}"; do
        pkill -x "$name" || true
    done
}

# SMAppService login items can only be removed by the app itself, in the user's session.
unregister_login_item() {
    local app name executable pid
    for i in "${!APPS[@]}"; do
        app="${APPS[$i]}"
        name="${APP_NAMES[$i]}"
        executable="$app/Contents/MacOS/$name"
        [ -x "$executable" ] || continue
        launchctl asuser "$(id -u "$user")" sudo -u "$user" \
            "$executable" --unregister-login-item >/dev/null 2>&1 &
        pid=$!
        for _ in $(seq "$UNREGISTER_TIMEOUT_SECONDS"); do
            kill -0 "$pid" 2>/dev/null || break
            sleep 1
        done
        kill "$pid" 2>/dev/null || true
    done
}

quit_app
unregister_login_item
rm -rf "${APPS[@]}" "${DRIVERS[@]}"
rm -f "$home/Library/LaunchAgents/com.oscilla.app.login.plist" # start-at-login fallback
for id in "${PACKAGE_IDS[@]}"; do
    pkgutil --forget "$id" >/dev/null 2>&1 || true
done
launchctl kickstart -k system/com.apple.audio.coreaudiod 2>/dev/null || killall coreaudiod || true

if $is_purge; then
    rm -rf "$home/Library/Application Support/Oscilla" "$home/Library/Logs/Oscilla"
    sudo -u "$user" tccutil reset Microphone com.oscilla.app >/dev/null 2>&1 || true
fi
echo "OscillaFX removed$($is_purge && echo ' (profiles and microphone permission purged)' || true)."
