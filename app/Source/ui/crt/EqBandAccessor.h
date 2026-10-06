#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// One EQ band of the CRT's equaliser exposed to accessibility clients (VoiceOver) as a slider:
// an invisible child laid over the band's column. Mouse and keyboard stay with the CRT itself.
class EqBandAccessor : public juce::Component
{
public:
    struct Range
    {
        double min, max, step;
    };

    EqBandAccessor (Range, std::function<float()> getGain, std::function<void (float)> setGain);

    void gainChanged();   // tells accessibility clients the value moved

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    Range range;
    std::function<float()> getGain;
    std::function<void (float)> setGain;
};
