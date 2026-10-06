#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>

// Shows a one-line help text (on the CRT) while the mouse is over a control, and clears it on exit.
// Declare it before the components it watches so it outlives them.
class HoverHints : private juce::MouseListener
{
public:
    explicit HoverHints (std::function<void (const juce::String&)> showHint) : show (std::move (showHint)) {}

    void attach (juce::Component& component, const juce::String& hint)
    {
        hints[&component] = hint;
        component.addMouseListener (this, false);
    }

    void setHint (juce::Component& component, const juce::String& hint)
    {
        hints[&component] = hint;
    }

private:
    void mouseEnter (const juce::MouseEvent& e) override
    {
        if (const auto found = hints.find (e.eventComponent); found != hints.end())
            show (found->second);
    }

    void mouseExit (const juce::MouseEvent&) override { show ({}); }

    std::function<void (const juce::String&)> show;
    std::map<juce::Component*, juce::String> hints;
};
