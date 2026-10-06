#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Everything on the instrument that never changes: case, badge, brushed faceplate, CRT bezel,
// engraved boxes, legends and knob scales. Painted once per pixel scale in design-space
// coordinates (Theme::Design) and cached by MainPanel.
namespace PanelArt
{
    void paint (juce::Graphics&);
}
