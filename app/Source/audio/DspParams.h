#pragma once

#include "core/DspSettings.h"
#include <array>

class DfxDsp;

// Plain-old-data snapshot of the DSP-relevant part of DspSettings, safe to hand to the audio thread.
struct DspParams
{
    static constexpr int numEffects = 5;
    static constexpr int numEqBands = DspSettings::numEqBands;

    bool power = true;
    std::array<float, numEffects> effects {};   // 0..10, DfxDsp::Effect order
    std::array<float, numEqBands> eqGainDb {};
    std::array<float, numEqBands> eqFreqHz {};
    float masterGainDb = 0.0f;
    float balanceDb = 0.0f;                     // DfxDsp convention: +dB attenuates left
    bool nightMode = false;                     // handled by Dynamics.h after DfxDsp, not by DfxDsp
    float limiterCeilingDb = 0.0f;              // >= -0.05 = limiter off

    // Night mode is folded in here: Dynamic Boost raised, Bass and low EQ boost capped (see the
    // NIGHT_* constants in DspParams.cpp), so the stored settings stay the user's own.
    static DspParams fromSettings (const DspSettings&);
};

namespace DspParamsApply
{
// FxSound runs the graphic EQ with 10 bands; call once on a fresh DfxDsp.
void configureBands (DfxDsp&);

// Pushes every value into the DSP.
void applyAll (DfxDsp&, const DspParams&);

// Pushes only values that differ from `previous` (keeps audio-thread work minimal).
void applyChanges (DfxDsp&, const DspParams& next, const DspParams& previous);

// Inverse mapping, used to read a loaded .fac preset back into settings.
void readInto (DfxDsp&, DspSettings&);
} // namespace DspParamsApply
