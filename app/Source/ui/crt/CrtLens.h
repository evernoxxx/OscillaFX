#pragma once

#include <juce_graphics/juce_graphics.h>

// Barrel curvature of the tube face. The composite shader samples the phosphor at
// toPhosphor (screen position), with the very same formula (see CrtShaders.cpp, `barrel`), so
// mouse hit-testing through this function is exact rather than approximate.
namespace Crt::Lens
{
    constexpr float barrel = 0.022f;   // gentle: a hint of curvature, no bulge

    // c in -1..1 across the screen; returns where on the (flat) phosphor plane that point looks.
    inline juce::Point<float> distort (juce::Point<float> c)
    {
        const auto r2 = c.x * c.x + c.y * c.y;
        return c * (1.0f + barrel * r2);
    }

    inline juce::Point<float> toPhosphor (juce::Point<float> screen, juce::Rectangle<float> bounds)
    {
        const auto half = juce::Point<float> (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const auto c = distort ({ (screen.x - centre.x) / half.x, (screen.y - centre.y) / half.y });
        return { centre.x + c.x * half.x, centre.y + c.y * half.y };
    }
}
