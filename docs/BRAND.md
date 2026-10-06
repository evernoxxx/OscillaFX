# OscillaFX brand

The butterfly (`assets/brand/butterfly-source.png`) is the logo and app icon. The artwork is never
redrawn: `assets/brand/make_logo.py` extracts its luminance, auto-levels it and renders it as teal CRT
phosphor (palette from `app/Source/ui/crt/CrtPalette.h`). Look: clean and crisp, a fine regular dot
matrix on the sharp layer, a very faint scanline set, soft bloom, no random grain.

## Assets

| File | What |
|------|------|
| `packaging/icon/OscillaFX.png` | App icon master, 1024x1024 RGBA. Apple grid: 824 px rounded-square CRT glass tile, transparent margin, soft shadow. |
| `packaging/icon/OscillaFX-small-{16,32,64}.png` | Simplified high-contrast icon (no dots/scanlines, bigger butterfly) for tiny sizes. |
| `assets/brand/logo-mark.png`, `logo-mark@{512,256,128,64}.png` | Butterfly mark, transparent background. |
| `assets/brand/logo-lockup.png` | Mark + OSCILLAFX wordmark (cream engraved "OSCILLA", teal phosphor "FX"), transparent. |
| `assets/brand/logo-mark-green.png`, `icon-green.png` | Original-green variants, in case the teal is not wanted. |
| `assets/brand/tray-glyph.png` (18), `tray-glyph@2x.png` (36), `tray-glyph-40.png` | Menu bar template: solid black silhouette on transparent. |
| `app/Source/ui/BrandAssets.h` | Generated: 192 px mark + 40 px tray glyph as PNG byte arrays (about 130 KB of source). |

In the app: `ui/Brand.{h,cpp}` decodes the arrays. Used by the faceplate badge (PanelArt), the tray icon
(template image, alpha x0.45 while power is off), and the CRT splash (`CrtDisplay::updateSplash`: the mark
burns in for about 0.5 s on power-on, fades over 1.2 s; a silent scope with no message keeps it very dimly
lit, static so the idle-stop logic still works).

## Regenerate

    python3 assets/brand/make_logo.py        # Pillow only

Tuning knobs at the top of the script: `GAMMA` (how much stays teal versus white-cyan), `CROP`, `MARK_PX`.
For the green icon in the build, copy `assets/brand/icon-green.png` over `packaging/icon/OscillaFX.png`.

## Palette

| Role | Hex |
|------|-----|
| Glass edge / centre | `#0b181b` / `#112a2d` |
| Phosphor body | `#25d0c0` |
| Phosphor core | `#d8fff8` |
| Halo | `#0b8a96` |
| Graticule | `#2a9d94` |
| Wordmark cream | `#e4d4ae` |
| Green variant body / core / halo | `#22e070` / `#f4ffb0` / `#118a3c` |

Screenshots: `docs/screenshots/brand-*.png`.
