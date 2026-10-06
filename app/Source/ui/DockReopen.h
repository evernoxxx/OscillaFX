#pragma once

#include <functional>

// Clicking the Dock icon (or launching the app again from Finder) while the window is hidden
// calls `onReopen`. JUCE's app delegate does not handle applicationShouldHandleReopen, so the
// method is added to it at runtime. Pass nullptr to disconnect.
namespace DockReopen
{
    void setHandler (std::function<void()> onReopen);
}
