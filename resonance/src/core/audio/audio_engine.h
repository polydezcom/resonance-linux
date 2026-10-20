#pragma once

#include "core/model/play_queue.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace resonance {

/**
 * Pull-based playback engine with a gapless queue.
 *
 * Control methods are called from the UI thread. They only touch atomics or
 * stop the decoder thread; the real-time miniaudio callback never blocks and
 * never calls into Qt.
 */
class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine &) = delete;
    AudioEngine &operator=(const AudioEngine &) = delete;

    bool initialize(std::string *error = nullptr);
    void shutdown();
    bool isInitialized() const;

    /// Replace the queue. startPositionSeconds is only meaningful on restore.
    void setQueue(const std::vector<std::string> &paths, int startIndex,
                  double startPositionSeconds = 0.0, bool autoPlay = true);
    void clearQueue();

    /// Convenience for dropping a single file.
    void playFile(const std::string &path);
    void playQueueIndex(int index);
    void next();
    void previous();

    void setShuffle(bool shuffle);
    bool shuffle() const;
    void setRepeatMode(RepeatMode mode);
    RepeatMode repeatMode() const;

    int currentIndex() const;
    int queueSize() const;
    std::string queuePathAt(int index) const;

    void pause();
    void resume();
    void togglePause();
    void stop();
    void seekToSeconds(double seconds);
    void setVolume(float volume);

    bool isPlaying() const;
    bool isPaused() const;
    bool isLoaded() const;
    bool hasTrack() const;
    bool hasEnded() const;

    double positionSeconds() const;
    double durationSeconds() const;
    std::uint32_t sampleRate() const;
    float volume() const;
    std::string currentPath() const;

    /// Returns and clears the last asynchronous error (for example a failed
    /// decoder open on the decoder thread).
    std::string takeError();

    std::uint64_t underrunCount() const;

private:
    void startAtIndex(int index, std::uint64_t startFrame, bool autoPlay);
    void render(float *output, std::uint32_t frameCount);
    void decodeLoop(std::uint64_t initialSeekFrame);
    void stopDecoderThread();
    bool requestFlush();
    void setError(const std::string &error);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace resonance
