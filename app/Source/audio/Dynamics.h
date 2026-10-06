#pragma once

#include <algorithm>
#include <cmath>

// Lock-free, allocation-free dynamics stages that run after DfxDsp on the capture thread:
//   NightCompressor  feed-forward stereo-linked compressor for quiet-hours listening
//   SafetyLimiter    soft-attack limiter with hard guard, never exceeds its ceiling
//   DynamicsChain    the two in order, with click-free enable/disable ramps
// Interleaved stereo float buffers, in place. prepare() runs on the message thread while the
// IOProcs are stopped; configure() and process() are for the audio thread only.
namespace Dynamics
{
constexpr float OFF_THRESHOLD_DB = -0.05f;   // limiter ceilings at or above this mean "off"
constexpr float MIN_CEILING_DB = -24.0f;
constexpr float NIGHT_SAFETY_CEILING_DB = -1.0f; // night mode never leaves the signal above this
constexpr float INSANE_SAMPLE = 1.0e6f;          // NaN, infinity or this loud: DSP fault, silenced rather than propagated

// Peak of a stereo frame; a NaN / inf / absurd frame is zeroed in place and counts as silence,
// so one bad block cannot poison the smoothing state or reach the speakers.
inline float framePeak (float* frame)
{
    const float left = std::fabs (frame[0]), right = std::fabs (frame[1]);
    if (left < INSANE_SAMPLE && right < INSANE_SAMPLE) // false for NaN too
        return std::max (left, right);
    frame[0] = frame[1] = 0.0f;
    return 0.0f;
}

inline float dbToGain (float db) { return std::exp (db * 0.11512925f); }       // 10^(db/20)
inline float ceilingGain (float db) { return std::pow (10.0f, db / 20.0f); }   // exact enough to compare against
inline float gainToDb (float gain) { return 8.685889638f * std::log (gain); }  // 20 log10

// One-pole coefficient for a time constant (ms); 0 ms gives an instantaneous follower.
inline float timeCoefficient (double sampleRate, float milliseconds)
{
    if (milliseconds <= 0.0f)
        return 0.0f;
    return (float) std::exp (-1.0 / (sampleRate * 0.001 * (double) milliseconds));
}

class NightCompressor
{
public:
    static constexpr float THRESHOLD_DB = -24.0f;
    static constexpr float RATIO = 3.0f;
    static constexpr float ATTACK_MS = 5.0f;
    static constexpr float RELEASE_MS = 200.0f;
    static constexpr float KNEE_DB = 6.0f;
    static constexpr float MAKEUP_REFERENCE_DB = -18.0f; // typical programme level that keeps its loudness

    // Static input->output curve, without makeup: gain reduction in dB (>= 0) for a peak level in dBFS.
    static float reductionDb (float levelDb)
    {
        const float over = levelDb - THRESHOLD_DB;
        const float slope = 1.0f - 1.0f / RATIO;
        if (2.0f * over < -KNEE_DB)
            return 0.0f;
        if (2.0f * std::fabs (over) <= KNEE_DB)
        {
            const float x = over + KNEE_DB * 0.5f;
            return slope * x * x / (2.0f * KNEE_DB);
        }
        return slope * over;
    }

    // Auto makeup: undo the reduction at the reference level, so quiet passages come up and
    // loud ones go down around an unchanged average loudness.
    static float makeupDb() { return reductionDb (MAKEUP_REFERENCE_DB); }

    void prepare (double sampleRate)
    {
        attack = timeCoefficient (sampleRate, ATTACK_MS);
        release = timeCoefficient (sampleRate, RELEASE_MS);
        reset();
    }

    void reset() { reductionState = 0.0f; }

    // Applies `amount` (0..1) of the effect: the gain change in dB is scaled, so amount 0 is unity.
    void process (float* interleaved, int frames, float amount)
    {
        const float makeup = makeupDb();
        for (int i = 0; i < frames; ++i)
        {
            float* frame = interleaved + 2 * i;
            const float peak = framePeak (frame);
            const float target = reductionDb (gainToDb (std::max (peak, 1.0e-6f)));
            const float coefficient = target > reductionState ? attack : release;
            reductionState = target + coefficient * (reductionState - target);
            const float gain = dbToGain (amount * (makeup - reductionState));
            frame[0] *= gain;
            frame[1] *= gain;
        }
    }

    float currentReductionDb() const { return reductionState; }

private:
    float attack = 0.0f, release = 0.0f;
    float reductionState = 0.0f;
};

class SafetyLimiter
{
public:
    static constexpr float ATTACK_MS = 0.3f;
    static constexpr float RELEASE_MS = 80.0f;
    static constexpr float KNEE = 0.92f;   // guard starts rounding peaks above this fraction of the ceiling

    void prepare (double sampleRate)
    {
        attack = timeCoefficient (sampleRate, ATTACK_MS);
        release = timeCoefficient (sampleRate, RELEASE_MS);
        reset();
    }

    void reset() { gain = 1.0f; }

    void setCeilingDb (float db) { ceiling = ceilingGain (std::min (db, 0.0f)); }
    void releaseOnly() { ceiling = RELEASE_ONLY_CEILING; } // lets the gain recover without limiting anything

    // Gain follower with a sub-millisecond attack and a smooth release, then a guard that bends and
    // finally clamps whatever the attack let through. |output| <= ceiling holds for every sample.
    void process (float* interleaved, int frames)
    {
        const float limit = ceiling;
        for (int i = 0; i < frames; ++i)
        {
            float* frame = interleaved + 2 * i;
            const float peak = framePeak (frame);
            const float target = peak > limit ? limit / peak : 1.0f;
            const float coefficient = target < gain ? attack : release;
            gain = target + coefficient * (gain - target);
            frame[0] = guard (frame[0] * gain, limit);
            frame[1] = guard (frame[1] * gain, limit);
        }
    }

    float currentGain() const { return gain; }

private:
    // Linear up to KNEE * limit, then a tanh shoulder that approaches (never passes) the limit.
    static float guard (float x, float limit)
    {
        const float knee = KNEE * limit;
        const float magnitude = std::fabs (x);
        if (magnitude <= knee)
            return x;
        const float room = limit - knee;
        const float shaped = std::min (limit, knee + room * std::tanh ((magnitude - knee) / room));
        return x < 0.0f ? -shaped : shaped;
    }

    static constexpr float RELEASE_ONLY_CEILING = 1.0e6f;

    float attack = 0.0f, release = 0.0f;
    float gain = 1.0f;
    float ceiling = 1.0f;
};

class DynamicsChain
{
public:
    void prepare (double sampleRate)
    {
        compressor.prepare (sampleRate);
        limiter.prepare (sampleRate);
        rampStep = (float) (1.0 / (sampleRate * RAMP_SECONDS));
        nightAmount = 0.0f;
        nightTarget = limiterTarget = false;
        configure (false, 0.0f);
    }

    // Audio thread. Night mode implies a -1 dBFS safety ceiling even when the user limiter is off.
    void configure (bool nightMode, float limiterCeilingDb)
    {
        const bool userLimiter = limiterCeilingDb < OFF_THRESHOLD_DB;
        float ceilingDb = userLimiter ? std::max (limiterCeilingDb, MIN_CEILING_DB) : 0.0f;
        if (nightMode)
            ceilingDb = std::min (ceilingDb, NIGHT_SAFETY_CEILING_DB);
        nightTarget = nightMode;
        limiterTarget = userLimiter || nightMode;
        if (limiterTarget)
            limiter.setCeilingDb (ceilingDb);
        else
            limiter.releaseOnly();
    }

    bool isActive() const
    {
        return nightTarget || limiterTarget || nightAmount > 0.0f || limiter.currentGain() < LIMITER_SETTLED_GAIN;
    }

    void process (float* interleaved, int frames)
    {
        if (! isActive())
            return;

        // Night mode fades in/out over RAMP_SECONDS. A limiter that gets switched off keeps
        // running in release-only mode until its gain is back at unity, then drops out.
        if (nightTarget || nightAmount > 0.0f)
        {
            int done = 0;
            while (done < frames)
            {
                const float goal = nightTarget ? 1.0f : 0.0f;
                const bool atGoal = nightTarget ? nightAmount >= 1.0f : nightAmount <= 0.0f;
                const int remaining = frames - done;
                const int steps = atGoal ? remaining : std::min (remaining, RAMP_BLOCK);
                nightAmount = moveToward (nightAmount, goal, rampStep * (float) steps);
                compressor.process (interleaved + 2 * done, steps, nightAmount);
                done += steps;
            }
        }
        if (limiterTarget || limiter.currentGain() < LIMITER_SETTLED_GAIN)
            limiter.process (interleaved, frames);
        else
            limiter.reset();
        if (! nightTarget && nightAmount <= 0.0f)
            compressor.reset();
    }

    const NightCompressor& nightCompressor() const { return compressor; }
    const SafetyLimiter& safetyLimiter() const { return limiter; }

private:
    static constexpr double RAMP_SECONDS = 0.05;
    static constexpr int RAMP_BLOCK = 32;
    static constexpr float LIMITER_SETTLED_GAIN = 0.9995f;

    static float moveToward (float value, float goal, float maxStep)
    {
        return value < goal ? std::min (goal, value + maxStep) : std::max (goal, value - maxStep);
    }

    NightCompressor compressor;
    SafetyLimiter limiter;
    float nightAmount = 0.0f, rampStep = 0.0f;
    bool nightTarget = false, limiterTarget = false;
};
} // namespace Dynamics
