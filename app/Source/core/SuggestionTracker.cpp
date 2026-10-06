#include "SuggestionTracker.h"

bool SuggestionTracker::deviceActivated (const DeviceKinds::Traits& traits)
{
    const auto before = suggestion;
    suggestion.reset();
    if (traits.uid.isNotEmpty() && ! profiles.suggestionHandled (traits.uid) && ! dismissedThisSession.contains (traits.uid))
    {
        kind = DeviceKinds::classify (traits);
        suggestion = ProfileSuggestion { traits.uid, traits.name, DeviceKinds::id (kind), DeviceKinds::label (kind),
                                         StarterProfiles::optionsFor (kind) };
    }
    const bool hadOne = before.has_value(), hasOne = suggestion.has_value();
    return hadOne != hasOne || (hasOne && before->deviceUid != suggestion->deviceUid);
}

std::optional<DspSettings> SuggestionTracker::apply (const juce::String& starterId, const DspSettings& current)
{
    if (! suggestion)
        return std::nullopt;
    const auto starter = StarterProfiles::find (kind, starterId);
    if (! starter)
        return std::nullopt;
    const auto result = StarterProfiles::applyTo (current, *starter);
    profiles.markSuggestionHandled (suggestion->deviceUid, result);
    clear();
    return result;
}

bool SuggestionTracker::dismiss (bool dontAskAgain, const DspSettings& current)
{
    if (! suggestion)
        return false;
    if (dontAskAgain)
        profiles.markSuggestionHandled (suggestion->deviceUid, current);
    else
        dismissedThisSession.addIfNotAlreadyThere (suggestion->deviceUid);
    return clear();
}

bool SuggestionTracker::userTookControl (const DspSettings& current)
{
    if (! suggestion)
        return false;
    profiles.markSuggestionHandled (suggestion->deviceUid, current);
    return clear();
}

bool SuggestionTracker::clear()
{
    const bool had = suggestion.has_value();
    suggestion.reset();
    return had;
}
