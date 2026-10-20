#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace resonance {

struct TrackBoundary
{
    std::uint64_t framePosition = 0;
    std::int32_t queueIndex = -1;
    std::uint64_t durationFrames = 0;
};

/**
 * SPSC queue used by the decoder thread to tell the real-time callback where
 * one track ends and the next begins in the continuous sample stream.
 */
class BoundaryQueue
{
public:
    static constexpr std::size_t Capacity = 64;

    bool push(const TrackBoundary &boundary)
    {
        const std::uint32_t head = m_head.load(std::memory_order_relaxed);
        const std::uint32_t tail = m_tail.load(std::memory_order_acquire);
        if (head - tail >= Capacity)
            return false;

        m_items[head & (Capacity - 1)] = boundary;
        m_head.store(head + 1, std::memory_order_release);
        return true;
    }

    bool peek(TrackBoundary &boundary) const
    {
        const std::uint32_t tail = m_tail.load(std::memory_order_relaxed);
        const std::uint32_t head = m_head.load(std::memory_order_acquire);
        if (tail == head)
            return false;

        boundary = m_items[tail & (Capacity - 1)];
        return true;
    }

    void pop()
    {
        const std::uint32_t tail = m_tail.load(std::memory_order_relaxed);
        m_tail.store(tail + 1, std::memory_order_release);
    }

    void clearByConsumer()
    {
        const std::uint32_t head = m_head.load(std::memory_order_acquire);
        m_tail.store(head, std::memory_order_release);
    }

    std::size_t size() const
    {
        return m_head.load(std::memory_order_acquire)
            - m_tail.load(std::memory_order_acquire);
    }

private:
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "BoundaryQueue capacity must be a power of two");

    std::array<TrackBoundary, Capacity> m_items{};
    std::atomic<std::uint32_t> m_head{0};
    std::atomic<std::uint32_t> m_tail{0};
};

} // namespace resonance
