#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Procedural surface textures, generated on first use and cached. Tiles are authored at 2x:
// draw them with a 0.5 scale so one texel lands on one physical pixel on Retina displays.
namespace Textures
{
    // Owns the cached images. Hold a juce::SharedResourcePointer<Textures::Cache> while drawing
    // (MainPanel does), so they are generated once and released before JUCE shuts down.
    class Cache
    {
    public:
        juce::Image orangePeel, paintedMetal, rubberGrain, spunAluminium;
    };

    juce::Image orangePeel();     // cream plastic: fine "orange peel" grain overlay, tileable
    juce::Image paintedMetal();   // module plates: finer, calmer grain overlay, tileable
    juce::Image rubberGrain();    // tube gasket rubber overlay, tileable
    juce::Image spunAluminium();  // knob cap: concentric spin marks with a bow-tie sheen, opaque
}
