// Offline DSP regression: float32 stereo in -> DfxDsp -> out, driven like the Windows app does.
// Checks: finite output, preset sanity, bass boost raises low-frequency energy, bypass is bit-exact.
#include "DfxDsp.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>

namespace {

const int kSampleRate = 48000;
const int kBlockFrames = 480;
const int kSpectrumBands = 10;
int g_failures = 0;

void check(bool ok, const char* what)
{
    printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_failures;
}

std::vector<float> makeSine(double freq, double amp, double seconds)
{
    int frames = (int)(seconds * kSampleRate);
    std::vector<float> in(frames * 2);
    for (int i = 0; i < frames; ++i)
        in[2 * i] = in[2 * i + 1] = (float)(amp * std::sin(2 * M_PI * freq * i / kSampleRate));
    return in;
}

void configure(DfxDsp& dsp, bool power_on)
{
    dsp.setSignalFormat(32, 2, kSampleRate, 32);
    dsp.powerOn(power_on);
}

std::vector<float> process(DfxDsp& dsp, const std::vector<float>& in)
{
    std::vector<float> out(in.size());
    int frames = (int)(in.size() / 2);
    for (int i = 0; i + kBlockFrames <= frames; i += kBlockFrames)
        dsp.processAudio((short*)&in[2 * i], (short*)&out[2 * i], kBlockFrames, 0);
    return out;
}

bool allFinite(const std::vector<float>& v)
{
    for (float x : v) if (!std::isfinite(x)) return false;
    return true;
}

// RMS of the second half (after filters settle).
double settledRms(const std::vector<float>& v)
{
    size_t start = v.size() / 2;
    double e = 0;
    for (size_t i = start; i < v.size(); ++i) e += (double)v[i] * v[i];
    return std::sqrt(e / (v.size() - start));
}

double toneGain(double freq, float bass, bool power_on = true)
{
    DfxDsp dsp;
    configure(dsp, power_on);
    dsp.setEffectValue(DfxDsp::Bass, bass);
    std::vector<float> in = makeSine(freq, 0.1, 2.0);
    return settledRms(process(dsp, in)) / settledRms(in);
}

void testPresets(const std::vector<std::string>& presets)
{
    printf("presets\n");
    std::vector<float> in = makeSine(100, 0.25, 3.0);
    std::vector<float> hi = makeSine(3000, 0.12, 3.0);
    for (size_t i = 0; i < in.size(); ++i) in[i] += hi[i];

    for (const std::string& name : presets) {
        DfxDsp dsp;
        configure(dsp, true);
        std::string path = std::string(OSCILLA_PRESET_DIR) + "/" + name;
        int rc = dsp.loadPreset(std::wstring(path.begin(), path.end()));
        std::vector<float> out = process(dsp, in);
        float bands[kSpectrumBands] = {0};
        dsp.getSpectrumBandValues(bands, kSpectrumBands);
        double band_sum = 0;
        for (float b : bands) band_sum += b;
        double rms = settledRms(out);
        printf(" %s: load=%d rms=%.4f bands_sum=%.3f\n", name.c_str(), rc, rms, band_sum);
        check(rc == 0 && allFinite(out) && rms > 0.01 && rms < 2.0 && band_sum > 0.0, name.c_str());
    }
}

void testBassBoost()
{
    printf("bass boost sweep (gain = out rms / in rms)\n");
    const double freqs[] = {40, 60, 100, 200, 1000, 4000, 12000};
    double low_off = 0, low_on = 0, high_off = 0, high_on = 0;
    for (double f : freqs) {
        double off = toneGain(f, 0.0), on = toneGain(f, 10.0);
        printf("  %6.0f Hz: bass0 %.3f  bass10 %.3f\n", f, off, on);
        if (f <= 100) { low_off += off; low_on += on; }
        if (f >= 4000) { high_off += off; high_on += on; }
    }
    check(low_on > low_off * 1.05, "bass=10 raises energy below 100 Hz");
    check(low_on / low_off > high_on / high_off, "bass boost favors lows over highs");
}

void testBypass()
{
    printf("bypass\n");
    DfxDsp dsp;
    configure(dsp, false);
    std::vector<float> in = makeSine(440, 0.3, 1.0);
    std::vector<float> out = process(dsp, in);
    check(!dsp.isPowerOn(), "isPowerOn false after powerOn(false)");
    check(std::memcmp(in.data(), out.data(), in.size() * sizeof(float)) == 0, "bypass output == input");
}

} // namespace

int main()
{
    {
        DfxDsp dsp;
        configure(dsp, true);
        check(dsp.isPowerOn(), "isPowerOn true after powerOn(true)");
        std::vector<float> out = process(dsp, makeSine(100, 0.25, 3.0));
        check(allFinite(out), "finite output, no preset");
    }
    testPresets({"Pop.fac", "Metal.fac", "Classical.fac", "Trap.fac", "Bass (Quizal).fac", "Jazz.fac"});
    testBassBoost();
    testBypass();
    printf(g_failures ? "FAILED: %d\n" : "ALL PASS\n", g_failures);
    return g_failures ? 1 : 0;
}
