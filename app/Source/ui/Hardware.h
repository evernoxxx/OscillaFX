#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Vector renderings of the physical parts mounted on the panel. Light comes from the upper left.
namespace Hardware
{
    struct KnobStyle
    {
        float radius;      // outer radius of the knurled skirt
        bool isSelector;   // rotary switch: adds a black bar pointer across the skirt
    };

    // Black knurled skirt with a spun-aluminium cap; `angle` is radians from 12 o'clock.
    void knob (juce::Graphics&, juce::Point<float> centre, const KnobStyle&, float angle);

    // Lamp that glows from 0 (dark glass) to 1 (lit) in the given colour.
    void led (juce::Graphics&, juce::Point<float> centre, float radius, float glow, juce::Colour colour);
    void bezelScrew (juce::Graphics&, juce::Point<float> centre, float radius);
    void chromeScrew (juce::Graphics&, juce::Point<float> centre, float radius);

    // Black bakelite slide switch in a slot cut into the panel; `position` 0..1 along the long axis.
    void slideSwitch (juce::Graphics&, juce::Rectangle<float> slot, float position, bool isHighlighted);

    // Cream square key set in a black surround. `lampGlow` < 0 draws no lamp, otherwise a small green LED.
    void creamKey (juce::Graphics&, juce::Rectangle<float>, bool isDown, bool isHighlighted, bool isEnabled, float lampGlow = -1.0f);
    void recessedPlate (juce::Graphics&, juce::Rectangle<float>, bool isHighlighted);

    // Slide-fader slot: dark and narrow, with the part between `litFrom` and `litTo` (0..1 along it) lit.
    void faderSlot (juce::Graphics&, juce::Rectangle<float> slot, bool isVertical, float litFrom, float litTo, float glow);

    // Brushed-silver fader cap with grip lines and an index line; the long edge lies across the travel.
    void faderCap (juce::Graphics&, juce::Rectangle<float> cap, bool isVertical, bool isHighlighted, bool isDown);

    void focusRing (juce::Graphics&, juce::Rectangle<float> area, float cornerSize);

    juce::ColourGradient chromeGradient (juce::Rectangle<float> area);
}
