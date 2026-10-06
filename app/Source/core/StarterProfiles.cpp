#include "StarterProfiles.h"
#include "StarterProfileData.h"

namespace
{
using StarterData::Family;
using StarterData::Flavor;

constexpr Flavor FLAVORS[] = { Flavor::flat, Flavor::warm, Flavor::bassHeavy, Flavor::voice, Flavor::wide };

Family familyFor (DeviceKind kind)
{
    switch (kind)
    {
        case DeviceKind::builtInSpeakers:     return Family::builtInSpeakers;
        case DeviceKind::headphonesWired:
        case DeviceKind::headphonesUsb:       return Family::headphones;
        case DeviceKind::headphonesBluetooth: return Family::bluetooth;
        case DeviceKind::airPods:             return Family::airPods;
        case DeviceKind::display:             return Family::display;
        case DeviceKind::externalSpeakers:    return Family::bluetooth; // unknown drivers: stay conservative
        case DeviceKind::unknown:             break;
    }
    return Family::generic;
}

StarterProfile build (Family family, Flavor flavor)
{
    const auto info = StarterData::flavorInfo (flavor);
    const auto voicing = StarterData::make (family, flavor);

    StarterProfile starter;
    starter.id = info.id;
    starter.name = info.name;
    starter.description = flavor == Flavor::flat ? StarterData::flatDescription (family) : info.description;
    auto& s = starter.settings;
    s.presetName = info.name;
    s.fidelity = voicing.effects[0];
    s.ambience = voicing.effects[1];
    s.surround = voicing.effects[2];
    s.dynamicBoost = voicing.effects[3];
    s.bass = voicing.effects[4];
    for (size_t b = 0; b < (size_t) DspSettings::numEqBands; ++b)
        s.eqGainDb[b] = voicing.eqDb[b];
    return starter;
}
} // namespace

namespace StarterProfiles
{
juce::Array<StarterProfile> optionsFor (DeviceKind kind)
{
    juce::Array<StarterProfile> options;
    for (auto flavor : FLAVORS)
        options.add (build (familyFor (kind), flavor));
    return options;
}

std::optional<StarterProfile> find (DeviceKind kind, const juce::String& starterId)
{
    for (auto& option : optionsFor (kind))
        if (option.id == starterId)
            return option;
    return std::nullopt;
}

DspSettings applyTo (const DspSettings& base, const StarterProfile& starter)
{
    DspSettings result = base;
    const auto& s = starter.settings;
    result.presetName = s.presetName;
    result.fidelity = s.fidelity;
    result.ambience = s.ambience;
    result.surround = s.surround;
    result.dynamicBoost = s.dynamicBoost;
    result.bass = s.bass;
    result.eqGainDb = s.eqGainDb;
    result.eqFreqHz = s.eqFreqHz;
    return result;
}
} // namespace StarterProfiles
