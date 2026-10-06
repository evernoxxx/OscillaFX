#include "ui/crt/CrtTraces.h"

namespace Crt::Traces
{
namespace
{
    constexpr int spectrumRows = 20;          // dash rows of a full-height column
    constexpr float dashFill = 0.58f;          // dash length / row pitch
    constexpr float columnFill = 0.28f;       // half-width of a column's two beam lines / slot

    float catmullRom (const float* v, int n, float t)
    {
        const auto i = juce::jlimit (0, n - 2, (int) t);
        const auto f = t - (float) i;
        const auto p0 = v[juce::jmax (0, i - 1)], p1 = v[i], p2 = v[i + 1], p3 = v[juce::jmin (n - 1, i + 2)];
        return 0.5f * (2.0f * p1 + (p2 - p0) * f + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * f * f
                       + (3.0f * p1 - p0 - 3.0f * p2 + p3) * f * f * f);
    }

    // Five fine beam lines side by side make a flat-topped, crisp-edged dash (a gaussian sum
    // with spacing = 2 sigma has almost no ripple); `flatTop` is their summed height at the centre.
    constexpr int dashLines = 5;
    constexpr float flatTop = 1.25f;

    void dash (Beams& beams, float x, float y, float length, float halfWidth, float energy)
    {
        const auto sigma = halfWidth * 0.2f;
        const auto spacing = halfWidth * 0.4f;
        for (int i = 0; i < dashLines; ++i)
        {
            const auto dx = ((float) i - (float) (dashLines - 1) * 0.5f) * spacing;
            beams.push_back ({ { x + dx, y + length * 0.5f }, { x + dx, y - length * 0.5f }, energy / flatTop, sigma });
        }
    }

    void column (Beams& beams, float x, float bottom, float pitch, float halfWidth, int litRows)
    {
        for (int row = 0; row < litRows; ++row)
        {
            const auto y = bottom - pitch * ((float) row + 0.5f);
            dash (beams, x, y, pitch * dashFill, halfWidth * 0.8f, 0.45f + 0.45f * (float) row / (float) spectrumRows);
        }
    }
}

void spectrum (Beams& beams, juce::Rectangle<float> area, const float* levels, const float* peaks, int numBands, int bars)
{
    const auto slot = area.getWidth() / (float) bars;
    const auto pitch = area.getHeight() / (float) spectrumRows;
    const auto halfWidth = slot * columnFill;

    for (int i = 0; i < bars; ++i)
    {
        const auto t = (float) i * (float) (numBands - 1) / (float) (bars - 1);
        const auto level = juce::jlimit (0.0f, 1.0f, catmullRom (levels, numBands, t));
        const auto peak = juce::jlimit (0.0f, 1.0f, catmullRom (peaks, numBands, t));
        const auto x = area.getX() + slot * ((float) i + 0.5f);
        const auto rows = juce::roundToInt (level * (float) spectrumRows * 0.92f);

        if (rows == 0)
            beams.push_back ({ { x - halfWidth * 0.6f, area.getBottom() - pitch * 0.5f },
                               { x + halfWidth * 0.6f, area.getBottom() - pitch * 0.5f }, 0.3f, 0.7f });
        column (beams, x, area.getBottom(), pitch, halfWidth, rows);

        const auto peakRows = peak * (float) spectrumRows * 0.92f;
        if (peakRows > (float) rows + 0.6f)
        {
            const auto y = area.getBottom() - pitch * (peakRows + 0.3f);
            beams.push_back ({ { x - halfWidth * 0.8f, y }, { x + halfWidth * 0.8f, y }, 0.85f, 0.75f });
        }
    }
}

void line (Beams& beams, const Points& points, float density, float sigma)
{
    for (size_t i = 1; i < points.size(); ++i)
        beams.push_back ({ points[i - 1], points[i], density, sigma });
}

void spot (Beams& beams, juce::Point<float> p, float density, float sigma)
{
    beams.push_back ({ p, p, density, sigma });
}

Points smooth (const Points& p, float step)
{
    if (p.size() < 2)
        return p;

    Points out { p.front() };
    for (size_t i = 0; i + 1 < p.size(); ++i)
    {
        const auto p0 = p[i == 0 ? 0 : i - 1], p1 = p[i], p2 = p[i + 1], p3 = p[juce::jmin (p.size() - 1, i + 2)];
        const auto steps = juce::jmax (1, (int) (p1.getDistanceFrom (p2) / step));
        for (int s = 1; s <= steps; ++s)
        {
            const auto t = (float) s / (float) steps, t2 = t * t, t3 = t2 * t;
            out.push_back ((p1 * 2.0f + (p2 - p0) * t + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2
                            + (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f);
        }
    }
    return out;
}

//==============================================================================================
namespace
{
    constexpr float peakKept = 0.25f;       // how much of a bucket's extreme survives the anti-alias filter
    constexpr float waveSmoothing = 1.9f;   // gaussian sigma in points (~1 kHz cut-off at a 1024-sample window)
    constexpr int maxVisibleReversals = 28; // noise-like material is smoothed harder until its line has at most this many turns
    constexpr float visibleSwing = 0.02f;   // a wiggle smaller than this fraction of the peak is invisible
    constexpr float xySmoothing = 1.2f;
    constexpr float silenceFloor = 2.0e-3f;
    constexpr float attackSeconds = 0.05f, releaseSeconds = 2.5f;

    // Triangle-weighted average over two buckets around each output point (a proper low-pass for
    // this decimation), nudged towards the bucket's larger extreme so drum hits keep their height.
    std::vector<float> decimate (const float* x, int count, int points, float peakBias)
    {
        std::vector<float> out ((size_t) points, 0.0f);
        if (count < 2 || points < 2)
            return out;

        const auto bucket = (float) count / (float) points;
        for (int i = 0; i < points; ++i)
        {
            const auto centre = ((float) i + 0.5f) * bucket;
            const auto first = juce::jmax (0, (int) std::floor (centre - bucket));
            const auto last = juce::jmin (count - 1, (int) std::ceil (centre + bucket));
            auto sum = 0.0f, weights = 0.0f, lowest = x[juce::jlimit (0, count - 1, (int) centre)], highest = lowest;
            for (int s = first; s <= last; ++s)
            {
                const auto w = 1.0f - std::abs ((float) s + 0.5f - centre) / bucket;
                if (w <= 0.0f)
                    continue;
                sum += w * x[s];
                weights += w;
                if (std::abs ((float) s + 0.5f - centre) <= bucket * 0.5f)
                {
                    lowest = juce::jmin (lowest, x[s]);
                    highest = juce::jmax (highest, x[s]);
                }
            }
            const auto mean = weights > 0.0f ? sum / weights : 0.0f;
            const auto extreme = highest - mean > mean - lowest ? highest : lowest;
            out[(size_t) i] = mean + peakBias * (extreme - mean);
        }
        return out;
    }

    // Direction changes of a curve, ignoring wiggles smaller than `minSwing`.
    int reversalsOf (const std::vector<float>& v, float minSwing)
    {
        auto reversals = 0, direction = 0;
        auto reference = v.empty() ? 0.0f : v.front();
        for (auto y : v)
        {
            if (direction == 0)
            {
                if (std::abs (y - reference) > minSwing)
                {
                    direction = y > reference ? 1 : -1;
                    reference = y;
                }
            }
            else if (direction > 0 ? y > reference : y < reference)
                reference = y;
            else if (std::abs (y - reference) > minSwing)
            {
                ++reversals;
                direction = -direction;
                reference = y;
            }
        }
        return reversals;
    }

    // Gaussian smoothing over `sigma` points (radius 3 sigma, edges clamped): the short low-pass that
    // turns the decimated samples into a smooth curve. (A 3-point B-spline is the sigma ~0.6 case.)
    void smooth (std::vector<float>& v, float sigma)
    {
        if (v.size() < 3 || sigma <= 0.0f)
            return;
        const auto radius = juce::jmax (1, (int) std::ceil (sigma * 3.0f));
        std::vector<float> kernel ((size_t) (2 * radius + 1));
        auto total = 0.0f;
        for (int k = -radius; k <= radius; ++k)
            total += kernel[(size_t) (k + radius)] = std::exp (-0.5f * (float) (k * k) / (sigma * sigma));

        std::vector<float> next (v.size());
        const auto last = (int) v.size() - 1;
        for (int i = 0; i <= last; ++i)
        {
            auto sum = 0.0f;
            for (int k = -radius; k <= radius; ++k)
                sum += kernel[(size_t) (k + radius)] * v[(size_t) juce::jlimit (0, last, i + k)];
            next[(size_t) i] = sum / total;
        }
        v.swap (next);
    }
}

std::vector<float> waveLevels (const float* mono, int count, int points)
{
    const auto decimated = decimate (mono, count, points, peakKept);

    // Tonal material keeps its detail; noise-like material (pink noise, dense mixes) would still
    // zig-zag, so the smoothing widens until the line has a calm number of turns.
    std::vector<float> levels;
    auto sigma = waveSmoothing;
    for (int attempt = 0; attempt < 5; ++attempt, sigma *= 1.4f)
    {
        levels = decimated;
        smooth (levels, sigma);
        if (reversalsOf (levels, visibleSwing * peakOf (levels)) <= maxVisibleReversals)
            break;
    }
    return levels;
}

Points waveLine (const std::vector<float>& levels, juce::Rectangle<float> area, float gain)
{
    Points out;
    const auto n = levels.size();
    out.reserve (n);
    for (size_t i = 0; i < n; ++i)
        out.push_back ({ area.getX() + area.getWidth() * (float) i / (float) juce::jmax ((size_t) 1, n - 1),
                         area.getCentreY() - deflect (gain * levels[i]) * area.getHeight() * 0.5f });
    return out;
}

XyLevels xyLevels (const float* left, const float* right, int count, int points)
{
    XyLevels out { decimate (left, count, points, 0.0f), decimate (right, count, points, 0.0f) };
    smooth (out.x, xySmoothing);
    smooth (out.y, xySmoothing);
    return out;
}

Points xyLine (const XyLevels& levels, juce::Point<float> centre, float halfExtent, float gain)
{
    Points out;
    out.reserve (levels.x.size());
    for (size_t i = 0; i < levels.x.size(); ++i)
        out.push_back ({ centre.x + deflect (gain * levels.x[i]) * halfExtent, centre.y - deflect (gain * levels.y[i]) * halfExtent });
    return out;
}

float peakOf (const std::vector<float>& v)
{
    auto peak = 0.0f;
    for (auto x : v)
        peak = juce::jmax (peak, std::abs (x));
    return peak;
}

float pathLength (const Points& p)
{
    auto length = 0.0f;
    for (size_t i = 1; i < p.size(); ++i)
        length += p[i - 1].getDistanceFrom (p[i]);
    return length;
}

LevelFollower::LevelFollower (float targetPeak, float minGain, float maxGain)
    : target (targetPeak), lowest (minGain), highest (maxGain)
{
}

void LevelFollower::update (float peak, float dt)
{
    if (peak < silenceFloor)
        return;
    if (! primed)
    {
        envelope = peak;
        primed = true;
        return;
    }
    const auto tau = peak > envelope ? attackSeconds : releaseSeconds;
    envelope += (peak - envelope) * (1.0f - std::exp (-dt / tau));
}

float LevelFollower::gain() const
{
    return primed ? juce::jlimit (lowest, highest, target / envelope) : 1.0f;
}

void LevelFollower::reset()
{
    primed = false;
    envelope = 0.0f;
}
}
