#pragma once

#include <cstdint>
#include <string>

namespace resonance {

struct DecoderInfo
{
    std::uint32_t sampleRate = 0;
    std::uint32_t channels = 0;
    std::uint64_t totalFrames = 0;
    double durationSeconds = 0.0;
};

/**
 * Non-real-time decoder interface. Implementations are created and used only
 * by the decoder thread; the audio callback never touches a decoder.
 */
class IDecoder
{
public:
    virtual ~IDecoder() = default;

    virtual bool open(const std::string &path, std::uint32_t outputSampleRate,
                      std::string *error) = 0;
    virtual void close() = 0;

    /// Reads up to @p frameCount interleaved stereo frames. Returns 0 at EOF.
    virtual std::size_t read(float *output, std::size_t frameCount) = 0;
    virtual bool seek(std::uint64_t frame) = 0;

    virtual DecoderInfo info() const = 0;
};

} // namespace resonance
