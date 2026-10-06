#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

// CPU-drawn single-channel masks the GPU renderer composites: the phosphor labels and the hover
// hint. (The graticule is drawn analytically in the shader.) Drawn only when content or size changes.
namespace Crt::Artwork
{
    struct Label
    {
        juce::String text;
        juce::Rectangle<float> area;   // component local
        float height = 10.0f;
        float brightness = 1.0f;       // 0..1
        juce::Justification justification = juce::Justification::centred;
        bool bold = true;
        bool singleLine = false;       // never wraps: too long text ends in "..."

        bool operator== (const Label& o) const
        {
            return text == o.text && area == o.area && juce::exactlyEqual (height, o.height)
                   && juce::exactlyEqual (brightness, o.brightness) && justification == o.justification
                   && bold == o.bold && singleLine == o.singleLine;
        }
    };

    // SingleChannel image of `pixels` size mapping `bounds` onto it.
    juce::Image labels (const std::vector<Label>&, juce::Rectangle<float> bounds, juce::Point<int> pixels);

    juce::Path textPath (const Label&);
}
