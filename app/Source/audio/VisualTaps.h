#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>

// Post-DSP taps written by the audio thread and read lock-free by the UI.
// Readers may see a partially updated block; acceptable for a visualiser.
class VisualTaps
{
public:
    static constexpr int NUM_SPECTRUM_BANDS = 10;
    static constexpr int SCOPE_FRAMES = 4096; // power of two

    void writeSpectrum (const float* bands)
    {
        for (int i = 0; i < NUM_SPECTRUM_BANDS; ++i)
            spectrum[(size_t) i].store (bands[i], std::memory_order_relaxed);
    }

    void readSpectrum (float* bands) const
    {
        for (int i = 0; i < NUM_SPECTRUM_BANDS; ++i)
            bands[i] = spectrum[(size_t) i].load (std::memory_order_relaxed);
    }

    void writeScope (const float* interleaved, int numFrames)
    {
        auto w = written.load (std::memory_order_relaxed);
        for (int i = 0; i < numFrames; ++i, ++w)
        {
            auto slot = (size_t) (w & (SCOPE_FRAMES - 1));
            scopeLeft[slot].store (interleaved[2 * i], std::memory_order_relaxed);
            scopeRight[slot].store (interleaved[2 * i + 1], std::memory_order_relaxed);
        }
        written.store (w, std::memory_order_release);
    }

    int readScope (float* left, float* right, int maxFrames) const
    {
        const auto end = written.load (std::memory_order_acquire);
        const auto count = (int) std::min<uint64_t> ({ (uint64_t) std::max (maxFrames, 0), end, (uint64_t) SCOPE_FRAMES });
        auto start = end - (uint64_t) count;
        for (int i = 0; i < count; ++i, ++start)
        {
            auto slot = (size_t) (start & (SCOPE_FRAMES - 1));
            left[i] = scopeLeft[slot].load (std::memory_order_relaxed);
            right[i] = scopeRight[slot].load (std::memory_order_relaxed);
        }
        return count;
    }

    // Only while the writer (capture IOProc) is stopped.
    void clear()
    {
        for (auto& band : spectrum)
            band.store (0.0f, std::memory_order_relaxed);
        written.store (0, std::memory_order_release);
        for (size_t i = 0; i < (size_t) SCOPE_FRAMES; ++i)
        {
            scopeLeft[i].store (0.0f, std::memory_order_relaxed);
            scopeRight[i].store (0.0f, std::memory_order_relaxed);
        }
    }

private:
    std::array<std::atomic<float>, NUM_SPECTRUM_BANDS> spectrum {};
    std::array<std::atomic<float>, SCOPE_FRAMES> scopeLeft {}, scopeRight {};
    std::atomic<uint64_t> written { 0 };
};
