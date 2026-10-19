#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace resonance {

/**
 * Pull-based playback engine.
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

    void playFile(const std::string &path);
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
    void startPlayback(const std::string &path, std::uint64_t startFrame);
    void render(float *output, std::uint32_t frameCount);
    void decodeLoop(std::string path);
    void stopDecoderThread();
    bool requestFlush();
    void setError(const std::string &error);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace resonance
