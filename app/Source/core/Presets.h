#pragma once

#include "DspSettings.h"
#include <optional>

// Bundled .fac presets (Oscilla's own plus FxSound's). Message thread only.
namespace Presets
{
// Existing search dirs, first match wins: app bundle Contents/Resources/Presets, then the
// source tree's presets/, then the FxSound checkout next to it (dev runs).
juce::Array<juce::File> directories();

juce::StringArray names(); // file stems, unique, sorted; Movies and Music first

// Effects and EQ from the preset; power, gain, balance and display mode kept from `base`.
std::optional<DspSettings> load (const juce::String& name, const DspSettings& base);
} // namespace Presets
