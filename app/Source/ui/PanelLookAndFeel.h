#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Knobs, menus and tooltips for the instrument panel: hardware-drawn knobs, dark popup menus and
// phosphor-on-dark-glass tooltips. Share one instance with juce::SharedResourcePointer, so it is
// deleted together with the last component using it, before JUCE shuts down.
class PanelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PanelLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;

    void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& text, juce::Point<int> screenPos,
                                           juce::Rectangle<int> parentArea) override;
};
