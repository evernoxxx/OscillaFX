#pragma once

#include "SpscRingBuffer.h"

// Consumer-side resampler that keeps the ring fill near a target by nudging the
// read ratio (PI controller), absorbing clock drift between two audio devices.
// render() runs on the output IOProc: no allocation, no locks.
class DriftResampler
{
public:
    void prepare (double inputRate, double outputRate, size_t targetFillFrames);

    // Writes numFrames interleaved stereo frames. Returns false (and silence) while
    // priming or after an underrun.
    bool render (SpscRingBuffer& ring, float* out, int numFrames);

    double ratio() const { return currentRatio; }
    double baseRatio() const { return nominalRatio; }
    double smoothedFill() const { return fillEstimate; }
    size_t targetFill() const { return target; }
    int underrunCount() const { return underruns; }

private:
    void updateRatio (size_t available);
    void interpolate (SpscRingBuffer& ring, float* out, int numFrames);

    double nominalRatio = 1.0, currentRatio = 1.0;
    double fraction = 0.0, fillEstimate = 0.0, integral = 0.0;
    size_t target = 1024;
    bool isPrimed = false;
    int underruns = 0;
};
