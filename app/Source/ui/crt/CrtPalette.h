#pragma once

#include <cstdint>

// Everything that decides how the tube LOOKS lives here, so the whole screen can be re-tuned in one
// place. Both renderers read it: the Metal shader gets these values compiled in as constants (see
// CrtShaders.cpp), the CPU fallback converts them to juce::Colours (PhosphorScreen.cpp).
//
// Look: Tektronix 7613. Dark teal-grey glass with a slight uniform glow, one thin sharp trace that
// burns from teal to near-white cyan with only a tight soft halo, and a crisp lit teal graticule.
// No grain, no dot mask, no colour fringing. docs/DESIGN_SWATCHES.md lists the same values in hex.
namespace Crt
{
    struct Rgb
    {
        float r, g, b;
    };

    constexpr Rgb hex (std::uint32_t rgb)
    {
        return { (float) ((rgb >> 16) & 0xff) / 255.0f, (float) ((rgb >> 8) & 0xff) / 255.0f, (float) (rgb & 0xff) / 255.0f };
    }

    struct Palette
    {
        // ---- tube face -------------------------------------------------------------------
        Rgb face       = hex (0x0b181b);   // unlit glass at the edges: dark teal-grey, never pure black
        Rgb faceCentre = hex (0x112a2d);   // the faint uniform glow lifts it a little towards the middle

        // ---- phosphor (energy ramps dark -> body -> core as the beam dwells) ----------------
        Rgb body = hex (0x25d0c0);         // teal: the skirt of the trace, columns, text
        Rgb core = hex (0xd8fff8);         // near-white cyan: the hot centre of the trace
        Rgb halo = hex (0x0b8a96);         // deeper blue-green soft glow around hot traces

        // ---- graticule (own sharp layer, never blurred by bloom) -----------------------------
        Rgb grid     = hex (0x2a9d94);     // lit division lines
        Rgb gridAxis = hex (0x46c9be);     // centre cross, frame and ticks
        Rgb text     = hex (0x7ff0e4);     // hover hint, drawn crisp over the glass

        // ---- tone map (energy 1.0 = what a static line of density 1.0 deposits) --------------
        float bodyGain   = 2.2f;           // how fast the skirt reaches full teal
        float whiteStart = 0.40f;          // energy where the core starts turning white-cyan
        float whiteEnd   = 1.05f;          // ... and where it is fully near-white
        float haloGain   = 1.15f;          // strength of the soft halo
        float saturation = 1.30f;          // phosphor saturation: stored energy never exceeds this

        // ---- trace --------------------------------------------------------------------------
        float traceSigma = 0.85f;          // beam radius in logical px: ~2 px bright core at 1x

        // ---- glass ----------------------------------------------------------------------------
        float scanlineDepth = 0.055f;      // very faint, one device-pixel row in every `period`
        float vignette      = 0.40f;
        float sheen         = 0.020f;      // broad reflection up-left
        float catchLight    = 0.030f;      // thin reflection along the top edge
        float gridLevel     = 1.0f;        // overall graticule brightness (lamp at full)
    };

    inline constexpr Palette palette {};
}
