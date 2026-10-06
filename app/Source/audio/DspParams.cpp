#include "DspParams.h"
#include "DfxDsp.h"

namespace
{
constexpr float BALANCE_RANGE_DB = 20.0f; // FxSound balance slider range is -20..+20 dB
constexpr int FXSOUND_EQ_BANDS = 10;

// Night mode: quiet passages come forward, thumping bass stays down.
constexpr float NIGHT_DYNAMIC_BOOST_MIN = 6.0f;
constexpr float NIGHT_BASS_MAX = 3.0f;
constexpr float NIGHT_LOW_BOOST_MAX_DB = 3.0f;
constexpr float NIGHT_LOW_BAND_HZ = 200.0f;
} // namespace

DspParams DspParams::fromSettings (const DspSettings& s)
{
    DspParams p;
    p.power = s.power;
    p.effects = { s.fidelity, s.ambience, s.surround, s.dynamicBoost, s.bass };
    p.eqGainDb = s.eqGainDb;
    p.eqFreqHz = s.eqFreqHz;
    p.masterGainDb = s.masterGainDb;
    p.balanceDb = s.balance * BALANCE_RANGE_DB;
    p.nightMode = s.nightMode && s.power; // power off = full bypass; only the safety limiter stays on
    p.limiterCeilingDb = s.limiterCeilingDb;
    if (p.nightMode)
    {
        p.effects[DfxDsp::DynamicBoost] = std::max (p.effects[DfxDsp::DynamicBoost], NIGHT_DYNAMIC_BOOST_MIN);
        p.effects[DfxDsp::Bass] = std::min (p.effects[DfxDsp::Bass], NIGHT_BASS_MAX);
        for (size_t b = 0; b < (size_t) numEqBands; ++b)
            if (p.eqFreqHz[b] < NIGHT_LOW_BAND_HZ)
                p.eqGainDb[b] = std::min (p.eqGainDb[b], NIGHT_LOW_BOOST_MAX_DB);
    }
    return p;
}

namespace DspParamsApply
{
void configureBands (DfxDsp& dsp)
{
    if (dsp.getNumEqBands() != FXSOUND_EQ_BANDS)
        dsp.setNumBands (FXSOUND_EQ_BANDS);
}

void applyAll (DfxDsp& dsp, const DspParams& p)
{
    dsp.powerOn (p.power);
    for (int e = 0; e < DspParams::numEffects; ++e)
        dsp.setEffectValue ((DfxDsp::Effect) e, p.effects[(size_t) e]);
    for (int b = 0; b < DspParams::numEqBands; ++b)
    {
        dsp.setEqBandFrequency (b, p.eqFreqHz[(size_t) b]);
        dsp.setEqBandBoostCut (b, p.eqGainDb[(size_t) b]);
    }
    dsp.setMasterGain (p.masterGainDb);
    dsp.setBalance (p.balanceDb);
}

void applyChanges (DfxDsp& dsp, const DspParams& next, const DspParams& prev)
{
    if (next.power != prev.power)
        dsp.powerOn (next.power);
    for (size_t e = 0; e < (size_t) DspParams::numEffects; ++e)
        if (! juce::exactlyEqual (next.effects[e], prev.effects[e]))
            dsp.setEffectValue ((DfxDsp::Effect) e, next.effects[e]);
    for (size_t b = 0; b < (size_t) DspParams::numEqBands; ++b)
    {
        if (! juce::exactlyEqual (next.eqFreqHz[b], prev.eqFreqHz[b]))
            dsp.setEqBandFrequency ((int) b, next.eqFreqHz[b]);
        if (! juce::exactlyEqual (next.eqGainDb[b], prev.eqGainDb[b]))
            dsp.setEqBandBoostCut ((int) b, next.eqGainDb[b]);
    }
    if (! juce::exactlyEqual (next.masterGainDb, prev.masterGainDb))
        dsp.setMasterGain (next.masterGainDb);
    if (! juce::exactlyEqual (next.balanceDb, prev.balanceDb))
        dsp.setBalance (next.balanceDb);
}

void readInto (DfxDsp& dsp, DspSettings& s)
{
    // getEffectValue() reports 0..1; FxSound multiplies by 10 for its UI scale.
    auto effect = [&dsp] (DfxDsp::Effect e) { return dsp.getEffectValue (e) * 10.0f; };
    s.fidelity = effect (DfxDsp::Fidelity);
    s.ambience = effect (DfxDsp::Ambience);
    s.surround = effect (DfxDsp::Surround);
    s.dynamicBoost = effect (DfxDsp::DynamicBoost);
    s.bass = effect (DfxDsp::Bass);
    for (int b = 0; b < DspSettings::numEqBands; ++b)
    {
        s.eqGainDb[(size_t) b] = dsp.getEqBandBoostCut (b);
        s.eqFreqHz[(size_t) b] = dsp.getEqBandFrequency (b);
    }
}
} // namespace DspParamsApply
