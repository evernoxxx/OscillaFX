#pragma once

#include "StarterProfiles.h"
#include "profiles/ProfileStore.h"

// Decides when the active output device deserves a starter-profile suggestion and remembers the
// answer. No audio, no UI: Controller feeds it device changes and relays the user's choice.
// A suggestion is pending for a device whose profile has not been "handled" (see ProfileStore) and
// that was not dismissed in this session. Nothing here ever changes settings by itself.
class SuggestionTracker
{
public:
    explicit SuggestionTracker (ProfileStore& store) : profiles (store) {}

    // The device became active (startup, plug-in, switch). Returns true if pending() changed.
    bool deviceActivated (const DeviceKinds::Traits&);

    const std::optional<ProfileSuggestion>& pending() const { return suggestion; }

    // Settings with the chosen starter applied on top of `current`; the device is then handled and
    // its profile saved. nullopt (nothing changes) if no suggestion is pending or the id is unknown.
    std::optional<DspSettings> apply (const juce::String& starterId, const DspSettings& current);

    // dontAskAgain: remembered for this device for good. Otherwise only until the app quits.
    // Returns true if a suggestion was pending.
    bool dismiss (bool dontAskAgain, const DspSettings& current);

    // The user changed settings by hand on a device that had a suggestion: they took control.
    // Returns true if a suggestion was pending.
    bool userTookControl (const DspSettings& current);

private:
    bool clear();

    ProfileStore& profiles;
    std::optional<ProfileSuggestion> suggestion;
    DeviceKind kind = DeviceKind::unknown;
    juce::StringArray dismissedThisSession;
};
