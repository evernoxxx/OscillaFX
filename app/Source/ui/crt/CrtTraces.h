#pragma once

#include "ui/crt/CrtScene.h"

// Beam geometry for the visualiser modes, in the phosphor plane (CrtDisplay local coordinates).
namespace Crt::Traces
{
    using Beams = std::vector<BeamSegment>;
    using Points = std::vector<juce::Point<float>>;

    // Spectrum as glowing segmented columns: the bands are smoothly interpolated to `bars` columns,
    // each a stack of short dashes, with a floating peak cap.
    void spectrum (Beams&, juce::Rectangle<float> area, const float* levels, const float* peaks, int numBands, int bars);

    // A drawn line of uniform brightness (waveform, X-Y, EQ curve, idle baseline).
    void line (Beams&, const Points&, float density, float sigma);

    void spot (Beams&, juce::Point<float>, float density, float sigma);

    // Catmull-Rom through the points, flattened to roughly `step` pixel segments.
    Points smooth (const Points&, float step);

    //==============================================================================================
    // Oscilloscope traces. Dense material (music, noise) must still read as ONE clean line, so the
    // raw samples are never plotted directly: they are anti-alias filtered down to a few hundred
    // points, peaks are kept partly, and a short gaussian smoothing removes what is left. Tested offline by
    // app/Tests/CrtTraceTest.cpp (a fuzzy trace fails it).
    constexpr int wavePointCount = 320;
    constexpr int xyPointCount = 256;

    // Soft vertical deflection: never leaves the screen, linear for small signals.
    inline float deflect (float level) { return std::tanh (level); }

    // `points` filtered, decimated levels (sample units, before gain) from `count` raw samples.
    std::vector<float> waveLevels (const float* mono, int count, int points = wavePointCount);

    // Level -> polyline across `area`: x spreads evenly, y is deflect (gain * level) about the centre.
    Points waveLine (const std::vector<float>& levels, juce::Rectangle<float> area, float gain);

    struct XyLevels
    {
        std::vector<float> x, y;   // left, right
    };

    XyLevels xyLevels (const float* left, const float* right, int count, int points = xyPointCount);

    // Left drives x, right drives y (up). `halfExtent` is the half-size of the square plot area.
    Points xyLine (const XyLevels&, juce::Point<float> centre, float halfExtent, float gain);

    float peakOf (const std::vector<float>&);
    float pathLength (const Points&);

    // Slow automatic gain: fast attack so a loud passage never clips, slow release so the picture
    // does not pump. gain() scales the (filtered) signal so its recent peak sits at `target`.
    class LevelFollower
    {
    public:
        explicit LevelFollower (float targetPeak, float minGain = 0.4f, float maxGain = 32.0f);

        void update (float peak, float dt);   // peaks below the silence floor are ignored: no gain pumping on pauses
        float gain() const;
        void reset();

    private:
        float target, lowest, highest;
        float envelope = 0.0f;
        bool primed = false;
    };
}
