#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Silk-screened / engraved panel lettering and line work: dark ink with a light lip below.
namespace Engraving
{
    juce::Font font (float height, bool bold = false);

    void text (juce::Graphics&, const juce::String&, juce::Point<float> centre, float height,
               bool bold = false, juce::Justification = juce::Justification::centred);

    void textInBox (juce::Graphics&, const juce::String&, juce::Rectangle<float> box, float height,
                    juce::Justification, bool bold = false, float opacity = 1.0f);

    // Wrapped text in a box, top-left or centred; no ellipsis, so give it room for `maxLines`.
    void fittedText (juce::Graphics&, const juce::String&, juce::Rectangle<float> box, float height,
                     juce::Justification, int maxLines, bool bold = false, float opacity = 1.0f);

    void path (juce::Graphics&, const juce::Path&, float thickness);
    void fill (juce::Graphics&, const juce::Path&);
    void line (juce::Graphics&, juce::Point<float> from, juce::Point<float> to, float thickness);
    void dot (juce::Graphics&, juce::Point<float> centre, float radius);
}
