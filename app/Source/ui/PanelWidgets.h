#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <optional>

// Draws a focus ring only when focus arrived from the keyboard (Tab), as macOS does; a mouse click
// focuses the control without a ring. A ring survives the window being deactivated and reactivated.
template <typename ComponentType>
class KeyboardFocusRing : public ComponentType
{
public:
    using ComponentType::ComponentType;

    bool isShowingFocusRing() const { return showsRing && this->hasKeyboardFocus (false); }

protected:
    void focusGained (juce::Component::FocusChangeType cause) override
    {
        const auto isWindowReactivation = cause == juce::Component::focusChangedDirectly && hadRingWhenDeactivated;
        showsRing = cause == juce::Component::focusChangedByTabKey || isWindowReactivation;
        hadRingWhenDeactivated = false;
        ComponentType::focusGained (cause);
        this->repaint();
    }

    void focusLost (juce::Component::FocusChangeType cause) override
    {
        // JUCE reports window deactivation as a mouse-click focus loss on the focused component.
        hadRingWhenDeactivated = showsRing && cause == juce::Component::focusChangedByMouseClick;
        showsRing = false;
        ComponentType::focusLost (cause);
        this->repaint();
    }

private:
    bool showsRing = false, hadRingWhenDeactivated = false;
};

// Base for the panel's buttons: pointing-hand cursor, keyboard focus, Space or Return to press.
class PanelButton : public KeyboardFocusRing<juce::Button>
{
public:
    explicit PanelButton (const juce::String& title);
    bool keyPressed (const juce::KeyPress&) override;
};

// Red power LED. An indicator only: the POWER switch is the control. Fades like a real LED.
class PowerLed : public juce::Component,
                 private juce::Timer
{
public:
    PowerLed();
    void placeAt (juce::Point<float> centre);
    void setLit (bool);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    float glow = 0.0f, target = 0.0f;
};

// Two-position slide switch that paints its own position legends, so the whole visible control is
// the hit area: clicking the slot toggles, clicking a legend moves the switch to that side.
class SlideSwitch : public PanelButton
{
public:
    enum class Axis { horizontal, vertical };

    SlideSwitch (const juce::String& title, Axis, const juce::String& offLegend, const juce::String& onLegend);

    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> slotArea() const;
    juce::Rectangle<float> legendArea (bool isOnSide) const;
    std::optional<bool> legendSideAt (juce::Point<float>) const;

    const Axis axis;
    const juce::String offLegend, onLegend;
};

// Momentary cream square key with a small green lamp; its legend is printed below.
class PushButton : public PanelButton
{
public:
    PushButton (const juce::String& title, const juce::String& legend);

    // For a latching key (setClickingTogglesState): the lamp shows the on state instead of a steady glow.
    void setLampShowsToggleState (bool shouldFollow) { lampFollowsToggle = shouldFollow; }

    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;

private:
    const juce::String legend;
    bool lampFollowsToggle = false;
};

// Small cream key with an arrow, for stepping through a list.
class StepKey : public PanelButton
{
public:
    StepKey (const juce::String& title, bool pointsRight);
    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;

private:
    const bool pointsRight;
};

// Dark name plate set into the panel that opens a menu when clicked (output device, preset).
class PlateButton : public PanelButton
{
public:
    explicit PlateButton (const juce::String& title, bool showsMenuArrow = true);
    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;

private:
    const bool showsMenuArrow;
};

// Small cream text key (a choice in a card of choices). `isDefault` draws it bolder.
class ChoiceButton : public PanelButton
{
public:
    explicit ChoiceButton (const juce::String& title);
    void setDefaultChoice (bool);
    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;

private:
    bool isDefault = false;
};

// Amber back-lit warning plate that asks the user to act (e.g. allow microphone access).
class AlertButton : public PanelButton
{
public:
    explicit AlertButton (const juce::String& title);
    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;
};

// Text screen-printed on the panel (decorative: VoiceOver reads the controls, not their captions).
class PrintedLabel : public juce::Component
{
public:
    PrintedLabel (float textSize, bool isBold, juce::Justification, int maxLines = 1);
    void setText (const juce::String&);
    void paint (juce::Graphics&) override;

private:
    const float textSize;
    const bool isBold;
    const juce::Justification justification;
    const int maxLines;
    juce::String text;
};

// Tiny dark-glass plate with a teal monospaced value: always-visible numbers under the sliders.
class LcdReadout : public juce::Component
{
public:
    LcdReadout();
    void setText (const juce::String&);
    void paint (juce::Graphics&) override;

private:
    juce::String text;
};

// Teal read-out on dark glass that pops up above a control while it is being changed.
class ValueReadout : public juce::Component,
                     private juce::Timer
{
public:
    ValueReadout();

    // `area` is in `anchor`'s coordinates; `anchor` may be any descendant of this read-out's parent.
    // While `isHeld` the read-out stays up; otherwise it hides itself shortly after the last call.
    void show (juce::Component& anchor, juce::Rectangle<int> area, const juce::String& title, const juce::String& value, bool isHeld);
    void hideSoon();

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    juce::String title, value;
};
