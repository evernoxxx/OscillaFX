#include "ui/crt/CrtArtwork.h"

namespace Crt::Artwork
{
namespace
{
    juce::Image blankMask (juce::Point<int> pixels)
    {
        return juce::Image (juce::Image::SingleChannel, juce::jmax (1, pixels.x), juce::jmax (1, pixels.y), true,
                            juce::SoftwareImageType());
    }

    juce::AffineTransform toPixels (juce::Rectangle<float> bounds, juce::Point<int> pixels)
    {
        return juce::AffineTransform::translation (-bounds.getX(), -bounds.getY())
            .scaled ((float) pixels.x / bounds.getWidth(), (float) pixels.y / bounds.getHeight());
    }

    juce::Font fontFor (const Label& label)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), label.height,
                                              label.bold ? juce::Font::bold : juce::Font::plain));
    }

    float widthOf (const juce::Font& font, const juce::String& text)
    {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText (font, text, 0.0f, 0.0f);
        return glyphs.getBoundingBox (0, -1, true).getWidth();
    }

    juce::String elided (const juce::Font& font, juce::String text, float width)
    {
        if (widthOf (font, text) <= width)
            return text;
        while (text.isNotEmpty() && widthOf (font, text + "...") > width)
            text = text.dropLastCharacters (1);
        return text.trimEnd() + "...";
    }
}

juce::Image labels (const std::vector<Label>& items, juce::Rectangle<float> bounds, juce::Point<int> pixels)
{
    auto image = blankMask (pixels);
    juce::Graphics g (image);
    g.addTransform (toPixels (bounds, pixels));
    for (const auto& label : items)
    {
        g.setColour (juce::Colours::white.withAlpha (juce::jlimit (0.0f, 1.0f, label.brightness)));
        g.fillPath (textPath (label));
    }
    return image;
}

juce::Path textPath (const Label& label)
{
    const auto font = fontFor (label);
    const auto& area = label.area;
    juce::GlyphArrangement glyphs;
    if (label.singleLine)
    {
        // snap the baseline to a whole logical pixel so the strokes land the same way every time
        const auto text = elided (font, label.text, area.getWidth());
        glyphs.addJustifiedText (font, text, area.getX(), std::round (area.getCentreY() + font.getAscent() * 0.5f - font.getDescent() * 0.5f),
                                 area.getWidth(), label.justification);
    }
    else
    {
        glyphs.addFittedText (font, label.text, area.getX(), area.getY(), area.getWidth(), area.getHeight(), label.justification, 4, 1.0f);
    }
    juce::Path p;
    glyphs.createPath (p);
    return p;
}
}
