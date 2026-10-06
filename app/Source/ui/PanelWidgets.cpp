#include "ui/PanelWidgets.h"
#include "ui/Engraving.h"
#include "ui/Hardware.h"
#include "ui/Theme.h"

namespace
{
    constexpr float ledRadius = 9.0f;
    constexpr float ledBounds = 72.0f;
    constexpr int ledFrameHz = 60;
    constexpr float ledResponse = 0.3f;   // fraction of the remaining distance per frame

    constexpr float legendSize = 14.0f;
    constexpr float horizontalSlotWidth = 56.0f, horizontalSlotHeight = 22.0f;
    constexpr float verticalSlotWidth = 22.0f, verticalSlotHeight = 52.0f;

    constexpr float pushWidth = 44.0f, pushHeight = 32.0f;
    constexpr float plateTextSize = 13.0f;

    constexpr int readoutHideMs = 1100;
    constexpr float readoutTitleSize = 11.0f, readoutValueSize = 20.0f;
    constexpr float readoutGap = 6.0f;
    constexpr float readoutPadding = 12.0f;

    juce::Font monoFont (float size, bool bold)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size,
                                              bold ? juce::Font::bold : juce::Font::plain));
    }

    juce::Path triangle (juce::Rectangle<float> box, bool pointsRight)
    {
        juce::Path p;
        if (pointsRight)
            p.addTriangle (box.getTopLeft(), box.getBottomLeft(), { box.getRight(), box.getCentreY() });
        else
            p.addTriangle (box.getTopRight(), box.getBottomRight(), { box.getX(), box.getCentreY() });
        return p;
    }
}

//==============================================================================
PanelButton::PanelButton (const juce::String& title) : KeyboardFocusRing (title)
{
    setTitle (title);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus (true);
}

bool PanelButton::keyPressed (const juce::KeyPress& key)
{
    if (isEnabled() && (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey))
    {
        triggerClick();
        return true;
    }
    return false;
}

//==============================================================================
PowerLed::PowerLed()
{
    setInterceptsMouseClicks (false, false);
    setAccessible (false); // the POWER switch reports the state
}

void PowerLed::placeAt (juce::Point<float> centre)
{
    setBounds (juce::Rectangle<float> (ledBounds, ledBounds).withCentre (centre).toNearestInt());
}

void PowerLed::setLit (bool isLit)
{
    target = isLit ? 1.0f : 0.0f;
    if (! juce::approximatelyEqual (glow, target))
        startTimerHz (ledFrameHz);
}

void PowerLed::timerCallback()
{
    glow += (target - glow) * ledResponse;
    if (std::abs (target - glow) < 0.01f)
    {
        glow = target;
        stopTimer();
    }
    repaint();
}

void PowerLed::paint (juce::Graphics& g)
{
    Hardware::led (g, getLocalBounds().toFloat().getCentre(), ledRadius, glow, Theme::ledRed);
}

//==============================================================================
SlideSwitch::SlideSwitch (const juce::String& title, Axis a, const juce::String& off, const juce::String& on)
    : PanelButton (title), axis (a), offLegend (off), onLegend (on)
{
    setClickingTogglesState (true);
}

juce::Rectangle<float> SlideSwitch::slotArea() const
{
    const auto bounds = getLocalBounds().toFloat();
    if (axis == Axis::horizontal)
        return juce::Rectangle<float> (horizontalSlotWidth, horizontalSlotHeight).withCentre (bounds.getCentre());
    return juce::Rectangle<float> (verticalSlotWidth, verticalSlotHeight).withCentre ({ bounds.getX() + 18.0f, bounds.getCentreY() });
}

juce::Rectangle<float> SlideSwitch::legendArea (bool isOnSide) const
{
    const auto bounds = getLocalBounds().toFloat();
    const auto slot = slotArea();
    if (axis == Axis::horizontal)
        return isOnSide ? bounds.withLeft (slot.getRight() + 6.0f) : bounds.withRight (slot.getX() - 6.0f);

    const auto right = bounds.withLeft (slot.getRight() + 8.0f);
    return isOnSide ? right.withTop (slot.getCentreY()) : right.withBottom (slot.getCentreY());
}

std::optional<bool> SlideSwitch::legendSideAt (juce::Point<float> p) const
{
    for (auto side : { false, true })
        if (legendArea (side).contains (p))
            return side;
    return std::nullopt;
}

void SlideSwitch::mouseUp (const juce::MouseEvent& e)
{
    // Clicking the legend of the side the switch is already on must not flip it to the other side.
    if (const auto side = legendSideAt (e.position); side.has_value() && *side == getToggleState())
    {
        setState (isMouseOver() ? buttonOver : buttonNormal);
        return;
    }
    Button::mouseUp (e);
}

void SlideSwitch::paintButton (juce::Graphics& g, bool isHighlighted, bool)
{
    const auto isOn = getToggleState();
    const auto slot = slotArea();
    Hardware::slideSwitch (g, slot, isOn ? 1.0f : 0.0f, isHighlighted);

    const auto horizontal = axis == Axis::horizontal;
    const auto offJustify = horizontal ? juce::Justification::centredRight : juce::Justification::centredLeft;
    const auto offBox = horizontal ? legendArea (false) : legendArea (false).withTrimmedBottom (4.0f);
    const auto onBox = horizontal ? legendArea (true) : legendArea (true).withTrimmedTop (4.0f);
    Engraving::textInBox (g, offLegend, offBox, legendSize, offJustify, ! isOn);
    Engraving::textInBox (g, onLegend, onBox, legendSize, juce::Justification::centredLeft, isOn);

    if (isShowingFocusRing())
        Hardware::focusRing (g, slot.expanded (4.0f), 5.0f);
}

//==============================================================================
PushButton::PushButton (const juce::String& title, const juce::String& legendText)
    : PanelButton (title), legend (legendText)
{
}

void PushButton::paintButton (juce::Graphics& g, bool isHighlighted, bool isDown)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto key = juce::Rectangle<float> (pushWidth, pushHeight).withCentre ({ bounds.getCentreX(), bounds.getY() + pushHeight * 0.5f + 2.0f });
    const auto lamp = lampFollowsToggle ? (getToggleState() ? 1.0f : 0.0f) : (isDown ? 1.0f : 0.55f);
    Hardware::creamKey (g, key, isDown, isHighlighted, isEnabled(), lamp);

    const auto opacity = isEnabled() ? 1.0f : 0.4f;
    Engraving::textInBox (g, legend, bounds.withTop (key.getBottom() + 5.0f), 13.0f, juce::Justification::centredTop,
                          isHighlighted && isEnabled(), opacity);

    if (isShowingFocusRing())
        Hardware::focusRing (g, key.expanded (3.0f), 6.0f);
}

//==============================================================================
StepKey::StepKey (const juce::String& title, bool right) : PanelButton (title), pointsRight (right)
{
}

void StepKey::paintButton (juce::Graphics& g, bool isHighlighted, bool isDown)
{
    const auto area = getLocalBounds().toFloat().reduced (1.0f);
    Hardware::creamKey (g, area, isDown, isHighlighted, isEnabled());

    const auto arrow = juce::Rectangle<float> (9.0f, 12.0f).withCentre (area.getCentre().translated (0.0f, isDown ? 1.0f : 0.0f));
    g.setColour (Theme::legendInk.withAlpha (isEnabled() ? 0.95f : 0.35f));
    g.fillPath (triangle (arrow, pointsRight));

    if (isShowingFocusRing())
        Hardware::focusRing (g, area.expanded (2.0f), 6.0f);
}

//==============================================================================
PlateButton::PlateButton (const juce::String& title, bool arrow) : PanelButton (title), showsMenuArrow (arrow)
{
}

void PlateButton::paintButton (juce::Graphics& g, bool isHighlighted, bool isDown)
{
    const auto area = getLocalBounds().toFloat().reduced (1.0f, 2.0f);
    Hardware::recessedPlate (g, area, isHighlighted || isDown);

    if (showsMenuArrow)
    {
        constexpr float arrowWidth = 10.0f;
        const auto arrowBox = juce::Rectangle<float> (arrowWidth, arrowWidth * 0.6f)
                                  .withCentre ({ area.getRight() - 14.0f, area.getCentreY() + 1.0f });
        juce::Path arrow;
        arrow.addTriangle (arrowBox.getTopLeft(), arrowBox.getTopRight(), { arrowBox.getCentreX(), arrowBox.getBottom() });
        g.setColour (Theme::metalGrayLight.withAlpha (0.85f));
        g.fillPath (arrow);
    }

    g.setColour (Theme::metalGrayLight.withAlpha (isEnabled() ? 0.95f : 0.4f));
    g.setFont (Engraving::font (plateTextSize, true));
    g.drawText (getButtonText(), area.withTrimmedLeft (10.0f).withTrimmedRight (showsMenuArrow ? 26.0f : 10.0f), juce::Justification::centred, true);

    if (isShowingFocusRing())
        Hardware::focusRing (g, area.expanded (2.0f), 6.0f);
}

//==============================================================================
ChoiceButton::ChoiceButton (const juce::String& title) : PanelButton (title)
{
}

void ChoiceButton::setDefaultChoice (bool shouldBeDefault)
{
    isDefault = shouldBeDefault;
    repaint();
}

void ChoiceButton::paintButton (juce::Graphics& g, bool isHighlighted, bool isDown)
{
    const auto area = getLocalBounds().toFloat().reduced (1.0f);
    Hardware::creamKey (g, area, isDown, isHighlighted, isEnabled());
    g.setColour (Theme::legendInk.withAlpha (isEnabled() ? 1.0f : 0.4f));
    g.setFont (Engraving::font (12.0f, isDefault || isHighlighted));
    g.drawText (getButtonText(), area.reduced (1.0f, 0.0f).translated (0.0f, isDown ? 1.0f : 0.0f), juce::Justification::centred, true);

    if (isShowingFocusRing())
        Hardware::focusRing (g, area.expanded (2.0f), 6.0f);
}

//==============================================================================
AlertButton::AlertButton (const juce::String& title) : PanelButton (title)
{
}

void AlertButton::paintButton (juce::Graphics& g, bool isHighlighted, bool isDown)
{
    const auto area = getLocalBounds().toFloat().reduced (2.0f);
    const auto lit = Theme::alertAmber.withMultipliedBrightness (isDown ? 0.8f : (isHighlighted ? 1.1f : 1.0f));

    g.setColour (Theme::alertAmber.withAlpha (0.25f));
    g.fillRoundedRectangle (area.expanded (2.0f), 6.0f);   // glow onto the bezel
    g.setGradientFill (juce::ColourGradient::vertical (lit.brighter (0.35f), area.getY(), lit.darker (0.35f), area.getBottom()));
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (area, 4.0f, 1.2f);

    const auto arrow = juce::Rectangle<float> (7.0f, 10.0f).withCentre ({ area.getRight() - 14.0f, area.getCentreY() });
    g.setColour (Theme::ink);
    g.fillPath (triangle (arrow, true));
    g.setFont (Engraving::font (plateTextSize, true));
    g.drawText (getButtonText(), area.withTrimmedLeft (8.0f).withTrimmedRight (24.0f), juce::Justification::centred, true);

    if (isShowingFocusRing())
        Hardware::focusRing (g, area.expanded (3.0f), 6.0f);
}

//==============================================================================
PrintedLabel::PrintedLabel (float size, bool bold, juce::Justification j, int lines)
    : textSize (size), isBold (bold), justification (j), maxLines (lines)
{
    setInterceptsMouseClicks (false, false);
    setAccessible (false);
}

void PrintedLabel::setText (const juce::String& newText)
{
    if (newText == text)
        return;
    text = newText;
    repaint();
}

void PrintedLabel::paint (juce::Graphics& g)
{
    Engraving::fittedText (g, text, getLocalBounds().toFloat(), textSize, justification, maxLines, isBold);
}

//==============================================================================
//==============================================================================
LcdReadout::LcdReadout()
{
    setInterceptsMouseClicks (false, false);
    setAccessible (false); // the slider announces its own value
}

void LcdReadout::setText (const juce::String& newText)
{
    if (newText == text)
        return;
    text = newText;
    repaint();
}

void LcdReadout::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (Theme::inkLip.withAlpha (0.4f));
    g.fillRoundedRectangle (area.translated (0.0f, 0.8f), 3.0f);
    g.setColour (Theme::darkGlass.withAlpha (1.0f));
    g.fillRoundedRectangle (area, 3.0f);
    g.setFont (monoFont (11.5f, true));
    g.setColour (Theme::glassTeal.withAlpha (0.25f));
    g.drawText (text, area.translated (0.0f, 0.5f), juce::Justification::centred, false);
    g.setColour (Theme::glassTeal.interpolatedWith (Theme::glassTealHot, 0.3f));
    g.drawText (text, area, juce::Justification::centred, false);
}

//==============================================================================
ValueReadout::ValueReadout()
{
    setInterceptsMouseClicks (false, false);
    setAlwaysOnTop (true);
    setAccessible (false); // controls announce their own values
    setVisible (false);
}

void ValueReadout::show (juce::Component& anchor, juce::Rectangle<int> area, const juce::String& newTitle,
                         const juce::String& newValue, bool isHeld)
{
    auto* parent = getParentComponent();
    if (parent == nullptr)
        return;

    title = newTitle.toUpperCase();
    value = newValue;

    const auto textWidth = juce::jmax (juce::GlyphArrangement::getStringWidth (monoFont (readoutTitleSize, false), title),
                                       juce::GlyphArrangement::getStringWidth (monoFont (readoutValueSize, true), value));
    const auto size = juce::Point<float> (textWidth + readoutPadding * 2.0f + 8.0f, readoutTitleSize + readoutValueSize + 22.0f);
    const auto target = parent->getLocalArea (&anchor, area).toFloat();

    auto box = juce::Rectangle<float> (size.x, size.y).withCentre ({ target.getCentreX(), 0.0f })
                   .withBottomY (target.getY() - readoutGap);
    if (box.getY() < 0.0f)
        box.setY (target.getBottom() + readoutGap);
    box = box.constrainedWithin (parent->getLocalBounds().toFloat());

    setBounds (box.toNearestInt());
    setVisible (true);
    repaint();

    if (isHeld)
        stopTimer();
    else
        hideSoon();
}

void ValueReadout::hideSoon()
{
    startTimer (readoutHideMs);
}

void ValueReadout::timerCallback()
{
    stopTimer();
    setVisible (false);
}

void ValueReadout::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (area.translated (1.5f, 2.5f), 7.0f);
    g.setColour (Theme::darkGlass);
    g.fillRoundedRectangle (area, 7.0f);

    juce::ColourGradient glassSheen (juce::Colours::white.withAlpha (0.07f), area.getTopLeft(),
                                     juce::Colours::transparentWhite, area.getCentre(), false);
    g.setGradientFill (glassSheen);
    g.fillRoundedRectangle (area, 7.0f);

    g.setColour (Theme::glassTeal.withAlpha (0.4f));
    g.drawRoundedRectangle (area.reduced (0.5f), 7.0f, 1.0f);

    auto text = area.reduced (readoutPadding, 9.0f);
    g.setFont (monoFont (readoutTitleSize, false));
    g.setColour (Theme::glassTeal.withAlpha (0.62f));
    g.drawText (title, text.removeFromTop (readoutTitleSize + 2.0f), juce::Justification::centred, false);

    // A soft halo under the digits stands in for phosphor bloom.
    g.setFont (monoFont (readoutValueSize, true));
    g.setColour (Theme::glassTeal.withAlpha (0.18f));
    for (auto offset : { juce::Point<float> (-1.0f, 0.0f), { 1.0f, 0.0f }, { 0.0f, -1.0f }, { 0.0f, 1.0f } })
        g.drawText (value, text.translated (offset.x, offset.y), juce::Justification::centred, false);
    g.setColour (Theme::glassTeal.interpolatedWith (Theme::glassTealHot, 0.35f));
    g.drawText (value, text, juce::Justification::centred, false);
}
