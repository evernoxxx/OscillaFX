#pragma once

#include "core/StarterProfiles.h"
#include "ui/PanelWidgets.h"
#include <array>

// Non-modal card offering starter sounds for a newly connected output device. It never takes focus
// by itself and never blocks the audio; keyboard users reach it first in the Tab order while it is up.
class SuggestionCard : public juce::Component
{
public:
    static constexpr size_t maxOptions = 5;

    SuggestionCard();

    void showFor (const ProfileSuggestion&);   // fills the card and makes it visible
    void hideCard();
    void setNotchX (float x) { notchX = x; repaint(); }   // where the pointer notch sits (own coordinates)

    size_t numOptions() const { return optionIds.size(); }
    ChoiceButton& optionButton (size_t index) { return options[index]; }
    juce::String optionDescription (size_t index) const { return optionDescriptions[index]; }
    ChoiceButton& keepButton() { return keep; }
    ChoiceButton& laterButton() { return later; }

    std::function<void (const juce::String& starterId)> onChoose;
    std::function<void (bool dontAskAgain)> onDismiss;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    std::array<ChoiceButton, maxOptions> options { ChoiceButton ("Flat"), ChoiceButton ("Warm"), ChoiceButton ("Bass-heavy"),
                                                    ChoiceButton ("Voice"), ChoiceButton ("Wide") };
    ChoiceButton keep { "Keep my settings" }, later { "Not now" };
    juce::StringArray optionIds, optionDescriptions;
    juce::String heading;
    float notchX = 100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SuggestionCard)
};
