#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Native window touches that JUCE does not expose (macOS).
namespace WindowChrome
{
    // Lets the panel fill the whole window: no title-bar strip, hidden title, and the traffic-light
    // buttons float over the top-left corner of the content. Safe to call again after the peer is recreated.
    void useFullSizeContent (juce::ComponentPeer&);

    // A light trackpad tap, for faders that snap to their detent. Does nothing without a haptic trackpad.
    void detentHaptic();
}
