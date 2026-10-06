// Writes Oscilla's own FxSound-format presets (Movies.fac, Music.fac) from tuned DfxDsp values.
// Usage: make_preset <output dir>. Values are verified by tests/preset_tuning.cpp.
// Master gain is not part of the .fac format; the presets are tuned to stay clip-free at 0 dB.
#include "DfxDsp.h"
#include <array>
#include <cstdio>
#include <string>

namespace {

const int SAMPLE_RATE = 48000;
const int EQ_BANDS = 10;

struct PresetSpec {
    const wchar_t* name;
    std::array<float, DfxDsp::NumEffects> effects; // 0..10, DfxDsp::Effect order
    std::array<float, EQ_BANDS> eqFreqHz;
    std::array<float, EQ_BANDS> eqGainDb;
};

// Effects: Fidelity, Ambience, Surround, DynamicBoost, Bass.
const PresetSpec PRESETS[] = {
    // Cinema bass without boom, dialogue presence, sibilance tamed, dynamic boost lifts quiet dialogue.
    { L"Movies",
      { 1.5f, 2.0f, 1.5f, 4.5f, 5.5f },
      { 62.5f, 100.0f, 250.0f, 400.0f, 800.0f, 1500.0f, 2800.0f, 4500.0f, 7200.0f, 14000.0f },
      { 3.0f, 1.0f, -2.5f, -1.5f, 0.0f, 1.5f, 2.5f, 0.0f, -3.0f, 0.0f } },
    // Punchy 50-100 Hz, flat mids, air on top, Surround + Ambience for width on two speakers.
    { L"Music",
      { 2.0f, 5.0f, 3.0f, 2.5f, 4.5f },
      { 62.5f, 100.0f, 220.0f, 400.0f, 800.0f, 1500.0f, 2800.0f, 5000.0f, 8000.0f, 14000.0f },
      { 2.0f, 1.0f, -1.0f, -0.5f, 0.0f, 0.5f, 0.0f, -1.0f, 0.0f, 3.0f } },
};

void applySpec(DfxDsp& dsp, const PresetSpec& spec)
{
    for (int e = 0; e < DfxDsp::NumEffects; ++e)
        dsp.setEffectValue((DfxDsp::Effect)e, spec.effects[e]);
    for (int b = 0; b < EQ_BANDS; ++b) {
        dsp.setEqBandFrequency(b, spec.eqFreqHz[b]);
        dsp.setEqBandBoostCut(b, spec.eqGainDb[b]);
    }
    dsp.setMasterGain(0.0f);
}

bool writePreset(const PresetSpec& spec, const std::wstring& dir)
{
    DfxDsp dsp;
    dsp.setSignalFormat(32, 2, SAMPLE_RATE, 32);
    dsp.setNumBands(EQ_BANDS);
    applySpec(dsp, spec);
    return dsp.savePreset(spec.name, dir) == 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: make_preset <output dir>\n");
        return 2;
    }
    std::string dir = argv[1];
    for (const PresetSpec& spec : PRESETS) {
        bool ok = writePreset(spec, std::wstring(dir.begin(), dir.end()));
        printf("%ls.fac: %s\n", spec.name, ok ? "written" : "FAILED");
        if (!ok) return 1;
    }
    return 0;
}
