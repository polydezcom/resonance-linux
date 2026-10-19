#include "core/audio/spsc_ring_buffer.h"

#include <QtTest>

#include <atomic>
#include <thread>
#include <vector>

using resonance::SpscRingBuffer;

class TestRingBuffer : public QObject
{
    Q_OBJECT

private slots:
    void roundTrip();
    void wrapAround();
    void partialWritesAndReads();
    void discardFromConsumer();
    void concurrentProducerConsumer();
};

void TestRingBuffer::roundTrip()
{
    SpscRingBuffer ring(16, 2);
    std::vector<float> input = {1, 2, 3, 4, 5, 6};
    QCOMPARE(ring.write(input.data(), 3), std::size_t(3));
    QCOMPARE(ring.availableFrames(), std::size_t(3));
    QCOMPARE(ring.freeFrames(), std::size_t(13));

    std::vector<float> output(6, 0.0f);
    QCOMPARE(ring.read(output.data(), 3), std::size_t(3));
    QCOMPARE(output, input);
    QCOMPARE(ring.availableFrames(), std::size_t(0));
}

void TestRingBuffer::wrapAround()
{
    SpscRingBuffer ring(8, 2);
    std::vector<float> first(6 * 2);
    for (int i = 0; i < 6; ++i) {
        first[i * 2] = static_cast<float>(i);
        first[i * 2 + 1] = static_cast<float>(i + 100);
    }
    QCOMPARE(ring.write(first.data(), 6), std::size_t(6));

    std::vector<float> discard(6 * 2);
    QCOMPARE(ring.read(discard.data(), 6), std::size_t(6));

    std::vector<float> second(7 * 2);
    for (int i = 0; i < 7; ++i) {
        second[i * 2] = static_cast<float>(i + 10);
        second[i * 2 + 1] = static_cast<float>(i + 200);
    }
    QCOMPARE(ring.write(second.data(), 7), std::size_t(7));

    std::vector<float> output(7 * 2);
    QCOMPARE(ring.read(output.data(), 7), std::size_t(7));
    QCOMPARE(output, second);
}

void TestRingBuffer::partialWritesAndReads()
{
    SpscRingBuffer ring(4, 2);
    std::vector<float> input(8 * 2, 3.0f);
    QCOMPARE(ring.write(input.data(), 8), std::size_t(4));
    QCOMPARE(ring.write(input.data(), 8), std::size_t(0));

    std::vector<float> output(8 * 2, 0.0f);
    QCOMPARE(ring.read(output.data(), 2), std::size_t(2));
    QCOMPARE(ring.freeFrames(), std::size_t(2));
    QCOMPARE(ring.write(input.data(), 2), std::size_t(2));
    QCOMPARE(ring.read(output.data(), 8), std::size_t(4));
    QCOMPARE(ring.availableFrames(), std::size_t(0));
}

void TestRingBuffer::discardFromConsumer()
{
    SpscRingBuffer ring(16, 1);
    std::vector<float> input = {1, 2, 3, 4, 5};
    ring.write(input.data(), input.size());
    QCOMPARE(ring.availableFrames(), std::size_t(5));

    ring.discardAllByConsumer();
    QCOMPARE(ring.availableFrames(), std::size_t(0));
    QCOMPARE(ring.read(input.data(), input.size()), std::size_t(0));
}

void TestRingBuffer::concurrentProducerConsumer()
{
    constexpr std::size_t frameCount = 200000;
    SpscRingBuffer ring(1024, 2);
    std::atomic<bool> producerDone{false};
    std::atomic<bool> sequenceOk{true};

    std::thread producer([&] {
        std::vector<float> block(17 * 2);
        std::size_t produced = 0;
        while (produced < frameCount) {
            const std::size_t count = std::min<std::size_t>(17, frameCount - produced);
            for (std::size_t i = 0; i < count; ++i) {
                const float value = static_cast<float>(produced + i);
                block[i * 2] = value;
                block[i * 2 + 1] = value + 0.5f;
            }
            std::size_t written = 0;
            while (written < count) {
                written += ring.write(block.data() + written * 2, count - written);
                if (written < count)
                    std::this_thread::yield();
            }
            produced += count;
        }
        producerDone.store(true, std::memory_order_release);
    });

    std::vector<float> block(13 * 2);
    std::size_t consumed = 0;
    while (consumed < frameCount) {
        const std::size_t count = ring.read(block.data(), 13);
        if (count == 0) {
            if (producerDone.load(std::memory_order_acquire)
                && ring.availableFrames() == 0) {
                break;
            }
            std::this_thread::yield();
            continue;
        }
        for (std::size_t i = 0; i < count; ++i) {
            const float expected = static_cast<float>(consumed + i);
            if (block[i * 2] != expected
                || block[i * 2 + 1] != expected + 0.5f) {
                sequenceOk.store(false, std::memory_order_release);
            }
        }
        consumed += count;
    }

    producer.join();
    QVERIFY(sequenceOk.load());
    QCOMPARE(consumed, frameCount);
}

QTEST_APPLESS_MAIN(TestRingBuffer)
#include "test_ring_buffer.moc"
