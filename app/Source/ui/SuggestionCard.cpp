#include "ui/SuggestionCard.h"
#include "ui/Theme.h"

namespace
{
    constexpr float pad = 14.0f, gap = 8.0f, rowHeight = 30.0f, notch = 9.0f;
}

SuggestionCard::SuggestionCard()
{
    setTitle ("New output device");
    setFocusContainerType (FocusContainerType::focusContainer);
    for (size_t i = 0; i < maxOptions; ++i)
    {
        addChildComponent (options[i]);
        options[i].onClick = [this, i] { if (onChoose != nullptr) onChoose (optionIds[(int) i]); };
    }
    addAndMakeVisible (keep);
    addAndMakeVisible (later);
    keep.onClick = [this] { if (onDismiss != nullptr) onDismiss (true); };
    later.onClick = [this] { if (onDismiss != nullptr) onDismiss (false); };
    keep.setTooltip ("Keep my settings - leave this device as it is and do not ask again");
    later.setTooltip ("Not now - close this; it may come back next time you start OscillaFX");
    keep.setHelpText (keep.getTooltip());
    later.setHelpText (later.getTooltip());
    setVisible (false);
    setAlwaysOnTop (true);
}

void SuggestionCard::showFor (const ProfileSuggestion& suggestion)
{
    optionIds.clear();
    optionDescriptions.clear();
    for (size_t i = 0; i < maxOptions; ++i)
    {
        const auto isUsed = (int) i < suggestion.options.size();
        options[i].setVisible (isUsed);
        if (! isUsed)
            continue;
        const auto& starter = suggestion.options.getReference ((int) i);
        optionIds.add (starter.id);
        optionDescriptions.add (starter.name + " - " + starter.description);
        options[i].setButtonText (starter.name);
        options[i].setTitle (starter.name);
        options[i].setTooltip (optionDescriptions[(int) i]);
        options[i].setHelpText (starter.description);
        options[i].setDefaultChoice (i == 0);
    }

    heading = "New device: " + suggestion.deviceName + " (" + suggestion.deviceKindLabel + "). Pick a starting sound:";
    setDescription (heading);
    repaint();
    setVisible (true);
}

void SuggestionCard::hideCard()
{
    setVisible (false);
}

void SuggestionCard::resized()
{
    const auto inner = getLocalBounds().toFloat().reduced (pad);
    const auto buttonWidth = (inner.getWidth() - gap * (float) (maxOptions - 1)) / (float) maxOptions;
    for (size_t i = 0; i < maxOptions; ++i)
        options[i].setBounds (juce::Rectangle<float> (inner.getX() + (buttonWidth + gap) * (float) i, inner.getY() + notch + 36.0f, buttonWidth, rowHeight).toNearestInt());

    const auto second = inner.getY() + notch + 36.0f + rowHeight + 8.0f;
    keep.setBounds (juce::Rectangle<float> (inner.getX(), second, 176.0f, rowHeight - 2.0f).toNearestInt());
    later.setBounds (juce::Rectangle<float> (inner.getX() + 176.0f + gap, second, 110.0f, rowHeight - 2.0f).toNearestInt());
}

void SuggestionCard::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced (3.0f).withTrimmedTop (notch);
    juce::Path shape;
    shape.addRoundedRectangle (area, 10.0f);
    shape.addTriangle (notchX - 9.0f, area.getY() + 0.5f, notchX + 9.0f, area.getY() + 0.5f, notchX, area.getY() - notch + 3.0f);

    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillPath (shape, juce::AffineTransform::translation (2.0f, 4.0f));
    g.setColour (Theme::darkGlass);
    g.fillPath (shape);
    g.setColour (Theme::glassTeal.withAlpha (0.55f));
    g.strokePath (shape, juce::PathStrokeType (1.2f));

    g.setColour (Theme::glassTealHot.withAlpha (0.95f));
    g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
    g.drawFittedText (heading, juce::Rectangle<int> ((int) pad, (int) (area.getY() + 8.0f), getWidth() - (int) pad * 2, 34), juce::Justification::topLeft, 2, 1.0f);
}
