#pragma once

#include "ui/Hardware.h"
#include "ui/PanelLookAndFeel.h"
#include "ui/PanelWidgets.h"

// Panel knob. Vertical drag, scroll wheel or arrow keys turn it; double-click resets it. While it
// is being changed, its value (with units) is shown in the panel's ValueReadout.
class Knob : public KeyboardFocusRing<juce::Slider>
{
public:
    Knob (const juce::String& title, Hardware::KnobStyle);
    ~Knob() override;

    // `formatValue` is used for the read-out and for VoiceOver, so it includes units.
    void configure (double min, double max, double defaultValue, double interval,
                    std::function<juce::String (double)> formatValue);

    // One plain-language line, shown as the tooltip and read by VoiceOver as the help text.
    void describe (const juce::String&);

    void setReadout (ValueReadout*);

    // Places the knob centred on a design-space point.
    void placeAt (juce::Point<float> centre);

    // Optional override of the pointer angle (radians from 12 o'clock) for a value.
    std::function<float (double)> angleForValue;

    const Hardware::KnobStyle style;

    bool hitTest (int x, int y) override;
    void valueChanged() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;   // arrows: coarse steps; with Shift: fine

private:
    void showReadout();

    juce::SharedResourcePointer<PanelLookAndFeel> lookAndFeel;
    ValueReadout* readout = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
};
