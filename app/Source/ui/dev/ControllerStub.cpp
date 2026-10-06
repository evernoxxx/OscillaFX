// UI preview only: a fake AudioEngine that synthesises signal so the panel can be developed and
// screenshotted without the virtual driver or touching system audio. Configure with
// -DOSCILLA_UI_STUB=ON (see the end of the top-level CMakeLists.txt); compiles to nothing otherwise.
#ifndef OSCILLA_UI_STUB
 #define OSCILLA_UI_STUB 0
#endif

#if OSCILLA_UI_STUB

#include "audio/AudioEngine.h"
#include <cmath>

struct AudioEngine::Impl
{
    juce::Array<OutputDevice> devices { OutputDevice { "stub-speakers", "MacBook Pro Speakers" },
                                        OutputDevice { "stub-airpods", "AirPods Pro" },
                                        OutputDevice { "stub-usb", "USB Audio DAC" } };
    OutputDevice current = devices.getFirst();
    juce::String error = juce::SystemStats::getEnvironmentVariable ("OSCILLA_STUB_ERROR", {});
    DspSettings settings;
    const double startTime = juce::Time::getMillisecondCounterHiRes() * 0.001;

    double now() const { return juce::Time::getMillisecondCounterHiRes() * 0.001 - startTime; }
    bool isSilent() const { return std::getenv ("OSCILLA_STUB_SILENT") != nullptr; } // re-read: snapshots toggle it
};

AudioEngine::AudioEngine() : impl (std::make_unique<Impl>()) {}
AudioEngine::~AudioEngine() = default;

bool AudioEngine::start() { return impl->error.isEmpty(); }
void AudioEngine::stop() {}
juce::String AudioEngine::lastError() const { return impl->error; }
void AudioEngine::applySettings (const DspSettings& s) { impl->settings = s; }
juce::Array<OutputDevice> AudioEngine::outputDevices() const { return impl->devices; }
OutputDevice AudioEngine::currentOutputDevice() const { return impl->current; }

void AudioEngine::setOutputDevice (const juce::String& uid)
{
    for (const auto& d : impl->devices)
        if (d.uid == uid)
            impl->current = d;
}

void AudioEngine::getSpectrum (float* bands10) const
{
    const auto t = impl->now();
    const auto beat = std::pow (0.5 + 0.5 * std::sin (t * juce::MathConstants<double>::twoPi * 2.0), 6.0);
    const auto isGated = impl->isSilent() || ! impl->settings.power;
    for (int i = 0; i < 10; ++i)
    {
        const auto tilt = 0.75 - 0.05 * i;
        const auto wobble = 0.18 * std::sin (t * (1.3 + 0.37 * i) + i);
        const auto kick = i < 3 ? 0.3 * beat : 0.0;
        bands10[i] = isGated ? 0.0f : (float) juce::jlimit (0.0, 1.0, tilt + wobble + kick - 0.2);
    }
}

namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    // Deterministic white noise per sample index: consecutive frames see the same signal, like a real stream.
    double noise (long long i)
    {
        auto x = (unsigned long long) i * 0x9E3779B97F4A7C15ull;
        x ^= x >> 29;
        x *= 0xBF58476D1CE4E5B9ull;
        x ^= x >> 32;
        return (double) (x & 0xffffff) / (double) 0x7fffff - 1.0;
    }

    // A dense stereo "music" mix: kick, bass, chord stabs panned apart, hi-hat noise bursts, hiss.
    void musicSample (long long i, double& left, double& right)
    {
        const auto t = (double) i / 48000.0;
        const auto beat = std::fmod (t * 2.0, 1.0);
        const auto eighth = std::fmod (t * 4.0, 1.0);
        const auto kick = std::sin (twoPi * (48.0 * t + 12.0 * (1.0 - std::exp (-beat * 25.0)) / 25.0 * 25.0)) * std::exp (-beat * 7.0) * 0.45;
        const auto bass = (std::sin (twoPi * 55.0 * t) + 0.4 * std::sin (twoPi * 110.0 * t)) * 0.18;
        const auto stab = std::exp (-eighth * 3.0);
        const auto chordA = (std::sin (twoPi * 220.0 * t) + std::sin (twoPi * 277.2 * t + 0.5) + std::sin (twoPi * 329.6 * t + 1.1)) * 0.07 * stab;
        const auto chordB = (std::sin (twoPi * 440.0 * t + 0.3) + std::sin (twoPi * 554.4 * t + 0.9) + 0.5 * std::sin (twoPi * 2637.0 * t)) * 0.05;
        const auto lead = std::sin (twoPi * 880.0 * t + 0.8 * std::sin (twoPi * 5.0 * t)) * 0.05 + std::sin (twoPi * 3520.0 * t) * 0.012;
        const auto hat = noise (i) * std::exp (-eighth * 14.0) * 0.16;
        const auto hiss = noise (i + 77) * 0.012;
        left = kick + bass + chordA * 1.3 + chordB * 0.5 + lead * 0.6 + hat + hiss;
        right = kick + bass + chordA * 0.5 + chordB * 1.3 + lead * 1.1 + noise (i + 5) * std::exp (-eighth * 14.0) * 0.16 + hiss;
    }

    void pinkSample (long long i, double& left, double& right)
    {
        static thread_local double b[2][7] {};
        for (int ch = 0; ch < 2; ++ch)
        {
            const auto white = noise (i * 2 + ch) * 0.5;
            auto* s = b[ch];
            s[0] = 0.99886 * s[0] + white * 0.0555179;
            s[1] = 0.99332 * s[1] + white * 0.0750759;
            s[2] = 0.96900 * s[2] + white * 0.1538520;
            s[3] = 0.86650 * s[3] + white * 0.3104856;
            s[4] = 0.55000 * s[4] + white * 0.5329522;
            s[5] = -0.7616 * s[5] - white * 0.0168980;
            const auto pink = (s[0] + s[1] + s[2] + s[3] + s[4] + s[5] + s[6] + white * 0.5362) * 0.35;
            s[6] = white * 0.115926;
            (ch == 0 ? left : right) = pink;
        }
    }

    void tonesSample (long long i, double now, double& left, double& right)
    {
        const auto s = (double) i / 48000.0;
        const auto swell = 0.8 + 0.2 * std::sin (now * twoPi * 0.7); // lets persistence show
        const auto tone = 0.55 * swell * std::sin (twoPi * 220.0 * s) + 0.2 * std::sin (twoPi * 660.0 * s + 0.4);
        const auto quadrature = 0.55 * std::sin (twoPi * 330.0 * s + 1.1);
        left = tone;
        right = 0.5 * (tone + quadrature);
    }
}

// OSCILLA_STUB_SIGNAL: music (default, dense stereo mix) | mono (the same, L = R) | pink | tones (two clean sines)
int AudioEngine::getScopeSamples (float* left, float* right, int maxSamples) const
{
    constexpr double sampleRate = 48000.0;
    const auto kind = juce::SystemStats::getEnvironmentVariable ("OSCILLA_STUB_SIGNAL", "music");
    const auto now = impl->now();
    const auto end = (long long) (now * sampleRate);
    const auto n = juce::jmin (maxSamples, 4096);
    const auto gate = impl->isSilent() || ! impl->settings.power ? 0.0 : 1.0;
    for (int k = 0; k < n; ++k)
    {
        const auto i = end - n + k;
        double l = 0.0, r = 0.0;
        if (kind == "pink")
            pinkSample (i, l, r);
        else if (kind == "tones")
            tonesSample (i, now, l, r);
        else
            musicSample (i, l, r);
        if (kind == "mono")
            r = l = 0.5 * (l + r);
        left[k] = (float) (l * gate);
        right[k] = (float) (r * gate);
    }
    return n;
}

#endif
