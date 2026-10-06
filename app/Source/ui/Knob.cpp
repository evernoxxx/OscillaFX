#include "ui/Knob.h"
#include "ui/Theme.h"

namespace
{
    constexpr float boundsPerRadius = 2.7f;     // room for the drop shadow and selector pointer
    constexpr float hitRadiusScale = 1.15f;
    constexpr int dragPixelsPerSweep = 220;
    constexpr double keyboardStepsPerSweep = 24.0;
}

Knob::Knob (const juce::String& title, Hardware::KnobStyle s)
    : KeyboardFocusRing (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox), style (s)
{
    setName (title);
    setTitle (title);
    setLookAndFeel (&lookAndFeel.get());
    // JUCE wants non-negative angles: the sweep is centred on 2 pi (12 o'clock) rather than 0.
    constexpr auto noon = juce::MathConstants<float>::twoPi;
    setRotaryParameters (noon - Theme::Design::knobSweep, noon + Theme::Design::knobSweep, true);
    setMouseDragSensitivity (dragPixelsPerSweep);
    setVelocityBasedMode (false);
    setPopupDisplayEnabled (false, false, nullptr);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    setWantsKeyboardFocus (true);
}

Knob::~Knob()
{
    setLookAndFeel (nullptr);
}

void Knob::configure (double min, double max, double defaultValue, double interval,
                      std::function<juce::String (double)> formatValue)
{
    setRange (min, max, interval);
    setDoubleClickReturnValue (true, defaultValue);
    textFromValueFunction = std::move (formatValue);
    setValue (defaultValue, juce::dontSendNotification);
}

void Knob::describe (const juce::String& text)
{
    setTooltip (text);
    setHelpText (text);
}

void Knob::setReadout (ValueReadout* r)
{
    readout = r;
}

void Knob::placeAt (juce::Point<float> centre)
{
    const auto size = juce::roundToInt (style.radius * boundsPerRadius);
    setBounds (juce::Rectangle<int> (size, size).withCentre (centre.roundToInt()));
}

bool Knob::hitTest (int x, int y)
{
    const auto centre = getLocalBounds().toFloat().getCentre();
    return centre.getDistanceFrom ({ (float) x, (float) y }) <= style.radius * hitRadiusScale;
}

void Knob::valueChanged()
{
    showReadout();
}

void Knob::mouseDown (const juce::MouseEvent& e)
{
    Slider::mouseDown (e);
    showReadout();
}

void Knob::mouseUp (const juce::MouseEvent& e)
{
    Slider::mouseUp (e);
    if (readout != nullptr)
        readout->hideSoon();
}

bool Knob::keyPressed (const juce::KeyPress& key)
{
    const auto code = key.getKeyCode();
    const auto direction = (code == juce::KeyPress::upKey || code == juce::KeyPress::rightKey) ? 1.0
                         : (code == juce::KeyPress::downKey || code == juce::KeyPress::leftKey) ? -1.0 : 0.0;
    const auto mods = key.getModifiers();
    if (direction == 0.0 || mods.isCommandDown() || mods.isCtrlDown() || mods.isAltDown() || ! isEnabled())
        return Slider::keyPressed (key);

    const auto interval = getInterval() > 0.0 ? getInterval() : (getMaximum() - getMinimum()) / 100.0;
    const auto coarse = juce::jmax (interval, std::round ((getMaximum() - getMinimum()) / keyboardStepsPerSweep / interval) * interval);
    setValue (getValue() + direction * (mods.isShiftDown() ? interval : coarse), juce::sendNotificationSync);
    return true;
}

void Knob::showReadout()
{
    if (readout != nullptr && isEnabled())
    {
        const auto visible = juce::roundToInt (style.radius * hitRadiusScale * 2.0f);
        const auto knobArea = juce::Rectangle<int> (visible, visible).withCentre (getLocalBounds().getCentre());
        readout->show (*this, knobArea, getTitle(), getTextFromValue (getValue()), isMouseButtonDown());
    }
}
