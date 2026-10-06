#include "ui/PanelLookAndFeel.h"
#include "ui/Hardware.h"
#include "ui/Knob.h"
#include "ui/Theme.h"

namespace
{
    constexpr float tooltipTextSize = 13.0f;
    constexpr int tooltipPadding = 9;
    constexpr int tooltipMaxWidth = 340;

    juce::TextLayout tooltipLayout (const juce::String& text)
    {
        juce::AttributedString s;
        s.setJustification (juce::Justification::centredLeft);
        s.append (text, juce::Font (juce::FontOptions (tooltipTextSize)), Theme::glassTealHot.withAlpha (0.92f));
        juce::TextLayout layout;
        layout.createLayoutWithBalancedLineLengths (s, (float) tooltipMaxWidth);
        return layout;
    }
}

PanelLookAndFeel::PanelLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff161616));
    setColour (juce::PopupMenu::textColourId, Theme::metalGrayLight);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::glassTeal.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, Theme::glassTealHot);
    setColour (juce::PopupMenu::headerTextColourId, Theme::metalGray);
    setColour (juce::TooltipWindow::textColourId, Theme::glassTealHot);
}

void PanelLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float startAngle, float endAngle, juce::Slider& slider)
{
    auto* knob = dynamic_cast<Knob*> (&slider);
    if (knob == nullptr)
        return;

    const auto centre = juce::Rectangle<int> (x, y, width, height).toFloat().getCentre();
    const auto angle = knob->angleForValue != nullptr ? knob->angleForValue (slider.getValue())
                                                      : juce::jmap (sliderPos, startAngle, endAngle);
    Hardware::knob (g, centre, knob->style, angle);

    if (knob->isShowingFocusRing())
    {
        const auto ring = knob->style.radius * 1.14f;
        Hardware::focusRing (g, juce::Rectangle<float> (ring * 2.0f, ring * 2.0f).withCentre (centre), ring);
    }
}

void PanelLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int, int)
{
    g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
}

juce::Rectangle<int> PanelLookAndFeel::getTooltipBounds (const juce::String& text, juce::Point<int> screenPos,
                                                         juce::Rectangle<int> parentArea)
{
    const auto layout = tooltipLayout (text);
    const auto w = (int) std::ceil (layout.getWidth()) + tooltipPadding * 2;
    const auto h = (int) std::ceil (layout.getHeight()) + tooltipPadding * 2;

    return juce::Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                                 screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6,
                                 w, h)
        .constrainedWithin (parentArea);
}

void PanelLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    const auto area = juce::Rectangle<int> (width, height).toFloat();
    g.fillAll (Theme::darkGlass.withAlpha (1.0f)); // TooltipWindow is opaque: no rounded corners
    g.setColour (Theme::glassTeal.withAlpha (0.4f));
    g.drawRect (area, 1.0f);

    tooltipLayout (text).draw (g, area.reduced ((float) tooltipPadding));
}
