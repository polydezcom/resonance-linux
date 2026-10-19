#include "core/audio/miniaudio_decoder.h"

#include <algorithm>

namespace resonance {

MiniaudioDecoder::MiniaudioDecoder() = default;

MiniaudioDecoder::~MiniaudioDecoder()
{
    close();
}

bool MiniaudioDecoder::open(const std::string &path,
                            std::uint32_t outputSampleRate,
                            std::string *error)
{
    close();

    ma_decoder_config config =
        ma_decoder_config_init(ma_format_f32, 2, outputSampleRate);
    const ma_result result =
        ma_decoder_init_file(path.c_str(), &config, &m_decoder);
    if (result != MA_SUCCESS) {
        if (error) {
            *error = "Could not open audio file: "
                + std::string(ma_result_description(result));
        }
        return false;
    }

    m_open = true;
    m_info.sampleRate = outputSampleRate;
    m_info.channels = 2;

    ma_uint64 length = 0;
    if (ma_decoder_get_length_in_pcm_frames(&m_decoder, &length) == MA_SUCCESS) {
        m_info.totalFrames = length;
        m_info.durationSeconds =
            outputSampleRate > 0
            ? static_cast<double>(length) / outputSampleRate
            : 0.0;
    }

    return true;
}

void MiniaudioDecoder::close()
{
    if (m_open) {
        ma_decoder_uninit(&m_decoder);
        m_open = false;
    }
    m_info = {};
}

std::size_t MiniaudioDecoder::read(float *output, std::size_t frameCount)
{
    if (!m_open || frameCount == 0)
        return 0;

    ma_uint64 framesRead = 0;
    const ma_result result = ma_decoder_read_pcm_frames(
        &m_decoder, output, static_cast<ma_uint64>(frameCount), &framesRead);
    if (result != MA_SUCCESS && result != MA_AT_END)
        return 0;

    return static_cast<std::size_t>(framesRead);
}

bool MiniaudioDecoder::seek(std::uint64_t frame)
{
    if (!m_open)
        return false;

    return ma_decoder_seek_to_pcm_frame(&m_decoder, frame) == MA_SUCCESS;
}

DecoderInfo MiniaudioDecoder::info() const
{
    return m_info;
}

} // namespace resonance
