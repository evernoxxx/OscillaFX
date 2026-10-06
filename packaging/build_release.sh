#!/bin/bash
# Build Release OscillaFX.app + the OscillaFX Audio driver (Oscilla.driver bundle), ad-hoc sign, and
# produce dist/OscillaFX-<version>.pkg.
# No sudo, installs nothing. Usage: packaging/build_release.sh   (ARCHS="arm64" for a native-only build)
set -euo pipefail

PACKAGING="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(dirname "$PACKAGING")"
ARCHS="${ARCHS:-arm64;x86_64}"
BUILD="$ROOT/build-release"
STAGE="$BUILD/pkg-stage"
DIST="$ROOT/dist"
VERSION="$(sed -nE 's/^project\(OscillaFX VERSION ([0-9.]+).*/\1/p' "$ROOT/CMakeLists.txt")"
APP="$BUILD/OscillaFX_artefacts/Release/OscillaFX.app"
DRIVER="$BUILD/driver/Oscilla.driver"
PRODUCT="$DIST/OscillaFX-$VERSION.pkg"

build_app() {
    cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release "-DCMAKE_OSX_ARCHITECTURES=$ARCHS"
    cmake --build "$BUILD" --config Release --target OscillaFX -j "$(sysctl -n hw.ncpu)"
    codesign --force --deep --sign - --timestamp=none "$APP"
    codesign --verify --deep --strict "$APP"
}

# driver/build.sh patches the BlackHole copy, adds OscillaFX.icns (sips/iconutil, from
# packaging/icon/OscillaFX.png) + LICENSE and signs the bundle. JUCE builds the app icns from the same PNG.
build_driver() {
    BUILD_DIR="$BUILD/driver" VERSION="$VERSION" "$ROOT/driver/build.sh"
}

# Usage: build_component <bundle> <app|driver> <install-location>
# Relocation off: otherwise Installer "upgrades" any same-ID bundle it finds (e.g. a dev build).
build_component() {
    local root="$STAGE/root-$2"
    local plist="$STAGE/$2.plist"
    mkdir -p "$root"
    ditto --noqtn "$1" "$root/$(basename "$1")"
    xattr -cr "$root"
    pkgbuild --analyze --root "$root" "$plist" >/dev/null
    if plutil -extract 0 raw "$plist" >/dev/null 2>&1; then
        plutil -replace 0.BundleIsRelocatable -bool NO "$plist"
    fi
    pkgbuild --root "$root" --component-plist "$plist" --identifier "com.oscilla.$2.pkg" \
        --version "$VERSION" --install-location "$3" --scripts "$PACKAGING/scripts/$2" \
        --ownership recommended "$STAGE/$2.pkg"
}

build_product() {
    local host_archs; host_archs="$(echo "$ARCHS" | tr ';' ',')"
    sed -e "s|@VERSION@|$VERSION|g" -e "s|@HOST_ARCHS@|$host_archs|g" \
        "$PACKAGING/distribution.xml.in" > "$STAGE/distribution.xml"
    mkdir -p "$DIST"
    productbuild --distribution "$STAGE/distribution.xml" --resources "$PACKAGING/resources" \
        --package-path "$STAGE" "$PRODUCT"
}

[ -n "$VERSION" ] || { echo "cannot read project VERSION from CMakeLists.txt"; exit 1; }
build_app
build_driver
rm -rf "$STAGE" && mkdir -p "$STAGE"
chmod +x "$PACKAGING"/scripts/*/*
build_component "$APP" app /Applications
build_component "$DRIVER" driver /Library/Audio/Plug-Ins/HAL
build_product
echo "built: $PRODUCT ($(du -h "$PRODUCT" | cut -f1))"
