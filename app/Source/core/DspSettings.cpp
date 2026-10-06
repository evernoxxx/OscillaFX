#include "DspSettings.h"
#include <cmath>

namespace
{
constexpr float EFFECT_MIN = 0.0f, EFFECT_MAX = 10.0f;
constexpr float EQ_MIN_DB = -12.0f, EQ_MAX_DB = 12.0f;
constexpr float EQ_MIN_HZ = 20.0f, EQ_MAX_HZ = 20000.0f;
constexpr float MASTER_MIN_DB = -20.0f, MASTER_MAX_DB = 20.0f;

juce::var arrayToVar (const std::array<float, DspSettings::numEqBands>& values)
{
    juce::Array<juce::var> list;
    for (float v : values)
        list.add (v);
    return list;
}

void varToArray (const juce::var& v, std::array<float, DspSettings::numEqBands>& out, float lo, float hi)
{
    auto* list = v.getArray();
    if (list == nullptr)
        return;
    for (int i = 0; i < DspSettings::numEqBands && i < list->size(); ++i)
        out[(size_t) i] = juce::jlimit (lo, hi, (float) (*list)[i]);
}

float readFloat (const juce::var& obj, const char* key, float fallback, float lo, float hi)
{
    const auto& v = obj[key];
    return v.isVoid() ? fallback : juce::jlimit (lo, hi, (float) v);
}
} // namespace

namespace
{
float clampFloat (float v, float fallback, float lo, float hi) { return std::isfinite (v) ? juce::jlimit (lo, hi, v) : fallback; }
} // namespace

void DspSettings::clampToRanges()
{
    const DspSettings defaults;
    fidelity = clampFloat (fidelity, defaults.fidelity, EFFECT_MIN, EFFECT_MAX);
    ambience = clampFloat (ambience, defaults.ambience, EFFECT_MIN, EFFECT_MAX);
    surround = clampFloat (surround, defaults.surround, EFFECT_MIN, EFFECT_MAX);
    dynamicBoost = clampFloat (dynamicBoost, defaults.dynamicBoost, EFFECT_MIN, EFFECT_MAX);
    bass = clampFloat (bass, defaults.bass, EFFECT_MIN, EFFECT_MAX);
    for (size_t i = 0; i < (size_t) numEqBands; ++i)
    {
        eqGainDb[i] = clampFloat (eqGainDb[i], 0.0f, EQ_MIN_DB, EQ_MAX_DB);
        eqFreqHz[i] = clampFloat (eqFreqHz[i], defaults.eqFreqHz[i], EQ_MIN_HZ, EQ_MAX_HZ);
    }
    masterGainDb = clampFloat (masterGainDb, 0.0f, MASTER_MIN_DB, MASTER_MAX_DB);
    balance = clampFloat (balance, 0.0f, -1.0f, 1.0f);
    limiterCeilingDb = clampFloat (limiterCeilingDb, 0.0f, limiterMinDb, 0.0f);
    displayMode = juce::jlimit (0, numDisplayModes - 1, displayMode);
    galleryIndex = juce::jmax (0, galleryIndex);
}

juce::var DspSettings::toVar() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("power", power);
    obj->setProperty ("preset", presetName);
    obj->setProperty ("fidelity", fidelity);
    obj->setProperty ("ambience", ambience);
    obj->setProperty ("surround", surround);
    obj->setProperty ("dynamicBoost", dynamicBoost);
    obj->setProperty ("bass", bass);
    obj->setProperty ("eqGainDb", arrayToVar (eqGainDb));
    obj->setProperty ("eqFreqHz", arrayToVar (eqFreqHz));
    obj->setProperty ("masterGainDb", masterGainDb);
    obj->setProperty ("balance", balance);
    obj->setProperty ("nightMode", nightMode);
    obj->setProperty ("limiterCeilingDb", limiterCeilingDb);
    obj->setProperty ("displayMode", displayMode);
    obj->setProperty ("eqEditMode", eqEditMode);
    obj->setProperty ("galleryIndex", galleryIndex);
    return juce::var (obj);
}

DspSettings DspSettings::fromVar (const juce::var& v)
{
    DspSettings s;
    if (! v.isObject())
        return s;

    s.power = v.getProperty ("power", s.power);
    s.presetName = v.getProperty ("preset", s.presetName).toString();
    s.fidelity = readFloat (v, "fidelity", s.fidelity, EFFECT_MIN, EFFECT_MAX);
    s.ambience = readFloat (v, "ambience", s.ambience, EFFECT_MIN, EFFECT_MAX);
    s.surround = readFloat (v, "surround", s.surround, EFFECT_MIN, EFFECT_MAX);
    s.dynamicBoost = readFloat (v, "dynamicBoost", s.dynamicBoost, EFFECT_MIN, EFFECT_MAX);
    s.bass = readFloat (v, "bass", s.bass, EFFECT_MIN, EFFECT_MAX);
    varToArray (v["eqGainDb"], s.eqGainDb, EQ_MIN_DB, EQ_MAX_DB);
    varToArray (v["eqFreqHz"], s.eqFreqHz, EQ_MIN_HZ, EQ_MAX_HZ);
    s.masterGainDb = readFloat (v, "masterGainDb", s.masterGainDb, MASTER_MIN_DB, MASTER_MAX_DB);
    s.balance = readFloat (v, "balance", s.balance, -1.0f, 1.0f);
    s.nightMode = v.getProperty ("nightMode", s.nightMode);
    s.limiterCeilingDb = readFloat (v, "limiterCeilingDb", s.limiterCeilingDb, limiterMinDb, 0.0f);
    s.displayMode = juce::jlimit (0, numDisplayModes - 1, (int) v.getProperty ("displayMode", s.displayMode));
    s.eqEditMode = v.getProperty ("eqEditMode", s.eqEditMode);
    s.galleryIndex = juce::jmax (0, (int) v.getProperty ("galleryIndex", s.galleryIndex));
    return s;
}

bool DspSettings::operator== (const DspSettings& o) const
{
    return power == o.power && presetName == o.presetName
        && juce::exactlyEqual (fidelity, o.fidelity) && juce::exactlyEqual (ambience, o.ambience)
        && juce::exactlyEqual (surround, o.surround) && juce::exactlyEqual (dynamicBoost, o.dynamicBoost)
        && juce::exactlyEqual (bass, o.bass)
        && eqGainDb == o.eqGainDb && eqFreqHz == o.eqFreqHz
        && juce::exactlyEqual (masterGainDb, o.masterGainDb) && juce::exactlyEqual (balance, o.balance)
        && nightMode == o.nightMode && juce::exactlyEqual (limiterCeilingDb, o.limiterCeilingDb)
        && displayMode == o.displayMode && eqEditMode == o.eqEditMode && galleryIndex == o.galleryIndex;
}
