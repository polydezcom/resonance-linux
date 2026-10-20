#include "core/audio/audio_engine.h"

#include "core/audio/boundary_queue.h"
#include "core/audio/miniaudio_decoder.h"
#include "core/audio/spsc_ring_buffer.h"

#include <miniaudio.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace resonance {
namespace {

constexpr std::uint64_t kNoSeek = std::numeric_limits<std::uint64_t>::max();
constexpr std::size_t kBufferFrames = 2048;
constexpr std::size_t kRingFrames = 262144; // ~5.4 s at 48 kHz
constexpr auto kDecoderIdleSleep = std::chrono::milliseconds(2);

std::string resultMessage(const char *prefix, ma_result result)
{
    return std::string(prefix) + ": " + ma_result_description(result);
}

} // namespace

struct AudioEngine::Impl
{
    ma_device device{};
    ma_context context{};
    bool contextInitialized = false;
    bool deviceInitialized = false;
    std::uint32_t sampleRate = 48000;
    float gainSmoothing = 0.0015f;

    SpscRingBuffer ring{kRingFrames, 2};
    BoundaryQueue boundaries;

    std::mutex queueMutex;
    PlayQueue queue;

    std::thread decoderThread;
    std::atomic<bool> stopRequested{false};
    std::atomic<bool> playing{false};
    std::atomic<bool> paused{false};
    std::atomic<bool> opening{false};
    std::atomic<bool> loaded{false};
    std::atomic<bool> hasTrack{false};
    std::atomic<bool> ended{false};
    std::atomic<bool> eos{false};

    std::atomic<int> audibleIndex{-1};
    std::atomic<std::uint64_t> streamConsumed{0};
    std::atomic<std::int64_t> trackStartFrame{0};
    std::atomic<std::uint64_t> durationFrames{0};
    std::atomic<std::uint64_t> seekRequest{kNoSeek};

    std::atomic<std::uint64_t> flushRequest{0};
    std::atomic<std::uint64_t> flushAck{0};
    std::atomic<float> targetVolume{0.7f};
    std::atomic<std::uint64_t> underruns{0};

    // Owned exclusively by the audio callback.
    float currentGain = 0.0f;

    mutable std::mutex stringMutex;
    std::string error;
};

AudioEngine::AudioEngine()
    : m_impl(std::make_unique<Impl>())
{
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

bool AudioEngine::initialize(std::string *error)
{
    if (m_impl->deviceInitialized)
        return true;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 0; // native device rate
    config.periodSizeInFrames = 512;
    config.performanceProfile = ma_performance_profile_low_latency;
    config.pUserData = this;
    config.dataCallback = [](ma_device *device, void *output,
                             const void *input, ma_uint32 frameCount) {
        auto *engine = static_cast<AudioEngine *>(device->pUserData);
        engine->render(static_cast<float *>(output), frameCount);
        (void)input;
    };

    ma_context *context = nullptr;
    const char *nullAudio = std::getenv("RESONANCE_NULL_AUDIO");
    if (nullAudio && std::strcmp(nullAudio, "0") != 0) {
        ma_backend backends[] = { ma_backend_null };
        ma_context_config contextConfig = ma_context_config_init();
        const ma_result contextResult = ma_context_init(
            backends, 1, &contextConfig, &m_impl->context);
        if (contextResult != MA_SUCCESS) {
            const std::string message = resultMessage(
                "Could not initialize the null audio backend", contextResult);
            setError(message);
            if (error)
                *error = message;
            return false;
        }
        m_impl->contextInitialized = true;
        context = &m_impl->context;
    }

    const ma_result result =
        ma_device_init(context, &config, &m_impl->device);
    if (result != MA_SUCCESS) {
        const std::string message = resultMessage("No audio device available", result);
        if (m_impl->contextInitialized) {
            ma_context_uninit(&m_impl->context);
            m_impl->contextInitialized = false;
        }
        setError(message);
        if (error)
            *error = message;
        return false;
    }

    m_impl->sampleRate =
        m_impl->device.sampleRate > 0 ? m_impl->device.sampleRate : 48000;
    m_impl->gainSmoothing = 1.0f
        - std::exp(-1.0f / (0.012f * static_cast<float>(m_impl->sampleRate)));
    m_impl->deviceInitialized = true;

    const ma_result startResult = ma_device_start(&m_impl->device);
    if (startResult != MA_SUCCESS) {
        const std::string message = resultMessage("Could not start audio device", startResult);
        ma_device_uninit(&m_impl->device);
        m_impl->deviceInitialized = false;
        if (m_impl->contextInitialized) {
            ma_context_uninit(&m_impl->context);
            m_impl->contextInitialized = false;
        }
        setError(message);
        if (error)
            *error = message;
        return false;
    }

    return true;
}

void AudioEngine::shutdown()
{
    if (!m_impl)
        return;

    stopDecoderThread();
    if (m_impl->deviceInitialized) {
        requestFlush();
        ma_device_uninit(&m_impl->device);
        m_impl->deviceInitialized = false;
    }
    if (m_impl->contextInitialized) {
        ma_context_uninit(&m_impl->context);
        m_impl->contextInitialized = false;
    }
}

bool AudioEngine::isInitialized() const
{
    return m_impl && m_impl->deviceInitialized;
}

void AudioEngine::setQueue(const std::vector<std::string> &paths,
                           int startIndex, double startPositionSeconds,
                           bool autoPlay)
{
    if (!m_impl->deviceInitialized) {
        setError("No audio device is available.");
        return;
    }

    stopDecoderThread();
    requestFlush();

    {
        std::lock_guard<std::mutex> lock(m_impl->queueMutex);
        m_impl->queue.setItems(paths);
    }

    if (paths.empty()) {
        m_impl->hasTrack.store(false, std::memory_order_release);
        m_impl->opening.store(false, std::memory_order_release);
        m_impl->loaded.store(false, std::memory_order_release);
        m_impl->playing.store(false, std::memory_order_release);
        m_impl->paused.store(false, std::memory_order_release);
        m_impl->ended.store(false, std::memory_order_release);
        m_impl->eos.store(false, std::memory_order_release);
        m_impl->audibleIndex.store(-1, std::memory_order_release);
        m_impl->durationFrames.store(0, std::memory_order_release);
        m_impl->streamConsumed.store(0, std::memory_order_release);
        m_impl->trackStartFrame.store(0, std::memory_order_release);
        m_impl->seekRequest.store(kNoSeek, std::memory_order_release);
        return;
    }

    const int index = std::clamp(startIndex, 0, static_cast<int>(paths.size()) - 1);
    std::uint64_t startFrame = 0;
    if (startPositionSeconds > 0.0) {
        startFrame = static_cast<std::uint64_t>(
            std::llround(startPositionSeconds * m_impl->sampleRate));
    }

    startAtIndex(index, startFrame, autoPlay);
}

void AudioEngine::clearQueue()
{
    setQueue({}, -1, 0.0, false);
}

void AudioEngine::playFile(const std::string &path)
{
    setQueue({path}, 0, 0.0, true);
}

void AudioEngine::playQueueIndex(int index)
{
    startAtIndex(index, 0, true);
}

void AudioEngine::next()
{
    if (!m_impl->hasTrack.load(std::memory_order_acquire))
        return;

    const int audible = currentIndex();
    int target = -1;
    {
        std::lock_guard<std::mutex> lock(m_impl->queueMutex);
        target = m_impl->queue.peekAdvanceFrom(audible, true);
    }
    if (target >= 0)
        startAtIndex(target, 0, true);
}

void AudioEngine::previous()
{
    if (!m_impl->hasTrack.load(std::memory_order_acquire))
        return;

    const int audible = currentIndex();
    int target = -1;
    {
        std::lock_guard<std::mutex> lock(m_impl->queueMutex);
        target = m_impl->queue.peekRewindFrom(audible, true);
    }
    if (target < 0)
        target = audible;
    if (target >= 0)
        startAtIndex(target, 0, true);
}

void AudioEngine::setShuffle(bool shuffle)
{
    std::lock_guard<std::mutex> lock(m_impl->queueMutex);
    m_impl->queue.setShuffle(shuffle);
}

bool AudioEngine::shuffle() const
{
    std::lock_guard<std::mutex> lock(m_impl->queueMutex);
    return m_impl->queue.shuffle();
}

void AudioEngine::setRepeatMode(RepeatMode mode)
{
    std::lock_guard<std::mutex> lock(m_impl->queueMutex);
    m_impl->queue.setRepeatMode(mode);
}

RepeatMode AudioEngine::repeatMode() const
{
    std::lock_guard<std::mutex> lock(m_impl->queueMutex);
    return m_impl->queue.repeatMode();
}

int AudioEngine::currentIndex() const
{
    return m_impl->audibleIndex.load(std::memory_order_acquire);
}

int AudioEngine::queueSize() const
{
    std::lock_guard<std::mutex> lock(m_impl->queueMutex);
    return m_impl->queue.size();
}

std::string AudioEngine::queuePathAt(int index) const
{
    std::lock_guard<std::mutex> lock(m_impl->queueMutex);
    if (index < 0 || index >= m_impl->queue.size())
        return {};
    return m_impl->queue.items()[static_cast<std::size_t>(index)];
}

void AudioEngine::startAtIndex(int index, std::uint64_t startFrame, bool autoPlay)
{
    if (!m_impl->deviceInitialized) {
        setError("No audio device is available.");
        return;
    }

    stopDecoderThread();
    requestFlush();

    {
        std::lock_guard<std::mutex> lock(m_impl->queueMutex);
        if (index < 0 || index >= m_impl->queue.size())
            return;
        m_impl->queue.setCurrentIndex(index);
    }

    m_impl->hasTrack.store(true, std::memory_order_release);
    m_impl->opening.store(true, std::memory_order_release);
    m_impl->loaded.store(false, std::memory_order_release);
    m_impl->playing.store(false, std::memory_order_release);
    m_impl->paused.store(!autoPlay, std::memory_order_release);
    m_impl->ended.store(false, std::memory_order_release);
    m_impl->eos.store(false, std::memory_order_release);
    m_impl->audibleIndex.store(index, std::memory_order_release);
    m_impl->durationFrames.store(0, std::memory_order_release);
    m_impl->streamConsumed.store(0, std::memory_order_release);
    m_impl->trackStartFrame.store(-static_cast<std::int64_t>(startFrame),
                                  std::memory_order_release);
    m_impl->seekRequest.store(kNoSeek, std::memory_order_release);
    m_impl->stopRequested.store(false, std::memory_order_release);

    m_impl->decoderThread = std::thread(&AudioEngine::decodeLoop, this, startFrame);
}

void AudioEngine::pause()
{
    if (!m_impl->hasTrack.load(std::memory_order_acquire))
        return;

    m_impl->paused.store(true, std::memory_order_release);
    m_impl->playing.store(false, std::memory_order_release);
}

void AudioEngine::resume()
{
    if (!m_impl->hasTrack.load(std::memory_order_acquire))
        return;

    const int index = currentIndex();
    if (index < 0)
        return;

    if (m_impl->opening.load(std::memory_order_acquire)) {
        m_impl->paused.store(false, std::memory_order_release);
        return;
    }

    if (m_impl->ended.load(std::memory_order_acquire)
        || !m_impl->loaded.load(std::memory_order_acquire)) {
        startAtIndex(index, 0, true);
        return;
    }

    m_impl->paused.store(false, std::memory_order_release);
    m_impl->playing.store(true, std::memory_order_release);
}

void AudioEngine::togglePause()
{
    if (!m_impl->hasTrack.load(std::memory_order_acquire))
        return;

    if (m_impl->paused.load(std::memory_order_acquire)
        || !m_impl->playing.load(std::memory_order_acquire)) {
        resume();
    } else {
        pause();
    }
}

void AudioEngine::stop()
{
    stopDecoderThread();
    requestFlush();

    m_impl->playing.store(false, std::memory_order_release);
    m_impl->paused.store(false, std::memory_order_release);
    m_impl->opening.store(false, std::memory_order_release);
    m_impl->loaded.store(false, std::memory_order_release);
    m_impl->ended.store(false, std::memory_order_release);
    m_impl->eos.store(false, std::memory_order_release);
    m_impl->streamConsumed.store(0, std::memory_order_release);
    m_impl->trackStartFrame.store(0, std::memory_order_release);
    m_impl->seekRequest.store(kNoSeek, std::memory_order_release);
}

void AudioEngine::seekToSeconds(double seconds)
{
    if (m_impl->sampleRate == 0)
        return;

    const int index = currentIndex();
    if (!m_impl->hasTrack.load(std::memory_order_acquire) || index < 0)
        return;

    const std::uint64_t duration =
        m_impl->durationFrames.load(std::memory_order_acquire);
    const double clamped = std::max(0.0, seconds);
    std::uint64_t frame = static_cast<std::uint64_t>(
        std::llround(clamped * static_cast<double>(m_impl->sampleRate)));
    if (duration > 0)
        frame = std::min(frame, duration);

    const bool ended = m_impl->ended.load(std::memory_order_acquire);
    const bool active = m_impl->loaded.load(std::memory_order_acquire)
        || m_impl->opening.load(std::memory_order_acquire);

    if (ended || !active) {
        startAtIndex(index, frame, true);
        return;
    }

    m_impl->seekRequest.store(frame, std::memory_order_release);
}

void AudioEngine::setVolume(float volume)
{
    m_impl->targetVolume.store(std::clamp(volume, 0.0f, 1.0f),
                               std::memory_order_release);
}

bool AudioEngine::isPlaying() const
{
    return m_impl->playing.load(std::memory_order_acquire);
}

bool AudioEngine::isPaused() const
{
    return m_impl->paused.load(std::memory_order_acquire);
}

bool AudioEngine::isLoaded() const
{
    return m_impl->loaded.load(std::memory_order_acquire);
}

bool AudioEngine::hasTrack() const
{
    return m_impl->hasTrack.load(std::memory_order_acquire);
}

bool AudioEngine::hasEnded() const
{
    return m_impl->ended.load(std::memory_order_acquire);
}

double AudioEngine::positionSeconds() const
{
    if (m_impl->sampleRate == 0)
        return 0.0;

    if (hasEnded())
        return durationSeconds();

    const std::int64_t consumed = static_cast<std::int64_t>(
        m_impl->streamConsumed.load(std::memory_order_acquire));
    const std::int64_t start =
        m_impl->trackStartFrame.load(std::memory_order_acquire);
    std::int64_t frames = consumed - start;
    if (frames < 0)
        frames = 0;

    double seconds = static_cast<double>(frames)
        / static_cast<double>(m_impl->sampleRate);
    const double total = durationSeconds();
    if (total > 0.0)
        seconds = std::min(seconds, total);
    return seconds;
}

double AudioEngine::durationSeconds() const
{
    if (m_impl->sampleRate == 0)
        return 0.0;
    return static_cast<double>(m_impl->durationFrames.load(std::memory_order_acquire))
        / static_cast<double>(m_impl->sampleRate);
}

std::uint32_t AudioEngine::sampleRate() const
{
    return m_impl->sampleRate;
}

float AudioEngine::volume() const
{
    return m_impl->targetVolume.load(std::memory_order_acquire);
}

std::string AudioEngine::currentPath() const
{
    return queuePathAt(currentIndex());
}

std::string AudioEngine::takeError()
{
    std::lock_guard<std::mutex> lock(m_impl->stringMutex);
    std::string error = std::move(m_impl->error);
    m_impl->error.clear();
    return error;
}

std::uint64_t AudioEngine::underrunCount() const
{
    return m_impl->underruns.load(std::memory_order_relaxed);
}

void AudioEngine::render(float *output, std::uint32_t frameCount)
{
    const std::size_t sampleCount = static_cast<std::size_t>(frameCount) * 2;
    std::fill_n(output, sampleCount, 0.0f);

    // A flush request is used for seeks, queue jumps and track changes. The
    // decoder waits for this acknowledgement before writing new samples.
    const std::uint64_t requested = m_impl->flushRequest.load(std::memory_order_acquire);
    if (requested != m_impl->flushAck.load(std::memory_order_relaxed)) {
        m_impl->ring.discardAllByConsumer();
        m_impl->boundaries.clearByConsumer();
        m_impl->streamConsumed.store(0, std::memory_order_release);
        m_impl->currentGain = 0.0f;
        m_impl->flushAck.store(requested, std::memory_order_release);
        return;
    }

    if (!m_impl->playing.load(std::memory_order_relaxed)
        || m_impl->paused.load(std::memory_order_relaxed)) {
        return;
    }

    const std::size_t framesRead = m_impl->ring.read(output, frameCount);
    if (framesRead == 0 && m_impl->eos.load(std::memory_order_acquire)) {
        m_impl->ended.store(true, std::memory_order_release);
        m_impl->playing.store(false, std::memory_order_release);
        return;
    }

    if (framesRead < frameCount && !m_impl->eos.load(std::memory_order_acquire))
        m_impl->underruns.fetch_add(1, std::memory_order_relaxed);

    if (framesRead == 0)
        return;

    const float target = m_impl->targetVolume.load(std::memory_order_relaxed);
    const float smoothing = m_impl->gainSmoothing;
    float gain = m_impl->currentGain;
    const std::size_t count = framesRead * 2;
    for (std::size_t i = 0; i < count; ++i) {
        gain += (target - gain) * smoothing;
        if (std::abs(gain) < 1.0e-8f)
            gain = 0.0f;
        output[i] *= gain;
    }
    m_impl->currentGain = gain;

    const std::uint64_t consumed = m_impl->streamConsumed.fetch_add(
        framesRead, std::memory_order_acq_rel) + framesRead;

    TrackBoundary boundary;
    while (m_impl->boundaries.peek(boundary)
           && boundary.framePosition <= consumed) {
        m_impl->boundaries.pop();
        m_impl->audibleIndex.store(boundary.queueIndex, std::memory_order_release);
        m_impl->durationFrames.store(boundary.durationFrames,
                                     std::memory_order_release);
        m_impl->trackStartFrame.store(
            static_cast<std::int64_t>(boundary.framePosition),
            std::memory_order_release);
    }
}

void AudioEngine::decodeLoop(std::uint64_t initialSeekFrame)
{
    std::string path;
    {
        std::lock_guard<std::mutex> lock(m_impl->queueMutex);
        path = m_impl->queue.currentPath();
    }

    auto decoder = std::make_unique<MiniaudioDecoder>();
    std::string error;
    if (!decoder->open(path, m_impl->sampleRate, &error)) {
        setError(error.empty() ? "Could not open audio file." : error);
        m_impl->opening.store(false, std::memory_order_release);
        m_impl->loaded.store(false, std::memory_order_release);
        m_impl->playing.store(false, std::memory_order_release);
        return;
    }

    DecoderInfo info = decoder->info();
    if (info.totalFrames > 0 && initialSeekFrame > info.totalFrames)
        initialSeekFrame = info.totalFrames;
    if (initialSeekFrame > 0) {
        if (!decoder->seek(initialSeekFrame)) {
            setError("The audio file does not support seeking.");
            initialSeekFrame = 0;
        } else {
            m_impl->trackStartFrame.store(
                -static_cast<std::int64_t>(initialSeekFrame),
                std::memory_order_release);
        }
    }

    m_impl->durationFrames.store(info.totalFrames, std::memory_order_release);
    m_impl->loaded.store(true, std::memory_order_release);
    m_impl->opening.store(false, std::memory_order_release);

    if (!m_impl->paused.load(std::memory_order_acquire)
        && !m_impl->stopRequested.load(std::memory_order_acquire)) {
        m_impl->playing.store(true, std::memory_order_release);
    }

    std::vector<float> buffer(kBufferFrames * 2);
    std::uint64_t produced = 0;

    while (!m_impl->stopRequested.load(std::memory_order_acquire)) {
        const std::uint64_t requestedSeek =
            m_impl->seekRequest.exchange(kNoSeek, std::memory_order_acq_rel);
        if (requestedSeek != kNoSeek) {
            if (!requestFlush())
                break;

            std::uint64_t target = requestedSeek;
            const DecoderInfo currentInfo = decoder->info();
            if (currentInfo.totalFrames > 0)
                target = std::min(target, currentInfo.totalFrames);

            if (decoder->seek(target)) {
                produced = 0;
                m_impl->trackStartFrame.store(
                    -static_cast<std::int64_t>(target),
                    std::memory_order_release);
                m_impl->ended.store(false, std::memory_order_release);
                m_impl->eos.store(false, std::memory_order_release);
            } else {
                setError("The audio file does not support seeking.");
            }
        }

        if (m_impl->paused.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(kDecoderIdleSleep);
            continue;
        }

        const std::size_t freeFrames = m_impl->ring.freeFrames();
        if (freeFrames == 0) {
            std::this_thread::sleep_for(kDecoderIdleSleep);
            continue;
        }

        const std::size_t want = std::min(freeFrames, kBufferFrames);
        const std::size_t framesRead = decoder->read(buffer.data(), want);
        if (framesRead == 0) {
            int nextIndex = -1;
            std::string nextPath;
            {
                std::lock_guard<std::mutex> lock(m_impl->queueMutex);
                nextIndex = m_impl->queue.advance(false);
                if (nextIndex >= 0)
                    nextPath = m_impl->queue.currentPath();
            }

            if (nextIndex < 0) {
                m_impl->eos.store(true, std::memory_order_release);
                break;
            }

            auto nextDecoder = std::make_unique<MiniaudioDecoder>();
            std::string nextError;
            if (!nextDecoder->open(nextPath, m_impl->sampleRate, &nextError)) {
                setError(nextError.empty() ? "Could not open the next track."
                                            : nextError);
                m_impl->eos.store(true, std::memory_order_release);
                break;
            }

            const DecoderInfo nextInfo = nextDecoder->info();
            TrackBoundary boundary;
            boundary.framePosition = produced;
            boundary.queueIndex = nextIndex;
            boundary.durationFrames = nextInfo.totalFrames;

            while (!m_impl->boundaries.push(boundary)) {
                if (m_impl->stopRequested.load(std::memory_order_acquire))
                    break;
                std::this_thread::sleep_for(kDecoderIdleSleep);
            }
            if (m_impl->stopRequested.load(std::memory_order_acquire))
                break;

            decoder = std::move(nextDecoder);
            continue;
        }

        std::size_t written = 0;
        while (written < framesRead
               && !m_impl->stopRequested.load(std::memory_order_acquire)) {
            const std::size_t count = m_impl->ring.write(
                buffer.data() + written * 2, framesRead - written);
            written += count;
            produced += count;
            if (count == 0)
                std::this_thread::sleep_for(kDecoderIdleSleep);
        }
    }
}

void AudioEngine::stopDecoderThread()
{
    if (!m_impl->decoderThread.joinable())
        return;

    m_impl->stopRequested.store(true, std::memory_order_release);
    m_impl->decoderThread.join();
    m_impl->stopRequested.store(false, std::memory_order_release);
}

bool AudioEngine::requestFlush()
{
    if (!m_impl->deviceInitialized)
        return false;

    const std::uint64_t generation =
        m_impl->flushRequest.fetch_add(1, std::memory_order_acq_rel) + 1;

    const auto deadline = std::chrono::steady_clock::now()
        + std::chrono::milliseconds(250);
    while (m_impl->flushAck.load(std::memory_order_acquire) != generation) {
        if (m_impl->stopRequested.load(std::memory_order_acquire))
            return false;
        if (std::chrono::steady_clock::now() >= deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

void AudioEngine::setError(const std::string &error)
{
    std::lock_guard<std::mutex> lock(m_impl->stringMutex);
    m_impl->error = error;
}

} // namespace resonance
