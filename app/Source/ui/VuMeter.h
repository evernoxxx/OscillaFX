#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Vu
{
    // 0 VU sits at -14 dBFS (RMS of a sine), leaving 14 dB of headroom before full scale: loud music
    // reaches the red, as on a real deck, without pinning the needle all the time.
    constexpr float referenceDbfs = -14.0f;
    constexpr float averageToDeflection = 0.7079f * 1.1107f / 0.19953f;   // mean |x| -> needle deflection, 1.0 = +3 VU
    constexpr float integrationSeconds = 0.3f;                           // time for the needle to reach 99 % of a step
    constexpr float peakThreshold = 0.95f;                               // sample peak that lights the lamp
    constexpr float peakHoldSeconds = 0.6f;

    // The classic meter movement: the needle follows the rectified average with a ~300 ms rise,
    // modelled as a single pole (99 % after `integrationSeconds`).
    class Ballistics
    {
    public:
        float step (float meanAbsolute, float seconds);
        float deflection() const { return needle; }
        void reset() { needle = 0.0f; }

    private:
        float needle = 0.0f;
    };
}

// Analog VU meter: cream dial in a black housing, needle with real VU ballistics, red zone above
// 0 VU and a peak lamp. The dial is drawn once and cached; only the needle and lamp move.
class VuMeter : public juce::Component
{
public:
    explicit VuMeter (const juce::String& channelLegend);   // "LEFT" / "RIGHT"
    ~VuMeter() override;

    // `deflection` 0..~1.05 of the scale (see Vu::Ballistics); `isPeakLit` for the lamp.
    void setReading (float deflection, bool isPeakLit);

    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    class Dial;
    std::unique_ptr<Dial> dial;
    float needle = 0.0f;
    bool peakLit = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VuMeter)
};
