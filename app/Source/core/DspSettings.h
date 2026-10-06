#pragma once

#include <array>
#include <juce_core/juce_core.h>

// Everything the user can change for one output device. Serialised per device by ProfileStore.
struct DspSettings
{
    static constexpr int numEqBands = 10;

    bool power = true;
    juce::String presetName = "Default";

    // Effect amounts, 0..10 (FxSound UI scale).
    float fidelity = 2.0f;
    float ambience = 1.0f;
    float surround = 1.0f;
    float dynamicBoost = 2.0f;
    float bass = 3.0f;

    std::array<float, numEqBands> eqGainDb {}; // -12..+12 dB
    // Band centre frequencies. FxSound presets move these, so they travel with the profile.
    std::array<float, numEqBands> eqFreqHz { 62.5f, 115.734f, 214.311f, 396.85f, 734.867f,
                                             1360.79f, 2519.84f, 4666.12f, 8640.48f, 16000.0f };
    float masterGainDb = 0.0f;
    float balance = 0.0f; // -1 (left) .. +1 (right)

    // Listening safety. Both default to off so profiles saved by older versions load unchanged,
    // and neither is part of a preset or a starter profile: loading those keeps the user's choice.
    // nightMode: gentle compressor (-24 dBFS, 3:1) with a -1 dBFS safety ceiling, more Dynamic
    //            Boost, capped bass. The stored effect/EQ values are not touched; the audio
    //            thread derives the effective ones (see DspParams::fromSettings). Inactive while power is off.
    // limiterCeilingDb: volume safety limiter, -24..0 dBFS at the very end of the chain; >= -0.05 = off.
    //            Stays active when power is off (it is a safety net, not an effect).
    static constexpr float limiterMinDb = -24.0f;
    bool nightMode = false;
    float limiterCeilingDb = 0.0f;
    bool limiterEnabled() const { return limiterCeilingDb < -0.05f; }

    // CRT state. Not DSP, but kept per device so the panel looks the way it was left.
    static constexpr int numDisplayModes = 4;
    int displayMode = 0;       // 0 spectrum, 1 waveform, 2 XY, 3 phosphor gallery
    bool eqEditMode = false;   // CRT shows the draggable EQ instead of the visualiser
    int galleryIndex = 0;      // image shown in gallery mode

    void clampToRanges();   // pulls every value into the range fromVar() would accept (NaN -> default)

    juce::var toVar() const;
    static DspSettings fromVar (const juce::var&);
    bool operator== (const DspSettings&) const;
    bool operator!= (const DspSettings& o) const { return ! (*this == o); }
};
