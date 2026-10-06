#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <vector>

// Single-producer / single-consumer ring of interleaved stereo float frames.
// Wait-free on both sides; all memory is allocated in the constructor.
class SpscRingBuffer
{
public:
    static constexpr int NUM_CHANNELS = 2;

    explicit SpscRingBuffer (size_t minCapacityFrames)
    {
        size_t capacity = 1;
        while (capacity < minCapacityFrames)
            capacity <<= 1;
        mask = capacity - 1;
        data.assign (capacity * NUM_CHANNELS, 0.0f);
    }

    size_t capacityFrames() const { return mask + 1; }

    // Producer side. Returns frames actually written (drops what does not fit).
    size_t write (const float* frames, size_t numFrames)
    {
        auto w = writePos.load (std::memory_order_relaxed);
        auto r = readPos.load (std::memory_order_acquire);
        numFrames = std::min (numFrames, capacityFrames() - (size_t) (w - r));
        for (size_t i = 0; i < numFrames; ++i)
        {
            auto* dst = &data[((w + i) & mask) * NUM_CHANNELS];
            dst[0] = frames[i * NUM_CHANNELS];
            dst[1] = frames[i * NUM_CHANNELS + 1];
        }
        writePos.store (w + numFrames, std::memory_order_release);
        return numFrames;
    }

    // Consumer side.
    size_t availableToRead() const
    {
        return (size_t) (writePos.load (std::memory_order_acquire) - readPos.load (std::memory_order_relaxed));
    }

    // Frame at `offset` past the read position; caller guarantees offset < availableToRead().
    const float* peek (size_t offset) const
    {
        return &data[((readPos.load (std::memory_order_relaxed) + offset) & mask) * NUM_CHANNELS];
    }

    void consume (size_t numFrames)
    {
        readPos.store (readPos.load (std::memory_order_relaxed) + numFrames, std::memory_order_release);
    }

    // Only while neither side is running.
    void reset()
    {
        writePos.store (0);
        readPos.store (0);
    }

private:
    std::vector<float> data;
    size_t mask = 0;
    alignas (64) std::atomic<size_t> writePos { 0 };
    alignas (64) std::atomic<size_t> readPos { 0 };
};
