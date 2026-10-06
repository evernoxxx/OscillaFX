#include "ui/Fader.h"
#include "ui/Theme.h"
#include "ui/WindowChrome.h"

namespace
{
    constexpr float fineFactor = 0.12f;           // drag distance multiplier while Shift is held
    constexpr double wheelPerNotch = 0.5;         // fraction of the range for a full wheel unit
    constexpr double keyboardStepsPerRange = 24.0;
    constexpr float grabSlack = 3.0f;
    constexpr float dragStartThreshold = 3.0f;
    constexpr int glideHz = 60;
    constexpr double glideResponse = 0.35;        // fraction of the remaining distance per frame
}

Fader::Fader (const juce::String& title, Orientation o)
    : KeyboardFocusRing (o == Orientation::vertical ? juce::Slider::LinearVertical : juce::Slider::LinearHorizontal,
                         juce::Slider::NoTextBox),
      orientation (o)
{
    setName (title);
    setTitle (title);
    setLookAndFeel (&lookAndFeel.get());
    setPopupDisplayEnabled (false, false, nullptr);
    setMouseCursor (isVertical() ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::LeftRightResizeCursor);
    setWantsKeyboardFocus (true);
}

Fader::~Fader()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void Fader::configure (double min, double max, double defaultVal, double interval,
                       std::function<juce::String (double)> formatValue)
{
    setRange (min, max, interval);
    defaultValue = defaultVal;
    setDoubleClickReturnValue (true, defaultVal);
    textFromValueFunction = std::move (formatValue);
    setValue (defaultVal, juce::dontSendNotification);
}

void Fader::describe (const juce::String& text)
{
    setTooltip (text);
    setHelpText (text);
}

void Fader::setReadout (ValueReadout* r)
{
    readout = r;
}

void Fader::setDetent (double value, double captureRange)
{
    hasDetent = true;
    detentValue = value;
    detentRange = captureRange;
}

void Fader::setLampsOn (bool shouldBeOn)
{
    if (lampsOn == shouldBeOn)
        return;
    lampsOn = shouldBeOn;
    repaint();
}

//==============================================================================
Fader::Track Fader::trackFor (juce::Rectangle<float> bounds, Orientation o)
{
    if (o == Orientation::vertical)
    {
        const auto top = bounds.getY() + lampSpace + edgePadding, bottom = bounds.getBottom() - edgePadding;
        return { juce::Rectangle<float> (slotThickness, bottom - top).withCentre ({ bounds.getCentreX(), (top + bottom) * 0.5f }),
                 bottom - capLong * 0.5f, top + capLong * 0.5f };
    }
    const auto left = bounds.getX() + edgePadding, right = bounds.getRight() - edgePadding;
    return { juce::Rectangle<float> (right - left, slotThickness).withCentre ({ (left + right) * 0.5f, bounds.getCentreY() }),
             left + capLong * 0.5f, right - capLong * 0.5f };
}

float Fader::capCentre (juce::Rectangle<float> bounds, Orientation o, float proportion)
{
    const auto track = trackFor (bounds, o);
    return juce::jmap (proportion, track.firstCentre, track.lastCentre);
}

float Fader::axisOf (juce::Point<float> p) const
{
    return isVertical() ? p.y : p.x;
}

float Fader::proportion()
{
    return (float) valueToProportionOfLength (getValue());
}

juce::Rectangle<float> Fader::capArea()
{
    const auto centre = capCentre (getLocalBounds().toFloat(), orientation, proportion());
    const auto slot = trackFor (getLocalBounds().toFloat(), orientation).slot;
    return isVertical() ? juce::Rectangle<float> (capWide, capLong).withCentre ({ slot.getCentreX(), centre })
                        : juce::Rectangle<float> (capLong, capWide).withCentre ({ centre, slot.getCentreY() });
}

double Fader::valueAtPixel (float axisPosition)
{
    const auto track = trackFor (getLocalBounds().toFloat(), orientation);
    const auto t = juce::jlimit (0.0f, 1.0f, (axisPosition - track.firstCentre) / (track.lastCentre - track.firstCentre));
    return proportionOfLengthToValue (t);
}

//==============================================================================
void Fader::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto track = trackFor (bounds, orientation);
    const auto t = proportion();
    const auto origin = hasDetent ? (float) valueToProportionOfLength (detentValue) : 0.0f;
    const auto glow = isEnabled() && lampsOn ? 1.0f : 0.0f;

    Hardware::faderSlot (g, track.slot, isVertical(), juce::jmin (t, origin), juce::jmax (t, origin), glow);

    if (isVertical())
    {
        const auto level = glow * juce::jlimit (0.15f, 1.0f, 0.35f + std::abs (t - origin) * 1.3f);
        Hardware::led (g, { bounds.getCentreX(), bounds.getY() + lampSpace * 0.5f }, 2.8f, level, Theme::lampGreen);
    }

    Hardware::faderCap (g, capArea(), isVertical(), isOver || isMouseButtonDown(), isMouseButtonDown());

    if (isShowingFocusRing())
        Hardware::focusRing (g, track.slot.expanded (isVertical() ? capWide * 0.5f + 3.0f : 4.0f, isVertical() ? 3.0f : capWide * 0.5f + 3.0f), 5.0f);
}

void Fader::valueChanged()
{
    showReadout();
}

void Fader::showReadout()
{
    if (readout != nullptr && isEnabled())
        readout->show (*this, capArea().expanded (4.0f).toNearestInt(), getTitle(), getTextFromValue (getValue()), isMouseButtonDown());
}

//==============================================================================
double Fader::applyDetent (double rawValue, bool isFine)
{
    const auto value = juce::jlimit (getMinimum(), getMaximum(), rawValue);
    if (! hasDetent || isFine || std::abs (value - detentValue) > detentRange)
    {
        isAtDetent = false;
        return value;
    }

    if (! isAtDetent)
        WindowChrome::detentHaptic();
    isAtDetent = true;
    return detentValue;
}

void Fader::setUserValue (double value)
{
    setValue (value, juce::sendNotificationSync);
}

void Fader::beginRelativeDrag (const juce::MouseEvent& e)
{
    dragStartAxis = axisOf (e.position);
    dragStartValue = getValue();
    wasFine = e.mods.isShiftDown();
}

void Fader::mouseDown (const juce::MouseEvent& e)
{
    if (! isEnabled() || e.mods.isPopupMenu())
        return;

    stopTimer();
    const auto cap = capArea();
    const auto at = axisOf (e.position);
    const auto centre = axisOf (cap.getCentre());
    isGrabbingCap = std::abs (at - centre) <= capLong * 0.5f + grabSlack;
    isAtDetent = hasDetent && juce::approximatelyEqual (getValue(), detentValue);

    if (isGrabbingCap)
    {
        beginRelativeDrag (e);
        showReadout();
        repaint();
        return;
    }

    glideTo (applyDetent (valueAtPixel (at), e.mods.isShiftDown()));
    dragStartAxis = at;
    dragStartValue = glideTarget;
    wasFine = e.mods.isShiftDown();
    showReadout();
}

void Fader::mouseDrag (const juce::MouseEvent& e)
{
    if (! isEnabled() || e.mods.isPopupMenu())
        return;

    const auto at = axisOf (e.position);
    if (! isGrabbingCap && std::abs (at - dragStartAxis) < dragStartThreshold)
        return; // still a click on the track: let the glide finish

    if (isTimerRunning())
    {
        stopTimer();
        setUserValue (glideTarget);
        beginRelativeDrag (e);
        dragStartAxis = at;
    }

    const auto isFine = e.mods.isShiftDown();
    if (isFine != wasFine) // Shift pressed or released mid-drag: carry on from here without a jump
    {
        beginRelativeDrag (e);
        dragStartAxis = at;
    }

    const auto track = trackFor (getLocalBounds().toFloat(), orientation);
    const auto travel = track.lastCentre - track.firstCentre;
    const auto movedProportion = (at - dragStartAxis) / travel;
    const auto startProportion = valueToProportionOfLength (dragStartValue);
    const auto raw = proportionOfLengthToValue (juce::jlimit (0.0, 1.0, startProportion + movedProportion * (isFine ? fineFactor : 1.0f)));
    setUserValue (applyDetent (raw, isFine));
}

void Fader::mouseUp (const juce::MouseEvent&)
{
    isGrabbingCap = false;
    if (readout != nullptr)
        readout->hideSoon();
    repaint();
}

void Fader::mouseDoubleClick (const juce::MouseEvent&)
{
    if (! isEnabled())
        return;
    stopTimer();
    setUserValue (defaultValue);
}

void Fader::mouseEnter (const juce::MouseEvent&)
{
    isOver = true;
    repaint();
}

void Fader::mouseExit (const juce::MouseEvent&)
{
    isOver = false;
    repaint();
}

void Fader::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (! isEnabled())
        return;

    const auto raw = std::abs (wheel.deltaX) > std::abs (wheel.deltaY) ? wheel.deltaX : wheel.deltaY;
    const auto delta = raw * (wheel.isReversed ? -1.0f : 1.0f);
    if (juce::approximatelyEqual (delta, 0.0f))
        return;

    stopTimer();
    const auto range = getMaximum() - getMinimum();
    const auto isFine = e.mods.isShiftDown();
    const auto next = getValue() + (double) delta * range * wheelPerNotch * (isFine ? fineFactor : 0.2);
    setUserValue (applyDetent (next, isFine));
}

bool Fader::keyPressed (const juce::KeyPress& key)
{
    const auto code = key.getKeyCode();
    const auto mods = key.getModifiers();
    if (! isEnabled() || mods.isCommandDown() || mods.isCtrlDown() || mods.isAltDown())
        return Slider::keyPressed (key);

    const auto interval = getInterval() > 0.0 ? getInterval() : (getMaximum() - getMinimum()) / 100.0;
    const auto coarse = juce::jmax (interval, std::round ((getMaximum() - getMinimum()) / keyboardStepsPerRange / interval) * interval);
    const auto step = mods.isShiftDown() ? interval : coarse;

    double target = getValue();
    if (code == juce::KeyPress::upKey || code == juce::KeyPress::rightKey)         target += step;
    else if (code == juce::KeyPress::downKey || code == juce::KeyPress::leftKey)   target -= step;
    else if (code == juce::KeyPress::pageUpKey)                                    target += step * 4.0;
    else if (code == juce::KeyPress::pageDownKey)                                  target -= step * 4.0;
    else if (code == juce::KeyPress::homeKey)                                      target = getMinimum();
    else if (code == juce::KeyPress::endKey)                                       target = getMaximum();
    else
        return Slider::keyPressed (key);

    stopTimer();
    setUserValue (juce::jlimit (getMinimum(), getMaximum(), target));
    return true;
}

//==============================================================================
void Fader::glideTo (double target)
{
    glideTarget = target;
    startTimerHz (glideHz);
}

void Fader::timerCallback()
{
    const auto remaining = glideTarget - getValue();
    const auto snapDistance = juce::jmax (getInterval(), (getMaximum() - getMinimum()) * 0.002);
    if (std::abs (remaining) <= snapDistance)
    {
        stopTimer();
        setUserValue (glideTarget);
        return;
    }
    setUserValue (getValue() + remaining * glideResponse);
}
