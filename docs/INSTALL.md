# Installing OscillaFX

Requires macOS 13 (Ventura) or later, Apple silicon or Intel.

## Install

1. Quit eqMac (and FxSound, Boom, SoundSource or any other system audio enhancer). Two
   enhancers that both capture system output fight over the default device. In eqMac use
   its menu > Quit, so it restores your speakers as the output first.
2. Double-click `OscillaFX-<version>.pkg` and follow the installer. It installs:
   - `/Applications/OscillaFX.app`
   - `/Library/Audio/Plug-Ins/HAL/Oscilla.driver` (the "OscillaFX Audio" virtual device)

   Audio drops for about two seconds at the end while Core Audio restarts. No reboot.

### "OscillaFX can't be opened" / "unidentified developer"

The package is not signed with an Apple Developer ID, so Gatekeeper may block it the first time:

- Right-click (or Control-click) the `.pkg` > **Open** > **Open**, or
- Open **System Settings > Privacy & Security**, scroll to the message about
  `OscillaFX-<version>.pkg`, click **Open Anyway**, then authenticate.

On recent macOS versions only the second route works. The same applies to `OscillaFX.app` if
macOS blocks it on first launch.

## First run

Open OscillaFX from Applications. macOS asks for **microphone** access: allow it. OscillaFX reads
the OscillaFX Audio virtual device through the input permission, processes it, and plays it on
your real output. Nothing is recorded or stored.

Until access is granted OscillaFX leaves your sound output alone and the screen shows
"MICROPHONE ACCESS NEEDED". If you denied it, enable OscillaFX in **System Settings > Privacy &
Security > Microphone**; OscillaFX starts by itself within a few seconds. Because the app is
ad-hoc signed, macOS may ask again after each update.

## Per-device suggestions, night mode, volume limiter

The first time an output device is used (speakers, headphones, AirPods, a display...) OscillaFX offers
five starting profiles tuned for that kind of device: Flat, Warm, Bass-heavy, Voice, Wide. Nothing
changes until you pick one; "don't ask again" is remembered per device.

**Night mode** (compressor: loud parts down, quiet parts up, bass kept in check) and the **volume
limiter** (a ceiling from -24 to 0 dBFS; 0 = off) are per-device settings and are off by default.
Presets and starter profiles never switch them off.

Your own pictures for the phosphor gallery live in `~/Library/Application Support/Oscilla/Gallery`
(JPEG, PNG, WebP, HEIC, GIF, TIFF, up to 50 MB each). App-wide choices (slideshow, first-run flag)
are in `~/Library/Application Support/Oscilla/prefs.json`.

## Start at login

Turn on **Start at login** in the menu bar menu. OscillaFX registers itself as a login item
(System Settings > General > Login Items lists it; macOS shows a "Background Items Added"
notice). When started at login it opens minimised to the menu bar; click the menu bar icon or
the Dock icon to show the window. If macOS refuses the registration (older macOS or signature
issues), OscillaFX falls back to `~/Library/LaunchAgents/com.oscilla.app.login.plist`.

## If OscillaFX crashes

While OscillaFX runs, the system output is the "OscillaFX Audio" device. If the app crashes or is
force-quit, a small helper (`OscillaFX.app/Contents/Helpers/OscillaWatchdog`) switches the system
output back to your real device, so the Mac never stays silent. It logs to
`~/Library/Logs/Oscilla/watchdog.log` and exits together with the app.

## Updating

Run the new `.pkg`. The installer quits a running OscillaFX first and replaces
app and driver. Your per-device profiles are kept.

## Uninstall

From the source tree:

```bash
packaging/uninstall.sh          # removes app + driver, keeps profiles
packaging/uninstall.sh --purge  # also deletes profiles and resets the microphone permission
```

It asks for your password, quits OscillaFX, removes its login item, removes the app and the driver and
restarts Core Audio. Profiles live in `~/Library/Application Support/Oscilla`.

## Building the installer

```bash
packaging/build_release.sh      # -> dist/OscillaFX-<version>.pkg (universal arm64 + x86_64)
ARCHS=arm64 packaging/build_release.sh   # native-only build
```

Needs Command Line Tools and CMake 3.22+. Presets come from `presets/` plus, if present,
`presets_fxsound/`; gallery images from `assets/gallery/`. The version comes
from `project(OscillaFX VERSION ...)` in `CMakeLists.txt`. The app icon is the 1024x1024
butterfly master `packaging/icon/OscillaFX.png` (JUCE builds the app icns from it; `driver/build.sh`
turns it into the driver's device icon `OscillaFX.icns` with sips/iconutil).

The driver is upstream BlackHole (GPL-3.0) in `driver/third_party/BlackHole`, left untouched:
`driver/build.sh` copies it into the build directory and applies `driver/patches/*.patch`
(OscillaFX names for device, box, manufacturer and plug-in factory). The upstream `LICENSE`
ships in `Oscilla.driver/Contents/Resources`.
