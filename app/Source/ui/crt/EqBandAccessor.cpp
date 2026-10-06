#include "ui/crt/EqBandAccessor.h"

namespace
{
    class GainValue final : public juce::AccessibilityValueInterface
    {
    public:
        GainValue (EqBandAccessor::Range r, std::function<float()> get, std::function<void (float)> set)
            : range (r), getGain (std::move (get)), setGain (std::move (set)) {}

        bool isReadOnly() const override { return false; }
        double getCurrentValue() const override { return getGain(); }

        juce::String getCurrentValueAsString() const override
        {
            const auto db = getGain();
            return (db > 0.0f ? "+" : "") + juce::String (db, 1) + " dB";
        }

        void setValue (double newValue) override
        {
            const auto snapped = std::round (newValue / range.step) * range.step;
            setGain ((float) juce::jlimit (range.min, range.max, snapped));
        }

        void setValueAsString (const juce::String& text) override { setValue (text.retainCharacters ("+-.0123456789").getDoubleValue()); }
        AccessibleValueRange getRange() const override { return { { range.min, range.max }, range.step }; }

    private:
        EqBandAccessor::Range range;
        std::function<float()> getGain;
        std::function<void (float)> setGain;
    };
}

EqBandAccessor::EqBandAccessor (Range r, std::function<float()> get, std::function<void (float)> set)
    : range (r), getGain (std::move (get)), setGain (std::move (set))
{
    setInterceptsMouseClicks (false, false);
    setWantsKeyboardFocus (false);
}

void EqBandAccessor::gainChanged()
{
    if (auto* handler = getAccessibilityHandler())
        handler->notifyAccessibilityEvent (juce::AccessibilityEvent::valueChanged);
}

std::unique_ptr<juce::AccessibilityHandler> EqBandAccessor::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler> (*this, juce::AccessibilityRole::slider, juce::AccessibilityActions {},
                                                         juce::AccessibilityHandler::Interfaces { std::make_unique<GainValue> (range, getGain, setGain) });
}
