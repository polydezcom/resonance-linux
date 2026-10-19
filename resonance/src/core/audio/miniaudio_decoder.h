#pragma once

#include "core/audio/decoder.h"

#include <miniaudio.h>

namespace resonance {

class MiniaudioDecoder final : public IDecoder
{
public:
    MiniaudioDecoder();
    ~MiniaudioDecoder() override;

    MiniaudioDecoder(const MiniaudioDecoder &) = delete;
    MiniaudioDecoder &operator=(const MiniaudioDecoder &) = delete;

    bool open(const std::string &path, std::uint32_t outputSampleRate,
              std::string *error) override;
    void close() override;

    std::size_t read(float *output, std::size_t frameCount) override;
    bool seek(std::uint64_t frame) override;

    DecoderInfo info() const override;

private:
    ma_decoder m_decoder{};
    DecoderInfo m_info;
    bool m_open = false;
};

} // namespace resonance
