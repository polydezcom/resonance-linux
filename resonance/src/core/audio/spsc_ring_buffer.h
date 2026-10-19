#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <vector>

namespace resonance {

/**
 * Single-producer / single-consumer lock-free ring buffer for interleaved
 * float audio frames.
 *
 * The producer is normally the decoder thread and the consumer is the
 * real-time audio callback. No operation in this class allocates or blocks.
 */
class SpscRingBuffer
{
public:
    SpscRingBuffer(std::size_t capacityFrames, std::size_t channels)
        : m_capacityFrames(capacityFrames)
        , m_channels(channels)
        , m_data(capacityFrames * channels, 0.0f)
    {
    }

    SpscRingBuffer(const SpscRingBuffer &) = delete;
    SpscRingBuffer &operator=(const SpscRingBuffer &) = delete;

    std::size_t write(const float *frames, std::size_t frameCount)
    {
        const std::size_t readIndex = m_readIndex.load(std::memory_order_acquire);
        const std::size_t writeIndex = m_writeIndex.load(std::memory_order_relaxed);
        const std::size_t freeFrames = m_capacityFrames - (writeIndex - readIndex);
        const std::size_t count = std::min(frameCount, freeFrames);
        if (count == 0)
            return 0;

        const std::size_t start = writeIndex % m_capacityFrames;
        const std::size_t firstFrames = std::min(count, m_capacityFrames - start);
        std::copy_n(frames, firstFrames * m_channels,
                    m_data.data() + start * m_channels);
        if (firstFrames < count) {
            std::copy_n(frames + firstFrames * m_channels,
                        (count - firstFrames) * m_channels,
                        m_data.data());
        }

        m_writeIndex.store(writeIndex + count, std::memory_order_release);
        return count;
    }

    std::size_t read(float *frames, std::size_t frameCount)
    {
        const std::size_t writeIndex = m_writeIndex.load(std::memory_order_acquire);
        const std::size_t readIndex = m_readIndex.load(std::memory_order_relaxed);
        const std::size_t availableFrames = writeIndex - readIndex;
        const std::size_t count = std::min(frameCount, availableFrames);
        if (count == 0)
            return 0;

        const std::size_t start = readIndex % m_capacityFrames;
        const std::size_t firstFrames = std::min(count, m_capacityFrames - start);
        std::copy_n(m_data.data() + start * m_channels,
                    firstFrames * m_channels, frames);
        if (firstFrames < count) {
            std::copy_n(m_data.data(),
                        (count - firstFrames) * m_channels,
                        frames + firstFrames * m_channels);
        }

        m_readIndex.store(readIndex + count, std::memory_order_release);
        return count;
    }

    std::size_t availableFrames() const
    {
        const std::size_t writeIndex = m_writeIndex.load(std::memory_order_acquire);
        const std::size_t readIndex = m_readIndex.load(std::memory_order_acquire);
        return writeIndex - readIndex;
    }

    std::size_t freeFrames() const
    {
        return m_capacityFrames - availableFrames();
    }

    std::size_t capacityFrames() const { return m_capacityFrames; }
    std::size_t channels() const { return m_channels; }

    /**
     * Discard everything currently queued. Only safe to call from the
     * consumer while the producer is paused (for example during a seek
     * handshake).
     */
    void discardAllByConsumer()
    {
        const std::size_t writeIndex = m_writeIndex.load(std::memory_order_acquire);
        m_readIndex.store(writeIndex, std::memory_order_release);
    }

private:
    const std::size_t m_capacityFrames;
    const std::size_t m_channels;
    std::vector<float> m_data;
    std::atomic<std::size_t> m_readIndex{0};
    std::atomic<std::size_t> m_writeIndex{0};
};

} // namespace resonance
