#include "ui/RotarySelector.h"
#include "ui/Engraving.h"
#include "ui/Theme.h"

namespace
{
    constexpr float legendGap = 7.0f;
    constexpr float legendHitMargin = 4.0f;
    constexpr float centreBand = 0.15f;      // radians either side of 12 o'clock where legends sit above the dot
    constexpr float dotRadius = 2.6f;
    constexpr float wheelStep = 0.12f;       // accumulated wheel delta per detent; trackpads send many small deltas
    constexpr int dragPixelsPerDetent = 26;
}

RotarySelector::RotarySelector (const juce::String& title, Geometry g)
    : geometry (g), knob (title, { g.knobRadius, true })
{
    // Untitled on purpose: VoiceOver should meet one "Display" control (the knob), not a group and a knob.
    setFocusContainerType (FocusContainerType::none);
    knob.angleForValue = [this] (double v) { return angleForIndex (juce::roundToInt (v)); };
    knob.textFromValueFunction = [this] (double v) { return items[juce::roundToInt (v)]; };
    knob.setDoubleClickReturnValue (false, 0.0);
    knob.setScrollWheelEnabled (false); // handled here, one detent per notch
    knob.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    knob.onValueChange = [this] { knobMoved(); };
    addAndMakeVisible (knob);
}

void RotarySelector::describe (const juce::String& text)
{
    knob.describe (text);
}

void RotarySelector::setReadout (ValueReadout* readout)
{
    knob.setReadout (readout);
}

juce::Point<float> RotarySelector::centre() const
{
    return { geometry.dotRadius + legendGap + geometry.maxLegendWidth + 4.0f, geometry.dotRadius + geometry.legendSize * 1.8f };
}

void RotarySelector::placeAt (juce::Point<float> c)
{
    const auto origin = centre();
    const auto below = juce::jmax (geometry.knobRadius * 1.4f, geometry.dotRadius + geometry.legendSize);
    setBounds (juce::Rectangle<float> (c.x - origin.x, c.y - origin.y, origin.x * 2.0f, origin.y + below).toNearestInt());
}

void RotarySelector::resized()
{
    knob.placeAt (centre());
    layoutLegends();
}

void RotarySelector::setItems (const juce::StringArray& names, int selectedIndex)
{
    items = names;
    const auto last = juce::jmax (1, items.size() - 1);
    knob.setRange (0.0, (double) last, 1.0);
    knob.setMouseDragSensitivity (dragPixelsPerDetent * last);
    knob.setEnabled (items.size() > 1);
    windowStart = 0;
    selected = -2; // differs from any valid selection and from "none", so the refresh below always runs
    setSelectedIndex (selectedIndex);
}

void RotarySelector::setSelectedIndex (int index)
{
    const auto newSelection = juce::isPositiveAndBelow (index, items.size()) ? index : -1;
    if (newSelection == selected)
        return;

    selected = newSelection;
    if (selected >= 0)
    {
        scrollWindowTo (selected);
        knob.setValue (selected, juce::dontSendNotification);
    }
    layoutLegends();
    knob.repaint();
    repaint();
}

float RotarySelector::slotAngle (int slot) const
{
    const auto visible = juce::jmin (items.size(), geometry.maxSlots);
    return ((float) slot - (float) (visible - 1) * 0.5f) * geometry.slotStepRadians;
}

float RotarySelector::angleForIndex (int index) const
{
    return slotAngle (juce::jlimit (0, juce::jmax (0, geometry.maxSlots - 1), index - windowStart));
}

void RotarySelector::scrollWindowTo (int index)
{
    const auto maxStart = juce::jmax (0, items.size() - geometry.maxSlots);
    if (index < windowStart)
        windowStart = index;
    else if (index >= windowStart + geometry.maxSlots)
        windowStart = index - geometry.maxSlots + 1;
    windowStart = juce::jlimit (0, maxStart, windowStart);
}

juce::String RotarySelector::legendText (int index) const
{
    if (legendForItem != nullptr)
        return legendForItem (items[index]);
    return items[index].toUpperCase();
}

void RotarySelector::layoutLegends()
{
    legends.clear();
    const auto c = centre();
    const auto visible = juce::jmin (items.size(), geometry.maxSlots);

    for (int slot = 0; slot < visible; ++slot)
    {
        const auto index = windowStart + slot;
        const auto a = slotAngle (slot);
        const auto dot = c + juce::Point<float> (std::sin (a), -std::cos (a)) * geometry.dotRadius;
        const auto font = Engraving::font (geometry.legendSize, true); // measure bold: the selected legend is bold
        const auto width = juce::jmin (geometry.maxLegendWidth, juce::GlyphArrangement::getStringWidth (font, legendText (index)) + 4.0f);
        juce::Rectangle<float> text (width, geometry.legendSize * 1.4f);

        if (a < -centreBand)
            text.setPosition (dot.x - legendGap - width, dot.y - text.getHeight() * 0.5f);
        else if (a > centreBand)
            text.setPosition (dot.x + legendGap, dot.y - text.getHeight() * 0.5f);
        else
            text.setCentre (dot.x, dot.y - geometry.legendSize * 1.1f);

        legends.push_back ({ index, dot, text });
    }
}

void RotarySelector::paint (juce::Graphics& g)
{
    for (const auto& legend : legends)
    {
        const auto isSelected = legend.index == selected;
        Engraving::dot (g, legend.dot, isSelected ? dotRadius * 1.3f : dotRadius);
        const auto justification = legend.text.getCentreX() < centre().x - 1.0f ? juce::Justification::centredRight
                                 : legend.text.getCentreX() > centre().x + 1.0f ? juce::Justification::centredLeft
                                                                                 : juce::Justification::centred;
        Engraving::textInBox (g, legendText (legend.index), legend.text, geometry.legendSize, justification,
                              isSelected, legend.index == hovered && ! isSelected ? 0.7f : 1.0f);
    }
}

int RotarySelector::legendIndexAt (juce::Point<float> p) const
{
    for (const auto& legend : legends)
        if (legend.text.expanded (legendHitMargin).contains (p))
            return legend.index;
    return -1;
}

void RotarySelector::mouseMove (const juce::MouseEvent& e)
{
    const auto index = legendIndexAt (e.position);
    if (index == hovered)
        return;
    hovered = index;
    setMouseCursor (index >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    repaint();
}

void RotarySelector::mouseExit (const juce::MouseEvent&)
{
    if (hovered < 0)
        return;
    hovered = -1;
    repaint();
}

void RotarySelector::mouseUp (const juce::MouseEvent& e)
{
    const auto index = legendIndexAt (e.position);
    if (index >= 0 && index != selected && knob.isEnabled())
        knob.setValue (index, juce::sendNotificationSync);
}

void RotarySelector::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (! knob.isEnabled() || wheel.isInertial)
        return;

    const auto delta = (std::abs (wheel.deltaX) > std::abs (wheel.deltaY) ? -wheel.deltaX : wheel.deltaY)
                     * (wheel.isReversed ? -1.0f : 1.0f);
    if (wheelAccumulator * delta < 0.0f)
        wheelAccumulator = 0.0f; // direction changed
    wheelAccumulator += delta;

    const auto steps = (int) (wheelAccumulator / wheelStep);
    if (steps == 0)
        return;

    wheelAccumulator -= (float) steps * wheelStep;
    const auto from = juce::jmax (0, selected);
    knob.setValue (juce::jlimit (0, items.size() - 1, from + steps), juce::sendNotificationSync);
}

bool RotarySelector::hitTest (int x, int y)
{
    return knob.getBounds().contains (x, y) || legendIndexAt ({ (float) x, (float) y }) >= 0;
}

void RotarySelector::knobMoved()
{
    const auto index = juce::roundToInt (knob.getValue());
    if (index == selected || ! juce::isPositiveAndBelow (index, items.size()))
        return;

    selected = index;
    scrollWindowTo (index);
    layoutLegends();
    repaint();

    if (onSelect != nullptr)
        onSelect (index);
}
