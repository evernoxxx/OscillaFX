#pragma once

#include "ui/Hardware.h"
#include "ui/PanelLookAndFeel.h"
#include "ui/PanelWidgets.h"

// Slide fader with a narrow slot and a brushed-silver cap, vertical or horizontal. Drag the cap (hold
// Shift for fine control), click the track to glide there, scroll, or use the arrow keys; double-click
// resets. Faders with a detent (balance, EQ bands) stick at the detent and click on a trackpad.
// While it changes, its value (with units) shows in the panel's ValueReadout. Built on juce::Slider
// for its range model and VoiceOver slider semantics (title, value with units, increment actions).
class Fader : public KeyboardFocusRing<juce::Slider>,
              private juce::Timer
{
public:
    enum class Orientation { vertical, horizontal };

    static constexpr float capLong = 20.0f;    // cap size along the travel
    static constexpr float capWide = 32.0f;    // and across it
    static constexpr float slotThickness = 7.0f;
    static constexpr float edgePadding = 3.0f;
    static constexpr float lampSpace = 12.0f;  // vertical faders keep this free above the slot for the lamp

    Fader (const juce::String& title, Orientation);
    ~Fader() override;

    // `formatValue` is used for the read-out and for VoiceOver, so it includes units.
    void configure (double min, double max, double defaultValue, double interval,
                    std::function<juce::String (double)> formatValue);

    // One plain-language line, shown as the tooltip and read by VoiceOver as the help text.
    void describe (const juce::String&);

    void setReadout (ValueReadout*);

    // Sticks within `captureRange` of `value` (not while Shift is held). The lit part of the slot
    // starts there, too.
    void setDetent (double value, double captureRange);

    void setLampsOn (bool);   // false: the position lamps go dark (power off)

    // Axis coordinate (x for horizontal, y for vertical) of the cap centre at `proportion` 0..1 of the
    // range, for a fader occupying `bounds`. Also used to draw the engraved scale on the panel.
    static float capCentre (juce::Rectangle<float> bounds, Orientation, float proportion);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void valueChanged() override;

private:
    struct Track
    {
        juce::Rectangle<float> slot;
        float firstCentre, lastCentre;   // cap centre at the minimum / at the maximum
    };

    static Track trackFor (juce::Rectangle<float> bounds, Orientation);

    bool isVertical() const { return orientation == Orientation::vertical; }
    float axisOf (juce::Point<float>) const;
    float proportion();
    juce::Rectangle<float> capArea();
    double valueAtPixel (float axisPosition);
    double applyDetent (double rawValue, bool isFine);
    void beginRelativeDrag (const juce::MouseEvent&);
    void glideTo (double target);
    void timerCallback() override;
    void setUserValue (double);
    void showReadout();

    const Orientation orientation;
    juce::SharedResourcePointer<PanelLookAndFeel> lookAndFeel;
    ValueReadout* readout = nullptr;
    double defaultValue = 0.0;
    double detentValue = 0.0, detentRange = 0.0;
    bool hasDetent = false;
    bool isAtDetent = false;
    bool lampsOn = true;
    bool isOver = false;

    bool isGrabbingCap = false;
    bool wasFine = false;
    float dragStartAxis = 0.0f;
    double dragStartValue = 0.0;
    double glideTarget = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Fader)
};
