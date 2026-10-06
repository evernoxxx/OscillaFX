#pragma once

#include "ui/Knob.h"

// Detented rotary switch with engraved position legends around it. Turn the knob, click a legend,
// scroll, or use the arrow keys. When there are more items than slots, the legends scroll as a
// window that always contains the selected item.
class RotarySelector : public juce::Component
{
public:
    struct Geometry
    {
        float knobRadius;
        float dotRadius;        // legend dots sit on this circle
        int maxSlots;
        float slotStepRadians;
        float maxLegendWidth;   // longer legends are shortened with an ellipsis
        float legendSize;
    };

    RotarySelector (const juce::String& title, Geometry);

    void placeAt (juce::Point<float> centre);
    void setItems (const juce::StringArray& names, int selectedIndex);
    void setSelectedIndex (int index);   // -1 = none of the items (e.g. a modified preset)
    int getSelectedIndex() const { return selected; }
    int getNumItems() const { return items.size(); }

    void describe (const juce::String&);
    void setReadout (ValueReadout*);

    // Short panel legend for an item; the full item text is announced and shown in the read-out.
    std::function<juce::String (const juce::String& item)> legendForItem;

    std::function<void (int)> onSelect;

    Knob& getKnob() { return knob; }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool hitTest (int x, int y) override;

private:
    struct Legend
    {
        int index;
        juce::Point<float> dot;
        juce::Rectangle<float> text;
    };

    juce::String legendText (int index) const;
    juce::Point<float> centre() const;
    float slotAngle (int slot) const;
    float angleForIndex (int index) const;
    int legendIndexAt (juce::Point<float>) const;
    void scrollWindowTo (int index);
    void layoutLegends();
    void knobMoved();

    const Geometry geometry;
    Knob knob;
    juce::StringArray items;
    int selected = -1;
    int hovered = -1;
    int windowStart = 0;
    float wheelAccumulator = 0.0f;
    std::vector<Legend> legends;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotarySelector)
};
