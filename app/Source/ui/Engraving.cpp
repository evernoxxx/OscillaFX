#include "ui/Engraving.h"
#include "ui/Theme.h"

namespace Engraving
{
namespace
{
    constexpr float lipOffset = 0.8f;
    constexpr float tracking = 0.07f;
    constexpr float widen = 1.04f;

    juce::Colour lipColour() { return Theme::inkLip.withAlpha (0.3f); }
}

juce::Font font (float height, bool bold)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain)
                           .withHorizontalScale (widen)
                           .withKerningFactor (tracking));
}

void textInBox (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> box, float height,
                juce::Justification justification, bool bold, float opacity)
{
    g.setFont (font (height, bold));
    g.setColour (lipColour().withMultipliedAlpha (opacity));
    g.drawText (s, box.translated (0.0f, lipOffset), justification, true);
    g.setColour (Theme::legendInk.withAlpha (opacity));
    g.drawText (s, box, justification, true);
}

void fittedText (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> box, float height,
                 juce::Justification justification, int maxLines, bool bold, float opacity)
{
    g.setFont (font (height, bold));
    g.setColour (lipColour().withMultipliedAlpha (opacity));
    g.drawFittedText (s, box.translated (0.0f, lipOffset).toNearestInt(), justification, maxLines, 1.0f);
    g.setColour (Theme::legendInk.withAlpha (opacity));
    g.drawFittedText (s, box.toNearestInt(), justification, maxLines, 1.0f);
}

void text (juce::Graphics& g, const juce::String& s, juce::Point<float> centre, float height,
           bool bold, juce::Justification justification)
{
    const auto width = juce::GlyphArrangement::getStringWidth (font (height, bold), s) + height;
    auto box = juce::Rectangle<float> (width, height * 1.4f).withCentre (centre);

    if (justification.testFlags (juce::Justification::left))
        box.setX (centre.x);
    else if (justification.testFlags (juce::Justification::right))
        box.setX (centre.x - box.getWidth());

    textInBox (g, s, box, height, justification, bold);
}

void path (juce::Graphics& g, const juce::Path& p, float thickness)
{
    const juce::PathStrokeType stroke (thickness, juce::PathStrokeType::mitered, juce::PathStrokeType::square);
    g.setColour (lipColour());
    g.strokePath (p, stroke, juce::AffineTransform::translation (0.0f, lipOffset));
    g.setColour (Theme::legendInk);
    g.strokePath (p, stroke);
}

void fill (juce::Graphics& g, const juce::Path& p)
{
    g.setColour (lipColour());
    g.fillPath (p, juce::AffineTransform::translation (0.0f, lipOffset));
    g.setColour (Theme::legendInk);
    g.fillPath (p);
}

void line (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to, float thickness)
{
    juce::Path p;
    p.startNewSubPath (from);
    p.lineTo (to);
    path (g, p, thickness);
}

void dot (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    g.setColour (lipColour());
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre.translated (0.0f, lipOffset)));
    g.setColour (Theme::legendInk);
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
}
}
