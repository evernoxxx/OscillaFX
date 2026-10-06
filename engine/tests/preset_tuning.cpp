// Measures Oscilla's own presets (presets/Movies.fac, presets/Music.fac) loaded the way the app does
// (core/Presets.cpp + DspParamsApply::applyAll) and checks tonal, dynamic and stereo targets vs bypass.
// Also measures the per-device starter profiles (app/Source/core/StarterProfileData.h) the same way.
// Signals: pink noise at -14 LUFS, hot pink noise at -12 dBFS RMS, a synthetic film scene
// (quiet dialogue, then loud dialogue + explosion), partially correlated and centre-panned stereo noise.
#include "DfxDsp.h"
#include "StarterProfileData.h"
#include <array>
#include <cmath>
#include <complex>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

namespace {

const int SAMPLE_RATE = 48000;
const int BLOCK_FRAMES = 480;
const int EQ_BANDS = 10;
const int FFT_SIZE = 8192;
const double SETTLE_SECONDS = 2.0;
const double TYPICAL_LUFS = -14.0;
const double HOT_PINK_RMS_DB = -12.0;
const double CLIP_CEILING_DB = -0.1;
const double QUIET_DIALOGUE_LUFS = -31.0;
const double LOUD_SCENE_LUFS = -12.0;
const double MASTER_CEILING_DB = -1.0; // mastered film peak
int g_failures = 0;

using Stereo = std::vector<float>; // interleaved L/R
using Mono = std::vector<double>;

struct Band { const char* name; double lo, hi; };
enum BandId { SUB, PUNCH, BASS, BOOM, LOW_MID, MID, PRESENCE, UPPER, SIBILANCE, AIR, NUM_BANDS };
const Band BANDS[NUM_BANDS] = {
    {"sub 30-50", 30, 50},          {"punch 50-100", 50, 100},    {"bass 100-200", 100, 200},
    {"boom 200-300", 200, 300},     {"low-mid 300-500", 300, 500}, {"mid 500-1k", 500, 1000},
    {"presence 1k-4k", 1000, 4000}, {"upper 4k-6k", 4000, 6000},   {"sibil. 6k-8k", 6000, 8000},
    {"air 10k-16k", 10000, 16000},
};

struct Settings {
    std::array<float, DfxDsp::NumEffects> effects;
    std::array<float, EQ_BANDS> eqFreqHz, eqGainDb;
};

void check(bool ok, const std::string& what)
{
    printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what.c_str());
    if (!ok) ++g_failures;
}

double toDb(double power) { return 10.0 * std::log10(power + 1e-30); }

// ---- DSP driving (mirrors the app) ----

bool readPreset(const std::string& path, Settings& out)
{
    DfxDsp dsp;
    dsp.setSignalFormat(32, 2, SAMPLE_RATE, 32);
    dsp.setNumBands(EQ_BANDS);
    if (dsp.loadPreset(std::wstring(path.begin(), path.end())) != 0) return false;
    for (int e = 0; e < DfxDsp::NumEffects; ++e)
        out.effects[e] = dsp.getEffectValue((DfxDsp::Effect)e) * 10.0f;
    for (int b = 0; b < EQ_BANDS; ++b) {
        out.eqFreqHz[b] = dsp.getEqBandFrequency(b);
        out.eqGainDb[b] = dsp.getEqBandBoostCut(b);
    }
    return true;
}

void applySettings(DfxDsp& dsp, const Settings& s)
{
    dsp.setSignalFormat(32, 2, SAMPLE_RATE, 32);
    dsp.setNumBands(EQ_BANDS);
    dsp.powerOn(true);
    for (int e = 0; e < DfxDsp::NumEffects; ++e)
        dsp.setEffectValue((DfxDsp::Effect)e, s.effects[e]);
    for (int b = 0; b < EQ_BANDS; ++b) {
        dsp.setEqBandFrequency(b, s.eqFreqHz[b]);
        dsp.setEqBandBoostCut(b, s.eqGainDb[b]);
    }
    dsp.setMasterGain(0.0f);
    dsp.setBalance(0.0f);
}

// nullptr = bypass (bit-exact, covered by dsp_offline).
Stereo process(const Settings* settings, const Stereo& in)
{
    if (!settings) return in;
    DfxDsp dsp;
    applySettings(dsp, *settings);
    Stereo out(in.size());
    int frames = (int)(in.size() / 2);
    for (int i = 0; i + BLOCK_FRAMES <= frames; i += BLOCK_FRAMES)
        dsp.processAudio((short*)&in[2 * i], (short*)&out[2 * i], BLOCK_FRAMES, 0);
    return out;
}

// ---- signal generation ----

struct Biquad {
    double b0, b1, b2, a1, a2, z1 = 0, z2 = 0;
    double run(double x)
    {
        double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

enum FilterType { LOWPASS, HIGHPASS, BANDPASS };

// RBJ cookbook filters (band-pass with 0 dB peak).
Biquad rbj(FilterType type, double fc, double q)
{
    double w = 2 * M_PI * fc / SAMPLE_RATE, alpha = std::sin(w) / (2 * q), cw = std::cos(w);
    double a0 = 1 + alpha, a1 = -2 * cw / a0, a2 = (1 - alpha) / a0;
    if (type == LOWPASS) return {(1 - cw) / 2 / a0, (1 - cw) / a0, (1 - cw) / 2 / a0, a1, a2};
    if (type == HIGHPASS) return {(1 + cw) / 2 / a0, -(1 + cw) / a0, (1 + cw) / 2 / a0, a1, a2};
    return {alpha / a0, 0, -alpha / a0, a1, a2};
}

Mono filtered(Mono x, Biquad f)
{
    for (double& v : x) v = f.run(v);
    return x;
}

Mono white(int frames, unsigned seed)
{
    std::mt19937 rng(seed);
    std::normal_distribution<double> dist(0.0, 1.0);
    Mono x(frames);
    for (double& v : x) v = dist(rng);
    return x;
}

// Paul Kellett's pink filter.
Mono pink(int frames, unsigned seed)
{
    Mono x = white(frames, seed);
    double b[7] = {0};
    for (double& v : x) {
        double w = v;
        b[0] = 0.99886 * b[0] + w * 0.0555179; b[1] = 0.99332 * b[1] + w * 0.0750759;
        b[2] = 0.96900 * b[2] + w * 0.1538520; b[3] = 0.86650 * b[3] + w * 0.3104856;
        b[4] = 0.55000 * b[4] + w * 0.5329522; b[5] = -0.7616 * b[5] - w * 0.0168980;
        v = b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362;
        b[6] = w * 0.115926;
    }
    return x;
}

// Speech-like source: gliding f0, three formants, syllable envelope, short 6-8 kHz "s" bursts.
Mono voice(int frames, unsigned seed)
{
    const double formants[3][3] = {{500, 150, 3.0}, {1500, 250, 2.0}, {2500, 300, 1.5}};
    Mono x(frames, 0.0);
    double phase = 0;
    for (int i = 0; i < frames; ++i) {
        double t = (double)i / SAMPLE_RATE, f0 = 140 + 25 * std::sin(2 * M_PI * 0.7 * t);
        phase += 2 * M_PI * f0 / SAMPLE_RATE;
        double env = std::sqrt(std::fmax(0.0, std::sin(2 * M_PI * 3.5 * t)));
        double sum = 0;
        for (int k = 1; k * f0 < 7000; ++k) {
            double f = k * f0, gain = 1.0;
            for (const auto& fm : formants) gain += fm[2] * std::exp(-std::pow((f - fm[0]) / fm[1], 2));
            sum += gain / (1 + f / 300) * std::sin(k * phase);
        }
        x[i] = env * sum;
    }
    Mono hiss = filtered(white(frames, seed), rbj(BANDPASS, 7000, 2.0));
    for (int i = 0; i < frames; ++i)
        if (std::fmod((double)i / SAMPLE_RATE, 0.6) < 0.06) x[i] += 0.6 * hiss[i];
    return x;
}

Mono explosion(int frames, unsigned seed)
{
    Mono rumble = filtered(filtered(white(frames, seed), rbj(LOWPASS, 120, 0.7)), rbj(LOWPASS, 120, 0.7));
    Mono crack = pink(frames, seed + 1);
    for (int i = 0; i < frames; ++i) {
        double t = std::fmod((double)i / SAMPLE_RATE, 1.5);
        rumble[i] = 6.0 * rumble[i] + crack[i] * std::exp(-t * 1.5);
    }
    return rumble;
}

Stereo toStereo(const Mono& l, const Mono& r)
{
    Stereo s(l.size() * 2);
    for (size_t i = 0; i < l.size(); ++i) { s[2 * i] = (float)l[i]; s[2 * i + 1] = (float)r[i]; }
    return s;
}

// Equal-power mix: correlation between L and R equals `correlation` (where left/right have content).
Stereo correlatedStereo(const Mono& common, const Mono& left, const Mono& right, double correlation)
{
    double c = std::sqrt(correlation), d = std::sqrt(1 - correlation);
    Mono l(common.size()), r(common.size());
    for (size_t i = 0; i < common.size(); ++i) { l[i] = c * common[i] + d * left[i]; r[i] = c * common[i] + d * right[i]; }
    return toStereo(l, r);
}

// Mix-like stereo: mono bass, correlation 0.5 above ~150 Hz.
Stereo musicStereo(int frames, unsigned seed)
{
    Biquad hp = rbj(HIGHPASS, 150, 0.707);
    return correlatedStereo(pink(frames, seed), filtered(pink(frames, seed + 1), hp),
                            filtered(pink(frames, seed + 2), hp), 0.5);
}

void scale(Stereo& s, double gain) { for (float& v : s) v = (float)(v * gain); }

// ---- analysis ----

size_t frameAt(double seconds) { return (size_t)(seconds * SAMPLE_RATE); }

// ITU-R BS.1770 K-weighted loudness (ungated; signals here are stationary per segment).
double lufs(const Stereo& s, size_t from, size_t to)
{
    double sum = 0;
    for (int ch = 0; ch < 2; ++ch) {
        Biquad shelf{1.53512485958697, -2.69169618940638, 1.19839281085285, -1.69065929318241, 0.73248077421585};
        Biquad highpass{1.0, -2.0, 1.0, -1.99004745483398, 0.99007225036621};
        double ms = 0;
        for (size_t i = 0; i < to; ++i) {
            double y = highpass.run(shelf.run(s[2 * i + ch]));
            if (i >= from) ms += y * y;
        }
        sum += ms / (to - from);
    }
    return -0.691 + toDb(sum);
}
double lufs(const Stereo& s) { return lufs(s, frameAt(SETTLE_SECONDS), s.size() / 2); }

void normalizeLufs(Stereo& s, double target) { scale(s, std::pow(10.0, (target - lufs(s, 0, s.size() / 2)) / 20)); }

double peakDb(const Stereo& s, size_t from = 0)
{
    double peak = 0;
    for (size_t i = 2 * from; i < s.size(); ++i) peak = std::fmax(peak, std::fabs(s[i]));
    return 20 * std::log10(peak + 1e-30);
}

void fft(std::vector<std::complex<double>>& a)
{
    size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        std::complex<double> wl = std::polar(1.0, -2 * M_PI / len);
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w = 1;
            for (size_t k = 0; k < len / 2; ++k, w *= wl) {
                std::complex<double> u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
            }
        }
    }
}

// Welch power spectrum (Hann, 50% overlap) summed over both channels, from `from` to `to`.
Mono powerSpectrum(const Stereo& s, size_t from, size_t to)
{
    Mono spec(FFT_SIZE / 2, 0.0);
    std::vector<std::complex<double>> buf(FFT_SIZE);
    for (int ch = 0; ch < 2; ++ch)
        for (size_t start = from; start + FFT_SIZE <= to; start += FFT_SIZE / 2) {
            for (int i = 0; i < FFT_SIZE; ++i)
                buf[i] = s[2 * (start + i) + ch] * 0.5 * (1 - std::cos(2 * M_PI * i / FFT_SIZE));
            fft(buf);
            for (int k = 0; k < FFT_SIZE / 2; ++k) spec[k] += std::norm(buf[k]);
        }
    return spec;
}

double bandDb(const Mono& spec, const Band& band)
{
    double binHz = (double)SAMPLE_RATE / FFT_SIZE, e = 0;
    for (size_t k = (size_t)std::ceil(band.lo / binHz); k * binHz < band.hi; ++k) e += spec[k];
    return toDb(e);
}

struct Width { double sideToMidDb, monoSumDb; };

// sideToMid: S/M energy. monoSum: (L+R)/2 energy vs mean channel energy (0 dB = mono, -3 dB = uncorrelated).
Width width(const Stereo& s, size_t from)
{
    double mid = 0, side = 0, chan = 0;
    for (size_t i = from; i < s.size() / 2; ++i) {
        double l = s[2 * i], r = s[2 * i + 1];
        mid += 0.25 * (l + r) * (l + r);
        side += 0.25 * (l - r) * (l - r);
        chan += 0.5 * (l * l + r * r);
    }
    return {toDb(side) - toDb(mid), toDb(mid) - toDb(chan)};
}

Stereo lowpassed(const Stereo& s, double fc)
{
    Biquad l = rbj(LOWPASS, fc, 0.707), r = l;
    Stereo out(s.size());
    for (size_t i = 0; i < s.size() / 2; ++i) { out[2 * i] = (float)l.run(s[2 * i]); out[2 * i + 1] = (float)r.run(s[2 * i + 1]); }
    return out;
}

Stereo monoSum(const Stereo& s)
{
    Stereo out(s.size());
    for (size_t i = 0; i < s.size(); i += 2) out[i] = out[i + 1] = 0.5f * (s[i] + s[i + 1]);
    return out;
}

// ---- test signals (built once) ----

// Loudness-normalise, then soft-limit below `ceilingDb` like a mastered soundtrack.
void master(Stereo& s, double targetLufs, double ceilingDb)
{
    normalizeLufs(s, targetLufs);
    double ceiling = std::pow(10.0, ceilingDb / 20);
    for (float& v : s) v = (float)(ceiling * std::tanh(v / ceiling));
}

const double SIGNAL_SECONDS = 8.0;
const double SCENE_HALF_SECONDS = 6.0;

struct Signals {
    Stereo pinkTypical, pinkHot, scene, wide, center;
};

Signals makeSignals()
{
    int frames = (int)frameAt(SIGNAL_SECONDS), half = (int)frameAt(SCENE_HALF_SECONDS);
    Signals sig;
    Mono p = pink(frames, 1);
    sig.pinkTypical = toStereo(p, p);
    normalizeLufs(sig.pinkTypical, TYPICAL_LUFS);
    sig.pinkHot = correlatedStereo(pink(frames, 2), pink(frames, 3), pink(frames, 4), 0.7);
    double ms = 0;
    for (float v : sig.pinkHot) ms += (double)v * v;
    scale(sig.pinkHot, std::pow(10.0, HOT_PINK_RMS_DB / 20) / std::sqrt(ms / sig.pinkHot.size()));

    Mono v = voice(2 * half, 5), boom = explosion(half, 6);
    Stereo quiet = toStereo(Mono(v.begin(), v.begin() + half), Mono(v.begin(), v.begin() + half));
    normalizeLufs(quiet, QUIET_DIALOGUE_LUFS);
    Mono loudVoice(v.begin() + half, v.end());
    Stereo loud = toStereo(loudVoice, loudVoice), boomS = toStereo(boom, boom);
    normalizeLufs(loud, -18.0);
    normalizeLufs(boomS, -14.0);
    for (size_t i = 0; i < loud.size(); ++i) loud[i] += boomS[i];
    master(loud, LOUD_SCENE_LUFS, MASTER_CEILING_DB);
    sig.scene = quiet;
    sig.scene.insert(sig.scene.end(), loud.begin(), loud.end());

    sig.wide = musicStereo(frames, 7);
    normalizeLufs(sig.wide, TYPICAL_LUFS);
    sig.center = sig.pinkTypical;
    return sig;
}

// ---- measurement ----

struct Measurements {
    std::array<double, NUM_BANDS> pinkBandDb;
    double pinkLufs, hotPeakDb;
    double quietLufs, loudLufs, scenePeakDb, voiceClarityDb, voiceSibilanceDb;
    Width wide, wideBass, center;
    double monoSumLufs, wideLufs;
};

Measurements measure(const Settings* settings, const Signals& sig)
{
    Measurements m;
    size_t settle = frameAt(SETTLE_SECONDS), end = frameAt(SIGNAL_SECONDS);
    Stereo pinkOut = process(settings, sig.pinkTypical);
    Mono spec = powerSpectrum(pinkOut, settle, end);
    for (int b = 0; b < NUM_BANDS; ++b) m.pinkBandDb[b] = bandDb(spec, BANDS[b]);
    m.pinkLufs = lufs(pinkOut);
    m.hotPeakDb = peakDb(process(settings, sig.pinkHot));

    Stereo scene = process(settings, sig.scene);
    size_t half = frameAt(SCENE_HALF_SECONDS);
    m.quietLufs = lufs(scene, settle, half);
    m.loudLufs = lufs(scene, half + settle, 2 * half);
    m.scenePeakDb = peakDb(scene);
    Mono voiceSpec = powerSpectrum(scene, settle, half);
    m.voiceClarityDb = bandDb(voiceSpec, {"", 1000, 4000}) - bandDb(voiceSpec, {"", 100, 500});
    m.voiceSibilanceDb = bandDb(voiceSpec, BANDS[SIBILANCE]) - bandDb(voiceSpec, BANDS[PRESENCE]);

    Stereo wide = process(settings, sig.wide);
    m.wide = width(wide, settle);
    m.wideBass = width(lowpassed(wide, 120), settle);
    m.center = width(process(settings, sig.center), settle);
    m.wideLufs = lufs(wide);
    m.monoSumLufs = lufs(monoSum(wide));
    return m;
}

// Band level change relative to the 500-1k reference, i.e. tonal balance independent of overall gain.
double tonalDelta(const Measurements& before, const Measurements& after, int band)
{
    return (after.pinkBandDb[band] - before.pinkBandDb[band]) - (after.pinkBandDb[MID] - before.pinkBandDb[MID]);
}

void printTable(const char* name, const Measurements& b, const Measurements& a)
{
    printf("%s (pink noise -14 LUFS; dB, before = bypass)\n", name);
    printf("  %-18s %8s %8s %8s %8s\n", "band", "before", "after", "delta", "tonal");
    for (int i = 0; i < NUM_BANDS; ++i)
        printf("  %-18s %8.1f %8.1f %+8.1f %+8.1f\n", BANDS[i].name, b.pinkBandDb[i], a.pinkBandDb[i],
               a.pinkBandDb[i] - b.pinkBandDb[i], tonalDelta(b, a, i));
    auto row = [](const char* what, double before, double after) {
        printf("  %-30s %8.2f %8.2f %+8.2f\n", what, before, after, after - before);
    };
    row("pink loudness LUFS", b.pinkLufs, a.pinkLufs);
    row("hot pink (-12 dBFS RMS) peak dBFS", b.hotPeakDb, a.hotPeakDb);
    row("scene quiet dialogue LUFS", b.quietLufs, a.quietLufs);
    row("scene loud (boom) LUFS", b.loudLufs, a.loudLufs);
    row("scene peak dBFS", b.scenePeakDb, a.scenePeakDb);
    row("voice clarity 1-4k vs 100-500", b.voiceClarityDb, a.voiceClarityDb);
    row("voice sibilance 6-8k vs 1-4k", b.voiceSibilanceDb, a.voiceSibilanceDb);
    row("wide S/M dB", b.wide.sideToMidDb, a.wide.sideToMidDb);
    row("wide mono-sum dB", b.wide.monoSumDb, a.wide.monoSumDb);
    row("wide <120 Hz mono-sum dB", b.wideBass.monoSumDb, a.wideBass.monoSumDb);
    row("wide mono-sum loudness - stereo", b.monoSumLufs - b.wideLufs, a.monoSumLufs - a.wideLufs);
    row("centre S/M dB", b.center.sideToMidDb, a.center.sideToMidDb);
}

void checkNoClipping(const Measurements& a)
{
    check(a.hotPeakDb < 0.0, "peak < 0 dBFS on -12 dBFS RMS pink noise");
    check(a.scenePeakDb < CLIP_CEILING_DB, "film scene peak < -0.1 dBFS");
}

void checkMovies(const Measurements& b, const Measurements& a)
{
    printf("movies checks\n");
    double punch = tonalDelta(b, a, PUNCH), boom = tonalDelta(b, a, BOOM);
    check(punch >= 3.0, "cinema bass: 50-100 Hz >= +3 dB vs mids");
    check(tonalDelta(b, a, SUB) >= 2.0, "cinema bass: 30-50 Hz >= +2 dB vs mids");
    check(boom <= 1.0 && boom <= punch - 3.0, "no boom: 200-300 Hz <= +1 dB and >= 3 dB under the bass lift");
    check(tonalDelta(b, a, LOW_MID) < 0.0, "slight low-mid cut 300-500 Hz");
    check(tonalDelta(b, a, PRESENCE) >= 1.5, "dialogue presence 1-4 kHz >= +1.5 dB vs mids");
    check(tonalDelta(b, a, SIBILANCE) <= tonalDelta(b, a, PRESENCE) - 1.0, "sibilance 6-8 kHz >= 1 dB under presence");
    check(a.voiceClarityDb - b.voiceClarityDb >= 1.5, "voice clarity (1-4k vs 100-500) +1.5 dB");
    check(a.voiceSibilanceDb - b.voiceSibilanceDb <= 0.0, "voice sibilance not raised vs presence");
    double quietLift = a.quietLufs - b.quietLufs, loudLift = a.loudLufs - b.loudLufs;
    check(quietLift - loudLift >= 3.0, "quiet dialogue lifted >= 3 dB more than loud scene");
    check(a.center.sideToMidDb < -12.0, "modest ambience: centre dialogue stays centred (S/M < -12 dB)");
    check(a.wide.sideToMidDb - b.wide.sideToMidDb <= 3.0, "modest width: S/M rises <= 3 dB");
    checkNoClipping(a);
}

void checkMusic(const Measurements& b, const Measurements& a)
{
    printf("music checks\n");
    check(tonalDelta(b, a, PUNCH) >= 3.0, "punchy bass: 50-100 Hz >= +3 dB vs mids");
    check(tonalDelta(b, a, BOOM) <= tonalDelta(b, a, PUNCH) - 2.0, "bass stays tight: 200-300 Hz >= 2 dB under 50-100");
    for (int band : {LOW_MID, PRESENCE})
        check(std::fabs(tonalDelta(b, a, band)) <= 2.0, std::string("balanced mids: ") + BANDS[band].name + " within 2 dB");
    check(tonalDelta(b, a, AIR) >= 2.0, "airy highs: 10-16 kHz >= +2 dB vs mids");
    check(a.wide.sideToMidDb - b.wide.sideToMidDb >= 4.0, "stereo width: S/M +4 dB on mix-like stereo");
    check(a.wide.monoSumDb >= -3.0, "mono compatible: mono-sum loss <= 3 dB");
    check((a.monoSumLufs - a.wideLufs) - (b.monoSumLufs - b.wideLufs) >= -2.0, "mono listener loses < 2 dB extra");
    check(a.wideBass.monoSumDb >= -1.0, "bass below 120 Hz stays mono (loss <= 1 dB)");
    check(a.center.sideToMidDb < -6.0, "centre-panned content keeps a centre image (S/M < -6 dB)");
    checkNoClipping(a);
}

// ---- starter profiles (per output device family) ----

const std::array<float, EQ_BANDS> STARTER_EQ_FREQ_HZ = { 62.5f, 115.734f, 214.311f, 396.85f, 734.867f,
                                                        1360.79f, 2519.84f, 4666.12f, 8640.48f, 16000.0f };

Settings starterSettings(StarterData::Family family, StarterData::Flavor flavor)
{
    StarterData::Voicing v = StarterData::make(family, flavor);
    Settings s;
    s.effects = v.effects;
    s.eqFreqHz = STARTER_EQ_FREQ_HZ;
    s.eqGainDb = v.eqDb;
    return s;
}

// Flat voicing vs bypass: what each kind of device needs.
void checkFlat(StarterData::Family family, const Measurements& b, const Measurements& a)
{
    using F = StarterData::Family;
    auto tonal = [&](int band) { return tonalDelta(b, a, band); };
    auto allWithin = [&](double limit) {
        for (int band = 0; band < NUM_BANDS; ++band)
            if (std::fabs(tonal(band)) > limit) return false;
        return true;
    };
    switch (family) {
    case F::builtInSpeakers:
        check(tonal(SUB) <= -2.0, "low-cut protection: 30-50 Hz <= -2 dB vs mids");
        check(tonal(PUNCH) <= -1.5, "low-cut protection: 50-100 Hz <= -1.5 dB vs mids");
        check(tonal(PRESENCE) >= 1.5, "presence: 1-4 kHz >= +1.5 dB vs mids");
        check(a.voiceClarityDb - b.voiceClarityDb >= 2.0, "speech clarity +2 dB");
        check(tonal(PUNCH) >= -6.0, "protection, not a hole: 50-100 Hz no lower than -6 dB");
        break;
    case F::headphones:
        check(tonal(PUNCH) >= 1.5 && tonal(PUNCH) <= 4.0, "slight bass: 50-100 Hz +1.5..+4 dB vs mids");
        check(tonal(AIR) >= 1.5 && tonal(AIR) <= 4.0, "air: 10-16 kHz +1.5..+4 dB vs mids");
        check(std::fabs(tonal(PRESENCE)) <= 1.5 && std::fabs(tonal(LOW_MID)) <= 1.5, "mids stay natural (within 1.5 dB)");
        break;
    case F::bluetooth:
        check(allWithin(2.5), "flat-ish: every band within 2.5 dB of the mids");
        check(a.hotPeakDb <= -0.25 && a.scenePeakDb <= -0.25, "Bluetooth headroom: peaks stay under the -0.3 dBFS maximizer ceiling");
        break;
    case F::airPods:
        check(allWithin(2.0), "nearly untouched: every band within 2 dB of the mids");
        check(a.hotPeakDb <= -0.25 && a.scenePeakDb <= -0.25, "headroom: peaks stay under the -0.3 dBFS maximizer ceiling");
        break;
    case F::display:
        check(tonal(PUNCH) >= 1.5, "body for thin speakers: 50-100 Hz >= +1.5 dB vs mids");
        check(tonal(LOW_MID) <= 0.0, "boxiness cut: 300-500 Hz not raised");
        check(tonal(PRESENCE) >= 1.0, "clarity: 1-4 kHz >= +1 dB vs mids");
        break;
    case F::generic:
        check(allWithin(1.5), "neutral: every band within 1.5 dB of the mids");
        break;
    }
    check(a.pinkLufs - b.pinkLufs <= 4.0, "loudness rise <= 4 dB (no surprise volume jump)");
}

// A flavour vs the Flat starter of the same family.
void checkFlavor(StarterData::Flavor flavor, const Measurements& flat, const Measurements& a)
{
    using FL = StarterData::Flavor;
    auto tonal = [&](int band) { return tonalDelta(flat, a, band); };
    switch (flavor) {
    case FL::flat:
        break;
    case FL::warm:
        check(std::fmax(tonal(PUNCH), tonal(BASS)) >= 1.0, "warm: 50-200 Hz >= +1 dB vs Flat");
        check(tonal(AIR) <= -1.5, "warm: 10-16 kHz >= 1.5 dB under Flat");
        check(tonal(SIBILANCE) <= -1.5, "warm: 6-8 kHz >= 1.5 dB under Flat");
        break;
    case FL::bassHeavy:
        check(tonal(PUNCH) >= 2.0, "bass-heavy: 50-100 Hz >= +2 dB vs Flat");
        check(tonal(BOOM) <= tonal(PUNCH) - 1.0, "bass-heavy: tight, 200-300 Hz >= 1 dB under 50-100 Hz");
        check(std::fabs(tonal(PRESENCE)) <= 1.5, "bass-heavy: presence within 1.5 dB of Flat");
        break;
    case FL::voice:
        check(tonal(PRESENCE) >= 1.5, "voice: 1-4 kHz >= +1.5 dB vs Flat");
        check(a.voiceClarityDb - flat.voiceClarityDb >= 1.0, "voice: speech clarity >= +1 dB vs Flat");
        check(tonal(PUNCH) <= -0.5, "voice: 50-100 Hz trimmed");
        check(a.voiceSibilanceDb - flat.voiceSibilanceDb <= 0.0, "voice: sibilance not raised vs presence");
        break;
    case FL::wide:
        check(a.wide.sideToMidDb - flat.wide.sideToMidDb >= 2.0, "wide: S/M >= +2 dB vs Flat on mix-like stereo");
        check(a.wide.monoSumDb >= -3.0, "wide: mono-compatible (mono-sum loss <= 3 dB)");
        check(a.wideBass.monoSumDb >= -1.0, "wide: bass below 120 Hz stays mono (loss <= 1 dB)");
        check(a.center.sideToMidDb < -6.0, "wide: centre-panned content keeps a centre image");
        check(std::fabs(tonal(PUNCH)) <= 1.0, "wide: bass level within 1 dB of Flat");
        break;
    }
}

void checkStarters(const Signals& sig, const Measurements& bypass)
{
    using StarterData::Family;
    using StarterData::Flavor;
    struct { Family family; const char* name; } families[] = {
        {Family::builtInSpeakers, "built-in speakers"}, {Family::headphones, "headphones"},
        {Family::bluetooth, "bluetooth / external"},    {Family::airPods, "airpods"},
        {Family::display, "hdmi / display"},            {Family::generic, "generic"}};
    const Flavor flavors[] = {Flavor::flat, Flavor::warm, Flavor::bassHeavy, Flavor::voice, Flavor::wide};
    for (const auto& fam : families) {
        printf("starter profiles: %s\n", fam.name);
        Measurements flat;
        for (Flavor flavor : flavors) {
            Settings s = starterSettings(fam.family, flavor);
            Measurements m = measure(&s, sig);
            if (flavor == Flavor::flat) flat = m;
            printf("  %-11s pinkLUFS %+6.2f  hotPeak %+6.2f  scenePeak %+6.2f | punch %+5.1f air %+5.1f pres %+5.1f | wide S/M %+6.2f\n",
                   StarterData::flavorInfo(flavor).name, m.pinkLufs - bypass.pinkLufs, m.hotPeakDb, m.scenePeakDb,
                   tonalDelta(bypass, m, PUNCH), tonalDelta(bypass, m, AIR), tonalDelta(bypass, m, PRESENCE),
                   m.wide.sideToMidDb - bypass.wide.sideToMidDb);
            std::string tag = std::string(fam.name) + " / " + StarterData::flavorInfo(flavor).name + ": ";
            int before = g_failures;
            checkNoClipping(m);
            if (flavor != Flavor::flat) check(m.pinkLufs - bypass.pinkLufs <= 6.0, "loudness rise <= 6 dB (no surprise volume jump)");
            if (flavor == Flavor::flat) checkFlat(fam.family, bypass, m);
            else checkFlavor(flavor, flat, m);
            if (g_failures != before) printf("  ^ failures in %s\n", tag.c_str());
        }
    }
}

} // namespace

int main()
{
    Signals sig = makeSignals();
    Measurements bypass = measure(nullptr, sig);
    struct { const char* file; void (*checks)(const Measurements&, const Measurements&); } presets[] = {
        {"Movies.fac", checkMovies}, {"Music.fac", checkMusic}};
    for (const auto& p : presets) {
        Settings s;
        bool loaded = readPreset(std::string(OSCILLA_TUNED_PRESET_DIR) + "/" + p.file, s);
        check(loaded, std::string("load ") + p.file);
        if (!loaded) continue;
        Measurements after = measure(&s, sig);
        printTable(p.file, bypass, after);
        p.checks(bypass, after);
    }
    checkStarters(sig, bypass);
    printf(g_failures ? "FAILED: %d\n" : "ALL PASS\n", g_failures);
    return g_failures ? 1 : 0;
}
