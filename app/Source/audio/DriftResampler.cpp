#include "DriftResampler.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
constexpr size_t HISTORY_FRAMES = 5;      // 4 Hermite taps + 1 frame rounding margin
constexpr double FILL_SMOOTHING = 0.02;   // per callback
constexpr double PROPORTIONAL_GAIN = 1.0e-3;
constexpr double INTEGRAL_GAIN = 1.0e-6;
constexpr double MAX_ADJUST = 5.0e-3;     // +-5000 ppm
constexpr size_t HIGH_WATER_FACTOR = 4;

float hermite (float xm1, float x0, float x1, float x2, float t)
{
    const float c1 = 0.5f * (x1 - xm1);
    const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * t + c2) * t + c1) * t + x0;
}
} // namespace

void DriftResampler::prepare (double inputRate, double outputRate, size_t targetFillFrames)
{
    nominalRatio = currentRatio = inputRate / outputRate;
    target = std::max<size_t> (targetFillFrames, 64);
    fraction = integral = 0.0;
    fillEstimate = (double) target;
    isPrimed = false;
    underruns = 0;
}

void DriftResampler::updateRatio (size_t available)
{
    fillEstimate += FILL_SMOOTHING * ((double) available - fillEstimate);
    const double error = (fillEstimate - (double) target) / (double) target;
    integral = std::clamp (integral + error * INTEGRAL_GAIN, -MAX_ADJUST, MAX_ADJUST);
    const double adjust = std::clamp (PROPORTIONAL_GAIN * error + integral, -MAX_ADJUST, MAX_ADJUST);
    currentRatio = nominalRatio * (1.0 + adjust);
}

void DriftResampler::interpolate (SpscRingBuffer& ring, float* out, int numFrames)
{
    size_t pos = 0;
    for (int i = 0; i < numFrames; ++i)
    {
        const float* a = ring.peek (pos);
        const float* b = ring.peek (pos + 1);
        const float* c = ring.peek (pos + 2);
        const float* d = ring.peek (pos + 3);
        const auto t = (float) fraction;
        out[2 * i] = hermite (a[0], b[0], c[0], d[0], t);
        out[2 * i + 1] = hermite (a[1], b[1], c[1], d[1], t);
        fraction += currentRatio;
        const auto whole = (size_t) fraction;
        pos += whole;
        fraction -= (double) whole;
    }
    ring.consume (pos);
}

bool DriftResampler::render (SpscRingBuffer& ring, float* out, int numFrames)
{
    auto available = ring.availableToRead();

    if (! isPrimed && available >= target + HISTORY_FRAMES)
    {
        isPrimed = true;
        fillEstimate = (double) available;
    }

    if (isPrimed && available > target * HIGH_WATER_FACTOR)
    {
        ring.consume (available - target);
        available = target;
        fillEstimate = (double) target;
    }

    if (isPrimed)
        updateRatio (available);

    const auto needed = (size_t) (fraction + numFrames * currentRatio) + HISTORY_FRAMES;
    if (! isPrimed || available < needed)
    {
        if (isPrimed)
            ++underruns;
        isPrimed = false;
        std::memset (out, 0, sizeof (float) * (size_t) numFrames * SpscRingBuffer::NUM_CHANNELS);
        return false;
    }

    interpolate (ring, out, numFrames);
    return true;
}
