#pragma once

#include "ui/PanelWidgets.h"
#include <vector>

// First-run tour: dims the panel, rings one control and explains it in a short card. Three steps,
// always skippable (Skip button, Escape). Does not touch the audio or any setting.
class TourOverlay : public juce::Component
{
public:
    struct Step
    {
        juce::String title, body;
        juce::Rectangle<float> target;   // in this component's coordinates
    };

    TourOverlay();

    void start (std::vector<Step>);
    void setAvoidArea (juce::Rectangle<float> area) { avoid = area; }   // e.g. the CRT: a native layer that draws over the card
    std::function<void()> onFinished;   // after the last step or Skip

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    juce::Rectangle<float> cardArea() const;
    void showStep (size_t index);
    void advance();
    void finish();

    std::vector<Step> steps;
    size_t current = 0;
    juce::Rectangle<float> avoid;
    ChoiceButton skip { "Skip" }, next { "Next" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TourOverlay)
};
