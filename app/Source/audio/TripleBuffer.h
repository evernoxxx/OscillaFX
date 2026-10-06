#pragma once

#include <array>
#include <atomic>

// Lock-free latest-value mailbox: one writer thread publishes, one reader thread
// picks up the newest value. T must be trivially copyable so neither side allocates.
template <typename T>
class TripleBuffer
{
public:
    void publish (const T& value)
    {
        slots[(size_t) back] = value;
        back = middle.exchange (back | DIRTY_BIT, std::memory_order_acq_rel) & INDEX_MASK;
    }

    // Returns true and updates `out` when a newer value was published.
    bool fetch (T& out)
    {
        if ((middle.load (std::memory_order_relaxed) & DIRTY_BIT) == 0)
            return false;
        front = middle.exchange (front, std::memory_order_acq_rel) & INDEX_MASK;
        out = slots[(size_t) front];
        return true;
    }

private:
    static constexpr int DIRTY_BIT = 4;
    static constexpr int INDEX_MASK = 3;

    std::array<T, 3> slots {};
    int back = 0;               // writer only
    int front = 1;              // reader only
    std::atomic<int> middle { 2 };
};
