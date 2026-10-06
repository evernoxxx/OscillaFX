#pragma once

#include <juce_core/juce_core.h>

namespace AppPaths
{
// ~/Library/Application Support/Oscilla, or $OSCILLA_DATA_DIR when set. The override keeps preview
// builds, screenshots and experiments away from the real profiles, prefs and gallery.
inline juce::File dataDirectory()
{
    const auto overridePath = juce::SystemStats::getEnvironmentVariable ("OSCILLA_DATA_DIR", {});
    if (overridePath.isNotEmpty() && juce::File::isAbsolutePath (overridePath))
        return juce::File (overridePath);
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Application Support/Oscilla");
}
} // namespace AppPaths
