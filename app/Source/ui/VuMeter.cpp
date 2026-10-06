#include "ui/VuMeter.h"
#include "ui/Engraving.h"
#include "ui/Hardware.h"
#include "ui/Theme.h"

namespace Vu
{
float Ballistics::step (float meanAbsolute, float seconds)
{
    const auto target = juce::jlimit (0.0f, 1.05f, meanAbsolute * averageToDeflection);
    const auto timeConstant = integrationSeconds / std::log (100.0f);   // 99 % after integrationSeconds
    needle += (target - needle) * (1.0f - std::exp (-seconds / timeConstant));
    return needle;
}
}

namespace
{
    using juce::Point;
    using juce::Rectangle;

    constexpr float sweep = juce::degreesToRadians (48.0f);   // needle travel either side of vertical
    const juce::Colour redZone { 0xffc2362b };

    float deflectionForVu (float vu) { return 0.7079f * std::pow (10.0f, vu / 20.0f); }
    float angleForDeflection (float d) { return juce::jmap (juce::jlimit (0.0f, 1.05f, d), -sweep, sweep); }

    struct Layout
    {
        Rectangle<float> housing, face;
        Point<float> pivot;
        float scaleRadius;
        Point<float> lamp;
    };

    Layout layoutFor (Rectangle<float> bounds)
    {
        Layout l;
        l.housing = bounds.reduced (3.0f);
        l.face = l.housing.reduced (11.0f);
        l.pivot = { l.face.getCentreX(), l.face.getBottom() + 4.0f };
        l.scaleRadius = l.face.getHeight() * 0.78f;
        l.lamp = { l.face.getRight() - 20.0f, l.face.getY() + 20.0f };
        return l;
    }

    Point<float> onScale (const Layout& l, float angle, float radius)
    {
        return l.pivot + Point<float> (std::sin (angle), -std::cos (angle)) * radius;
    }

    void paintHousing (juce::Graphics& g, const Layout& l)
    {
        juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 5, { 1, 3 }).drawForRectangle (g, l.housing.toNearestInt());
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3835), l.housing.getTopLeft(), juce::Colour (0xff0b0a09), l.housing.getBottomRight(), false));
        g.fillRoundedRectangle (l.housing, 12.0f);
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.drawRoundedRectangle (l.housing.reduced (1.0f), 11.0f, 1.0f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (l.face.expanded (3.0f), 7.0f);
    }

    void paintFace (juce::Graphics& g, const Layout& l)
    {
        const auto face = l.face;
        g.setGradientFill (juce::ColourGradient::vertical (Theme::creamLight, face.getY(), Theme::putty.darker (0.08f), face.getBottom()));
        g.fillRoundedRectangle (face, 5.0f);

        juce::Graphics::ScopedSaveState state (g);
        juce::Path clip;
        clip.addRoundedRectangle (face, 5.0f);
        g.reduceClipRegion (clip);

        // Window recess: the dial sits behind the housing lip.
        g.setGradientFill (juce::ColourGradient::vertical (juce::Colours::black.withAlpha (0.38f), face.getY(), juce::Colours::transparentBlack, face.getY() + 20.0f));
        g.fillRect (face);
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.22f), face.getX(), 0.0f, juce::Colours::transparentBlack, face.getX() + 14.0f, 0.0f, false));
        g.fillRect (face);
    }

    void paintScale (juce::Graphics& g, const Layout& l)
    {
        const auto r = l.scaleRadius;
        const auto zero = angleForDeflection (deflectionForVu (0.0f));

        juce::Path arc;
        arc.addCentredArc (l.pivot.x, l.pivot.y, r, r, 0.0f, angleForDeflection (deflectionForVu (-20.0f)), zero, true);
        g.setColour (Theme::legendInk);
        g.strokePath (arc, juce::PathStrokeType (1.5f));

        juce::Path red;
        red.addCentredArc (l.pivot.x, l.pivot.y, r - 2.5f, r - 2.5f, 0.0f, zero, angleForDeflection (deflectionForVu (3.0f)), true);
        g.setColour (redZone);
        g.strokePath (red, juce::PathStrokeType (6.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));

        struct Mark { float vu; const char* text; };
        const Mark majors[] = { { -20, "-20" }, { -10, "-10" }, { -7, "-7" }, { -5, "-5" }, { -3, "-3" }, { -2, "-2" }, { -1, "-1" },
                                { 0, "0" }, { 1, "+1" }, { 2, "+2" }, { 3, "+3" } };
        for (const auto& m : majors)
        {
            const auto a = angleForDeflection (deflectionForVu (m.vu));
            g.setColour (m.vu > 0.0f ? redZone.darker (0.2f) : Theme::legendInk);
            g.drawLine ({ onScale (l, a, r), onScale (l, a, r - 9.0f) }, 1.6f);
            g.setFont (juce::Font (juce::FontOptions (12.5f, juce::Font::bold)));
            g.drawText (m.text, Rectangle<float> (26.0f, 14.0f).withCentre (onScale (l, a, r + 12.0f)), juce::Justification::centred, false);
        }

        g.setColour (Theme::legendInk);
        for (const float vu : { -15.0f, -9.0f, -8.0f, -6.0f, -4.0f })
        {
            const auto a = angleForDeflection (deflectionForVu (vu));
            g.drawLine ({ onScale (l, a, r), onScale (l, a, r - 5.0f) }, 1.0f);
        }
    }

    void paintLegends (juce::Graphics& g, const Layout& l, const juce::String& channel)
    {
        const auto face = l.face;
        g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSerifFontName(), 26.0f, juce::Font::bold)));
        g.setColour (Theme::legendInk);
        g.drawText ("VU", Rectangle<float> (80.0f, 30.0f).withCentre ({ face.getCentreX(), face.getBottom() - 62.0f }), juce::Justification::centred, false);

        Engraving::text (g, channel, { face.getX() + 38.0f, face.getBottom() - 14.0f }, 11.0f, true);
        Engraving::text (g, "PEAK", { l.lamp.x - 24.0f, l.lamp.y }, 10.5f, true, juce::Justification::right);
    }

    void paintHub (juce::Graphics& g, const Layout& l)
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (l.face.toNearestInt());
        const auto c = l.pivot;
        constexpr float radius = 24.0f;
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.fillEllipse (Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c.translated (2.0f, 3.0f)));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4845), c.translated (-radius * 0.5f, -radius * 0.8f), juce::Colour (0xff050505), c.translated (radius * 0.6f, radius * 0.6f), false));
        g.fillEllipse (Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c));
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawEllipse (Rectangle<float> (radius * 1.8f, radius * 1.8f).withCentre (c), 1.0f);
    }
}

//==============================================================================
// The static dial, cached by JUCE as an image at the display's pixel scale.
class VuMeter::Dial : public juce::Component
{
public:
    explicit Dial (const juce::String& channelLegend) : channel (channelLegend)
    {
        setInterceptsMouseClicks (false, false);
        setAccessible (false);
        setBufferedToImage (true);
        setOpaque (false);
    }

    void paint (juce::Graphics& g) override
    {
        const auto layout = layoutFor (getLocalBounds().toFloat());
        paintHousing (g, layout);
        paintFace (g, layout);
        paintScale (g, layout);
        paintLegends (g, layout, channel);
    }

private:
    const juce::String channel;
};

VuMeter::VuMeter (const juce::String& channelLegend) : dial (std::make_unique<Dial> (channelLegend))
{
    setInterceptsMouseClicks (false, false);
    setAccessible (false);
    addAndMakeVisible (*dial);
}

VuMeter::~VuMeter() = default;

void VuMeter::resized()
{
    dial->setBounds (getLocalBounds());
}

void VuMeter::setReading (float deflection, bool isPeakLit)
{
    if (juce::approximatelyEqual (deflection, needle) && isPeakLit == peakLit)
        return;
    needle = deflection;
    peakLit = isPeakLit;
    repaint();
}

void VuMeter::paintOverChildren (juce::Graphics& g)
{
    const auto layout = layoutFor (getLocalBounds().toFloat());

    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path clip;
        clip.addRoundedRectangle (layout.face, 5.0f);
        g.reduceClipRegion (clip);

        const auto angle = angleForDeflection (needle);
        const auto tip = onScale (layout, angle, layout.scaleRadius + 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.22f));
        g.drawLine ({ layout.pivot.translated (2.5f, 3.0f), tip.translated (2.5f, 3.0f) }, 2.0f);
        g.setColour (juce::Colour (0xff0e0c0a));
        g.drawLine ({ layout.pivot, tip }, 1.7f);
    }

    paintHub (g, layout);   // the hub covers the needle's root

    Hardware::led (g, layout.lamp, 4.6f, peakLit ? 1.0f : 0.0f, Theme::ledRed);

    // Glass reflection over the dial.
    juce::Graphics::ScopedSaveState state (g);
    juce::Path clip;
    clip.addRoundedRectangle (layout.face, 5.0f);
    g.reduceClipRegion (clip);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.2f), layout.face.getX(), layout.face.getY(),
                                             juce::Colours::white.withAlpha (0.0f), layout.face.getCentreX(), layout.face.getCentreY(), false));
    g.fillRect (layout.face);
}
