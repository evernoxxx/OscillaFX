#pragma once

#include "DeviceKind.h"
#include "DspSettings.h"
#include <optional>

// One selectable starting point for a device that has no profile yet.
struct StarterProfile
{
    juce::String id;           // "flat" | "warm" | "bass-heavy" | "voice" | "wide"
    juce::String name;         // "Flat" | "Warm" | "Bass-heavy" | "Voice" | "Wide"
    juce::String description;  // one line, for the UI
    DspSettings settings;      // complete settings; effects + EQ are what matter, see StarterProfiles::applyTo
};

// What Controller::pendingSuggestion() reports.
struct ProfileSuggestion
{
    juce::String deviceUid, deviceName;
    juce::String deviceKind;       // DeviceKinds::id(): "builtin-speakers", "headphones", "airpods", ...
    juce::String deviceKindLabel;  // for display: "Built-in speakers", "AirPods", ...
    juce::Array<StarterProfile> options; // always the five starters, Flat first
};

namespace StarterProfiles
{
// "Flat" is the device-appropriate neutral voicing (e.g. low-cut protection on laptop speakers),
// not a bypass. The other four add a character on top. Values are measured in engine/tests/preset_tuning.cpp.
juce::Array<StarterProfile> optionsFor (DeviceKind);
std::optional<StarterProfile> find (DeviceKind, const juce::String& starterId);

// `base` with the starter's effects, EQ and preset name. Power, master gain, balance, night mode,
// limiter and display state stay as they are (same rule as Presets::load).
DspSettings applyTo (const DspSettings& base, const StarterProfile&);
} // namespace StarterProfiles
