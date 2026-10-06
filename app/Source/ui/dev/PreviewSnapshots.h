#pragma once

#include "core/Controller.h"
#include <juce_gui_basics/juce_gui_basics.h>

// UI preview builds only (OSCILLA_UI_STUB): when OSCILLA_SNAPSHOT_DIR is set, steps through the
// display modes, writes 1x and 2x PNGs of the window content there (plus crt-*.png: the tube face
// read back from the GPU), then quits.
// OSCILLA_DEV_RENDER_OCCLUDED keeps the CRT animating while the window is occluded (CPU profiling).
// OSCILLA_DEV_CLOSE_WINDOW closes (hides) the window shortly after launch, for testing reopen.
namespace PreviewSnapshots
{
    void scheduleIfRequested (Controller&, juce::Component& content);
}
