# OscillaFX swatches

Both engineers append a section: Panel (below) and CRT (CRT engineer, teal palette).

## Panel (UI engineer)

Target: Tektronix 7613 (`assets/reference/tek7613.png`). The photo carries a strong yellow cast, so the
values below are sampled medians (cream, frame, module plate, ink, lamp) pulled about 25 % toward neutral
so the panel reads as aged putty rather than mustard. Preview: `docs/screenshots/swatches.png`
(regenerate with `python3 docs/tools/make_swatches.py`).

| Name | Hex | Sampled from | Used for |
|------|-----|--------------|----------|
| panel cream | `#d8c7a0` | upper control panel (raw `#d6be93`) | faceplate base |
| frame putty | `#e4d4ae` | tube frame highlight (raw `#f0debb`) | tube frame lit edge, key caps |
| frame body | `#cdb88c` | frame mid-tone (raw `#d1bc93`) | tube frame, key cap sides |
| shadow | `#9b8559` | cream in shade (raw `#967c4b`) | soft shadows, wear, yellowing |
| ink black | `#17120b` | printed legends (raw `#140a00`) | knobs, legends, outline boxes |
| legend black | `#1c1710` | outline boxes | thin group outlines |
| lamp green | `#4cf29a` | power lamp core (raw `#e6fce3`, hue from halo) | pushbutton LED, fader position LED |
| metal gray | `#cfc8b6` | module plates (raw `#d5bd93`, desaturated further) | painted-metal plates |
| metal gray dark | `#aaa392` | module plate shade | plate gradient, seams |
| teal glass | `#4be3d6` | trace `#77fffb`, screen `#045c4c` | read-out text, tooltip text, focus glow |

Surface recipe: matte plastic with fine "orange peel" (3 noise octaves lit from the upper left, about
4 to 6 % contrast), a diagonal warm yellowing gradient plus low-frequency blotches, and faint edge wear
(lighter rubbed edges and a few chips). Plates are the same grain at half the contrast.

## CRT  - source of truth: app/Source/ui/crt/CrtPalette.h

| Role | Hex |
|------|-----|
| Tube face (edge) | #0b181b |
| Tube face (centre glow) | #112a2d |
| Phosphor core (hot centre) | #d8fff8 |
| Phosphor body (teal) | #25d0c0 |
| Halo (deep blue-green) | #0b8a96 |
| Graticule lines | #2a9d94 |
| Graticule axes / frame / ticks | #46c9be |
| Hover-hint text | #7ff0e4 |

Screenshots: docs/screenshots/crt-*.png

Panel tweak after first render: panel cream `#dac38f`, frame putty `#e6d3a5`, metal gray `#c9c7ba` (warmer faceplate, cooler plates).
