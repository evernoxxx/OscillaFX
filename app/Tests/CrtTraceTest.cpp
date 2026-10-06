// Offline checks of the CRT trace generation (ui/crt/CrtTraces): no window, no GPU.
//
// The CRT must draw ONE clean, thin line like a Tektronix 7613, even for dense music or noise.
// A grainy trace plots every audio wiggle. These tests run the
// very same code CrtDisplay uses on pink noise and on a dense stereo music-like mix, and bound the
// polyline's roughness (direction reversals and total variation per screen). The naive way of plotting
// (strided raw samples) is run through the same measures and must FAIL them, so a regression to a
// fuzzy trace cannot slip through.
#include "ui/crt/CrtTraces.h"

#include <cmath>
#include <cstdio>
#include <random>

namespace
{
int failures = 0;

void check (bool ok, const char* what)
{
    std::printf ("  [%s] %s\n", ok ? "PASS" : "FAIL", what);
    if (! ok)
        ++failures;
}

using Crt::Traces::Points;

// The tube face as CrtDisplay lays it out: graticule of a 456 x 342 glass with 25 / 26 px margins.
const juce::Rectangle<float> graticule { 25.0f, 26.0f, 406.0f, 290.0f };
constexpr int scopeWindow = 1024;   // CrtDisplay's waveWindow
constexpr int scopeBuffer = 4096;
constexpr double sampleRate = 48000.0;
constexpr float waveTarget = 0.7f, xyTarget = 0.85f;  // CrtDisplay's auto-gain targets

//==============================================================================
// signals

std::vector<float> pinkNoise (int n, unsigned seed)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> white (-1.0f, 1.0f);
    float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    std::vector<float> out ((size_t) n);
    for (auto& v : out)
    {
        const auto w = white (rng);
        b0 = 0.99886f * b0 + w * 0.0555179f;
        b1 = 0.99332f * b1 + w * 0.0750759f;
        b2 = 0.96900f * b2 + w * 0.1538520f;
        b3 = 0.86650f * b3 + w * 0.3104856f;
        b4 = 0.55000f * b4 + w * 0.5329522f;
        b5 = -0.7616f * b5 - w * 0.0168980f;
        v = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362f) * 0.11f;
        b6 = w * 0.115926f;
    }
    return out;
}

struct Stereo
{
    std::vector<float> left, right;
    std::vector<float> mono() const
    {
        std::vector<float> m (left.size());
        for (size_t i = 0; i < m.size(); ++i)
            m[i] = 0.5f * (left[i] + right[i]);
        return m;
    }
};

// Kick, bass, panned chord stabs and leads (partials up to several kHz), plus hi-hat style noise
// bursts and hiss: about as busy as a scope buffer gets.
Stereo denseMusic (int n, double startSeconds, unsigned seed)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<double> white (-1.0, 1.0);
    const auto twoPi = 6.283185307179586;
    Stereo out { std::vector<float> ((size_t) n), std::vector<float> ((size_t) n) };
    for (int i = 0; i < n; ++i)
    {
        const auto t = startSeconds + (double) i / sampleRate;
        const auto beat = std::fmod (t * 2.0, 1.0), eighth = std::fmod (t * 4.0, 1.0);
        const auto kick = std::sin (twoPi * (50.0 * t + 8.0 * (1.0 - std::exp (-beat * 25.0)))) * std::exp (-beat * 7.0) * 0.45;
        const auto bass = (std::sin (twoPi * 55.0 * t) + 0.4 * std::sin (twoPi * 110.0 * t)) * 0.18;
        const auto stab = std::exp (-eighth * 3.0);
        const auto chordA = (std::sin (twoPi * 220.0 * t) + std::sin (twoPi * 277.2 * t) + std::sin (twoPi * 329.6 * t)) * 0.07 * stab;
        const auto chordB = (std::sin (twoPi * 440.0 * t) + std::sin (twoPi * 554.4 * t) + 0.5 * std::sin (twoPi * 2637.0 * t)) * 0.05;
        const auto lead = std::sin (twoPi * 880.0 * t + 0.8 * std::sin (twoPi * 5.0 * t)) * 0.05 + std::sin (twoPi * 3520.0 * t) * 0.02
                          + std::sin (twoPi * 7040.0 * t) * 0.012 + std::sin (twoPi * 11000.0 * t) * 0.01;
        const auto burst = std::exp (-eighth * 14.0) * 0.18;
        out.left[(size_t) i] = (float) (kick + bass + chordA * 1.3 + chordB * 0.5 + lead * 0.6 + white (rng) * burst + white (rng) * 0.015);
        out.right[(size_t) i] = (float) (kick + bass + chordA * 0.5 + chordB * 1.3 + lead * 1.1 + white (rng) * burst + white (rng) * 0.015);
    }
    return out;
}

//==============================================================================
// the two ways of drawing a wave

// What CrtDisplay does now.
Points cleanTrace (const std::vector<float>& mono, int start)
{
    const auto levels = Crt::Traces::waveLevels (mono.data() + start, scopeWindow);
    Crt::Traces::LevelFollower follower (waveTarget);
    follower.update (Crt::Traces::peakOf (levels), 1.0f);
    return Crt::Traces::waveLine (levels, graticule, follower.gain());
}

// Naive plotting: every other raw sample, straight to the screen. Must fail the measures below.
Points fuzzyTrace (const std::vector<float>& mono, int start)
{
    auto peak = 0.0f;
    for (int i = 0; i < scopeWindow; ++i)
        peak = juce::jmax (peak, std::abs (mono[(size_t) (start + i)]));
    const auto gain = waveTarget / juce::jmax (peak, 1.0e-4f);

    Points out;
    for (int k = 0; k < scopeWindow; k += 3)
        out.push_back ({ graticule.getX() + graticule.getWidth() * (float) k / (float) (scopeWindow - 1),
                         graticule.getCentreY() - std::tanh (gain * mono[(size_t) (start + k)] * 1.15f) * graticule.getHeight() * 0.5f });
    return out;
}

//==============================================================================
// measures

struct Roughness
{
    int reversals = 0;            // direction changes of y bigger than `minSwing` pixels
    float variation = 0.0f;       // total variation of y, in screen heights
};

Roughness measure (const Points& p)
{
    constexpr float minSwing = 1.5f;   // wiggles smaller than this are not visible as a reversal
    Roughness r;
    auto reference = p.front().y;
    auto direction = 0;   // +1 going down the screen, -1 going up, 0 not yet decided
    for (size_t i = 1; i < p.size(); ++i)
    {
        const auto y = p[i].y;
        r.variation += std::abs (y - p[i - 1].y);
        if (direction == 0)
        {
            if (std::abs (y - reference) > minSwing)
            {
                direction = y > reference ? 1 : -1;
                reference = y;
            }
        }
        else if (direction > 0)
        {
            if (y > reference)
                reference = y;
            else if (y < reference - minSwing)
            {
                ++r.reversals;
                direction = -1;
                reference = y;
            }
        }
        else
        {
            if (y < reference)
                reference = y;
            else if (y > reference + minSwing)
            {
                ++r.reversals;
                direction = 1;
                reference = y;
            }
        }
    }
    r.variation /= graticule.getHeight();
    return r;
}

// Worst case over many windows of a signal.
struct Worst
{
    Roughness clean, fuzzy;
};

template <typename Signal>
Worst worstOver (Signal&& makeMono)
{
    Worst w;
    for (int trial = 0; trial < 24; ++trial)
    {
        const auto mono = makeMono (trial);
        const auto start = scopeBuffer - scopeWindow - 37 * trial % 600;
        const auto clean = measure (cleanTrace (mono, start)), fuzzy = measure (fuzzyTrace (mono, start));
        w.clean.reversals = juce::jmax (w.clean.reversals, clean.reversals);
        w.clean.variation = juce::jmax (w.clean.variation, clean.variation);
        w.fuzzy.reversals = juce::jmin (trial == 0 ? 1 << 30 : w.fuzzy.reversals, fuzzy.reversals);
        w.fuzzy.variation = juce::jmin (trial == 0 ? 1.0e9f : w.fuzzy.variation, fuzzy.variation);
    }
    return w;
}

// A clean line crosses the screen with a handful of visible direction changes. These bounds sit
// well above what the filter produces and well below what raw samples produce (see the printout).
constexpr int maxReversals = 36;
constexpr float maxVariation = 7.0f;     // screen heights of up-and-down travel

void checkSignal (const char* name, const Worst& w)
{
    std::printf ("  %-22s clean: %3d reversals, TV %5.2f   |   raw samples (best of 24): %3d reversals, TV %5.2f\n",
                 name, w.clean.reversals, w.clean.variation, w.fuzzy.reversals, w.fuzzy.variation);
    check (w.clean.reversals <= maxReversals && w.clean.variation <= maxVariation,
           (juce::String (name) + ": trace is one clean line (reversals <= " + juce::String (maxReversals)
            + ", total variation <= " + juce::String (maxVariation, 0) + " screens)").toRawUTF8());
    check (w.fuzzy.reversals > maxReversals || w.fuzzy.variation > maxVariation,
           (juce::String (name) + ": the same measure FAILS the old fuzzy plotting (test has teeth)").toRawUTF8());
}

//==============================================================================
void testWaveCleanliness()
{
    std::printf ("wave trace cleanliness\n");
    checkSignal ("pink noise", worstOver ([] (int trial) { return pinkNoise (scopeBuffer, 100u + (unsigned) trial); }));
    checkSignal ("dense stereo music", worstOver ([] (int trial) { return denseMusic (scopeBuffer, 1.37 * trial + 0.2, 7u + (unsigned) trial).mono(); }));
}

void testWaveShape()
{
    std::printf ("wave trace shape\n");
    const auto mono = denseMusic (scopeBuffer, 3.1, 5u).mono();
    const auto points = cleanTrace (mono, scopeBuffer - scopeWindow);
    check (points.size() >= 256 && points.size() <= 400, "256..400 points");

    auto monotonic = true, inside = true;
    for (size_t i = 0; i < points.size(); ++i)
    {
        monotonic &= i == 0 || points[i].x > points[i - 1].x;
        inside &= graticule.expanded (0.5f).contains (points[i]);
    }
    check (monotonic, "x advances steadily left to right");
    check (inside, "trace stays inside the graticule");

    // a clean sine must survive the filter with its height and its cycle count
    std::vector<float> sine ((size_t) scopeBuffer);
    for (size_t i = 0; i < sine.size(); ++i)
        sine[i] = 0.5f * (float) std::sin (6.283185307179586 * 187.5 * (double) i / sampleRate);   // 4 cycles in 1024 samples
    const auto line = cleanTrace (sine, 0);
    auto top = 1.0e9f, bottom = -1.0e9f;
    for (auto& p : line)
    {
        top = juce::jmin (top, p.y);
        bottom = juce::jmax (bottom, p.y);
    }
    const auto fill = (bottom - top) / graticule.getHeight();
    std::printf ("  sine fills %.0f%% of the screen height, %d reversals\n", fill * 100.0f, measure (line).reversals);
    check (fill > 0.5f && fill < 0.75f, "auto-gain: a steady tone fills about 60% of the height, at any level");
    check (measure (line).reversals >= 7 && measure (line).reversals <= 9, "4 cycles of a sine draw 4 cycles (about 8 reversals), not mush");
}

//==============================================================================
// X-Y

struct Extents
{
    float width = 0.0f, height = 0.0f, spread = 0.0f;   // bounding box of the plot, and mean distance from the diagonal
};

Extents plotExtents (const Stereo& s, int start, Crt::Traces::LevelFollower& follower)
{
    const auto levels = Crt::Traces::xyLevels (s.left.data() + start, s.right.data() + start, scopeWindow);
    follower.update (juce::jmax (Crt::Traces::peakOf (levels.x), Crt::Traces::peakOf (levels.y)), 1.0f);
    const auto half = juce::jmin (graticule.getWidth(), graticule.getHeight()) * 0.5f * 0.98f;
    const auto points = Crt::Traces::xyLine (levels, graticule.getCentre(), half, follower.gain());

    Extents e;
    auto minX = 1.0e9f, maxX = -1.0e9f, minY = 1.0e9f, maxY = -1.0e9f;
    for (auto& p : points)
    {
        minX = juce::jmin (minX, p.x);
        maxX = juce::jmax (maxX, p.x);
        minY = juce::jmin (minY, p.y);
        maxY = juce::jmax (maxY, p.y);
        const auto dx = p.x - graticule.getCentreX(), dy = graticule.getCentreY() - p.y;   // screen y is down
        e.spread += std::abs (dx - dy) * 0.70710678f;                                      // distance from the L = R diagonal
    }
    e.width = (maxX - minX) / (half * 2.0f);
    e.height = (maxY - minY) / (half * 2.0f);
    e.spread /= (float) points.size();
    return e;
}

void testXY()
{
    std::printf ("X-Y trace\n");
    const auto music = denseMusic (scopeBuffer, 2.0, 11u);

    // mono: left == right, thin line on the diagonal
    Stereo mono { music.mono(), music.mono() };
    Crt::Traces::LevelFollower monoFollower (xyTarget);
    const auto thin = plotExtents (mono, scopeBuffer - scopeWindow, monoFollower);
    std::printf ("  mono: spread from the diagonal %.2f px, plot %.0f%% x %.0f%%\n", thin.spread, thin.width * 100.0f, thin.height * 100.0f);
    check (thin.spread < 0.5f, "mono signal draws a thin diagonal line (it sits on L = R)");

    // stereo: a loose ellipse that fills about 60% of the plot, whether quiet or loud
    for (auto level : { 0.08f, 0.3f, 1.0f })
    {
        Stereo scaled = music;
        for (auto& v : scaled.left) v *= level;
        for (auto& v : scaled.right) v *= level;
        Crt::Traces::LevelFollower follower (xyTarget);
        Extents e;
        for (int k = 0; k < 6; ++k)
            e = plotExtents (scaled, scopeBuffer - scopeWindow - k * 100, follower);
        const auto fill = juce::jmax (e.width, e.height);
        std::printf ("  stereo at %.2f: plot %.0f%% x %.0f%%, spread %.1f px\n", level, e.width * 100.0f, e.height * 100.0f, e.spread);
        check (fill > 0.4f && fill < 0.85f, ("auto-gain: music at level " + juce::String (level, 2) + " fills about 60% of the plot").toRawUTF8());
        check (e.spread > 1.0f, "stereo is wider than the mono diagonal");
    }

    // loudness changes are followed slowly (no pumping): a 1 s gap does not move the gain much
    Crt::Traces::LevelFollower follower (xyTarget);
    for (int i = 0; i < 100; ++i)
        follower.update (0.5f, 0.016f);
    const auto settled = follower.gain();
    for (int i = 0; i < 60; ++i)
        follower.update (0.0f, 0.016f);   // silence: gain must hold
    check (std::abs (follower.gain() - settled) < 1.0e-4f, "gain holds through silence");
    for (int i = 0; i < 60; ++i)
        follower.update (0.25f, 0.016f);  // 1 s at half the level
    const auto after = follower.gain() / settled;
    check (after > 1.0f && after < 1.6f, "gain rises slowly after the signal gets quieter (release ~2.5 s)");
}
}

int main()
{
    testWaveCleanliness();
    testWaveShape();
    testXY();
    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILED", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
