#pragma once

// Values behind the per-device starter profiles. Plain C++ (no JUCE) so engine/tests/preset_tuning.cpp
// measures exactly what the app ships. core/StarterProfiles.cpp turns these into DspSettings.
//
// A starter is a "family voicing" (what the output device needs: the Flat starter) plus a flavour
// delta (what the listener wants: Warm, Bass-heavy, Voice, Wide), scaled per family so small
// speakers and Bluetooth links are not pushed into distortion or clipping.
// EQ bands sit at DspSettings::eqFreqHz defaults: 62.5, 116, 214, 397, 735, 1361, 2520, 4666, 8640, 16000 Hz.
// Effects: Fidelity, Ambience, Surround, Dynamic Boost, Bass (0..10).
#include <algorithm>
#include <array>

namespace StarterData
{
constexpr int numEffects = 5;
constexpr int numBands = 10;
constexpr int numFlavors = 5;

enum class Family { builtInSpeakers, headphones, bluetooth, airPods, display, generic };
enum class Flavor { flat, warm, bassHeavy, voice, wide };

struct Voicing
{
    std::array<float, numEffects> effects;
    std::array<float, numBands> eqDb;
};

struct FlavorInfo { const char* id; const char* name; const char* description; };

constexpr FlavorInfo flavorInfo (Flavor f)
{
    switch (f)
    {
        case Flavor::flat:      return { "flat", "Flat", "" }; // description depends on the family
        case Flavor::warm:      return { "warm", "Warm", "Fuller low end, softer highs. Easy on the ears." };
        case Flavor::bassHeavy: return { "bass-heavy", "Bass-heavy", "Strong, tight bass for music and games." };
        case Flavor::voice:     return { "voice", "Voice", "Clear dialogue and vocals for podcasts and video calls." };
        case Flavor::wide:      return { "wide", "Wide", "Roomier stereo image with the bass kept centred." };
    }
    return { "flat", "Flat", "" };
}

constexpr const char* flatDescription (Family f)
{
    switch (f)
    {
        case Family::builtInSpeakers: return "Gentle low-cut protection and extra presence for small speakers.";
        case Family::headphones:      return "Slight bass lift and a little air. Natural for headphones.";
        case Family::bluetooth:       return "Close to neutral, with headroom kept for Bluetooth.";
        case Family::airPods:         return "Nearly untouched: AirPods are already tuned.";
        case Family::display:         return "Adds body and clarity to thin monitor or TV speakers.";
        case Family::generic:         return "Neutral starting point.";
    }
    return "";
}

// What the output needs, before any flavour.
// Measured behaviour of the legacy DSP shapes these: Fidelity is a high-frequency exciter (+1.8 dB air per
// unit), Dynamic Boost adds ~1.3 dB loudness per unit, the maximizer holds peaks at -0.3 dBFS.
constexpr Voicing familyVoicing (Family f)
{
    switch (f)
    {
        case Family::builtInSpeakers:
            return { { 0.5f, 1.0f, 0.5f, 1.0f, 0.0f }, { -6.0f, -3.0f, -1.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.5f, 0.5f, 0.5f } };
        case Family::headphones:
            return { { 0.5f, 0.5f, 0.5f, 1.0f, 1.5f }, { 1.5f, 1.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 1.5f, 2.0f } };
        case Family::bluetooth:
            return { { 0.5f, 0.5f, 0.5f, 1.0f, 0.5f }, { 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.5f } };
        case Family::airPods:
            return { { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f }, { 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f } };
        case Family::display:
            return { { 1.0f, 1.0f, 0.5f, 1.0f, 1.5f }, { 2.0f, 1.5f, 0.5f, -1.0f, 0.0f, 0.5f, 1.5f, 1.0f, 0.5f, 0.5f } };
        case Family::generic:
            return { { 0.5f, 0.5f, 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } };
    }
    return {};
}

// What the listener wants, added to the family voicing (flat = no change).
constexpr Voicing flavorDelta (Flavor f)
{
    switch (f)
    {
        case Flavor::flat:
            return {};
        case Flavor::warm:
            return { { -1.0f, 0.0f, 0.0f, 0.0f, 0.5f }, { 2.0f, 2.0f, 1.5f, 1.0f, 0.0f, 0.0f, -1.0f, -2.0f, -2.5f, -2.5f } };
        case Flavor::bassHeavy:
            return { { -0.5f, 0.0f, 0.0f, 0.0f, 2.0f }, { 3.5f, 3.0f, 1.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } };
        case Flavor::voice:
            return { { 0.0f, -0.5f, -0.5f, 0.5f, -0.5f }, { -2.0f, -1.5f, -1.0f, -1.0f, 0.0f, 1.5f, 2.5f, 1.5f, -1.0f, 0.0f } };
        case Flavor::wide:
            return { { 0.0f, 2.0f, 2.5f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f } };
    }
    return {};
}

// How much of a flavour the family can take: small drivers and lossy links get less.
constexpr float flavorScale (Family f, Flavor flavor)
{
    switch (f)
    {
        case Family::builtInSpeakers: return flavor == Flavor::bassHeavy ? 0.6f : flavor == Flavor::wide ? 0.8f : 0.9f;
        case Family::bluetooth:       return 0.8f;
        case Family::airPods:         return 0.8f;
        case Family::headphones:
        case Family::display:
        case Family::generic:         break;
    }
    return 1.0f;
}

constexpr float EQ_LIMIT_DB = 12.0f;
constexpr float EFFECT_LIMIT = 10.0f;

inline Voicing make (Family family, Flavor flavor)
{
    Voicing v = familyVoicing (family);
    const Voicing d = flavorDelta (flavor);
    const float scale = flavorScale (family, flavor);
    for (size_t i = 0; i < (size_t) numEffects; ++i)
        v.effects[i] = std::clamp (v.effects[i] + scale * d.effects[i], 0.0f, EFFECT_LIMIT);
    for (size_t i = 0; i < (size_t) numBands; ++i)
        v.eqDb[i] = std::clamp (v.eqDb[i] + scale * d.eqDb[i], -EQ_LIMIT_DB, EQ_LIMIT_DB);
    return v;
}
} // namespace StarterData
