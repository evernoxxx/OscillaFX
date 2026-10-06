#include "ui/TourOverlay.h"
#include "ui/Theme.h"

namespace
{
    constexpr float cardWidth = 380.0f, cardHeight = 126.0f, cardPad = 16.0f, ringMargin = 10.0f, edgeMargin = 16.0f;
    constexpr float buttonHeight = 30.0f;
}

TourOverlay::TourOverlay()
{
    setTitle ("Welcome tour");
    setWantsKeyboardFocus (true);
    setAlwaysOnTop (true);
    setVisible (false);
    addAndMakeVisible (skip);
    addAndMakeVisible (next);
    skip.setTooltip ("Skip the tour");
    skip.setHelpText ("Skip the tour");
    skip.onClick = [this] { finish(); };
    next.onClick = [this] { advance(); };
}

void TourOverlay::start (std::vector<Step> newSteps)
{
    steps = std::move (newSteps);
    if (steps.empty())
        return;
    setVisible (true);
    toFront (false);
    showStep (0);

    // The panel may not be on screen yet (or the window is hidden in the menu bar): take focus once it is.
    juce::Component::SafePointer<TourOverlay> safe (this);
    juce::Timer::callAfterDelay (300, [safe]
    {
        if (safe != nullptr && safe->isVisible() && safe->isShowing())
            safe->grabKeyboardFocus();
    });
}

void TourOverlay::showStep (size_t index)
{
    current = index;
    const auto isLast = current + 1 == steps.size();
    next.setButtonText (isLast ? "Done" : "Next");
    next.setTitle (isLast ? "Done" : "Next");
    skip.setVisible (! isLast);
    setDescription (juce::String ((int) current + 1) + " of " + juce::String ((int) steps.size()) + ". " + steps[current].title + ". " + steps[current].body);
    resized();
    repaint();
}

void TourOverlay::advance()
{
    if (current + 1 >= steps.size())
        finish();
    else
        showStep (current + 1);
}

void TourOverlay::finish()
{
    setVisible (false);
    if (onFinished != nullptr)
        onFinished();
}

bool TourOverlay::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)  { finish();  return true; }
    if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey || key == juce::KeyPress::rightKey)  { advance();  return true; }
    return true; // a tour in progress swallows other keys
}

juce::Rectangle<float> TourOverlay::cardArea() const
{
    if (steps.empty())
        return {};

    const auto target = steps[current].target;
    const auto bounds = getLocalBounds().toFloat();
    const auto gap = ringMargin + 14.0f;
    const auto clampX = [&] (float x) { return juce::jlimit (edgeMargin, bounds.getWidth() - edgeMargin - cardWidth, x); };
    const auto clampY = [&] (float y) { return juce::jlimit (edgeMargin, bounds.getHeight() - edgeMargin - cardHeight, y); };
    const juce::Rectangle<float> candidates[] = {
        { clampX (target.getCentreX() - cardWidth * 0.5f), target.getBottom() + gap, cardWidth, cardHeight },
        { clampX (target.getCentreX() - cardWidth * 0.5f), target.getY() - gap - cardHeight, cardWidth, cardHeight },
        { target.getRight() + gap, clampY (target.getCentreY() - cardHeight * 0.5f), cardWidth, cardHeight },
        { target.getX() - gap - cardWidth, clampY (target.getCentreY() - cardHeight * 0.5f), cardWidth, cardHeight } };
    for (const auto& c : candidates)
        if (bounds.contains (c) && ! c.intersects (avoid) && ! c.intersects (target.expanded (ringMargin)))
            return c;
    return candidates[0];
}

void TourOverlay::resized()
{
    const auto card = cardArea().reduced (cardPad);
    next.setBounds (juce::Rectangle<float> (card.getRight() - 96.0f, card.getBottom() - buttonHeight, 96.0f, buttonHeight).toNearestInt());
    skip.setBounds (juce::Rectangle<float> (card.getRight() - 96.0f - 88.0f, card.getBottom() - buttonHeight, 80.0f, buttonHeight).toNearestInt());
}

void TourOverlay::paint (juce::Graphics& g)
{
    if (steps.empty())
        return;

    const auto& step = steps[current];
    const auto hole = step.target.expanded (ringMargin);

    juce::Path dim;
    dim.addRectangle (getLocalBounds().toFloat());
    dim.addRoundedRectangle (hole, 12.0f);
    dim.setUsingNonZeroWinding (false);
    g.setColour (juce::Colours::black.withAlpha (0.66f));
    g.fillPath (dim);
    g.setColour (Theme::glassTeal.withAlpha (0.9f));
    g.drawRoundedRectangle (hole, 12.0f, 2.5f);

    const auto card = cardArea();
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (card.translated (2.0f, 5.0f), 12.0f);
    g.setColour (Theme::darkGlass);
    g.fillRoundedRectangle (card, 12.0f);
    g.setColour (Theme::glassTeal.withAlpha (0.6f));
    g.drawRoundedRectangle (card.reduced (0.5f), 12.0f, 1.2f);

    auto text = card.reduced (cardPad);
    g.setColour (Theme::glassTeal.withAlpha (0.7f));
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText ("STEP " + juce::String ((int) current + 1) + " OF " + juce::String ((int) steps.size()), text.removeFromTop (16.0f), juce::Justification::centredLeft, false);
    g.setColour (Theme::glassTealHot);
    g.setFont (juce::Font (juce::FontOptions (17.0f, juce::Font::bold)));
    g.drawText (step.title, text.removeFromTop (24.0f), juce::Justification::centredLeft, true);
    g.setColour (Theme::glassTealHot.withAlpha (0.9f));
    g.setFont (juce::Font (juce::FontOptions (13.5f)));
    g.drawFittedText (step.body, text.withTrimmedBottom (buttonHeight + 6.0f).toNearestInt(), juce::Justification::topLeft, 4, 1.0f);
}
