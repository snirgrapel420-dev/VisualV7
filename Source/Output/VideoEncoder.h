#pragma once
// ============================================================================
//  VideoEncoder — platform H.264 + AAC writer into an .mp4 file.
//   Windows  Media Foundation Sink Writer (hardware encoder when the GPU has one)
//   macOS    AVAssetWriter (VideoToolbox hardware encoder)
//  JUCE-free on purpose: the platform files include only their system headers.
//  Used from ONE thread only (the recorder's writer thread).
//
//  Video: BGRA 8-bit, rows top-down, exactly width*4 bytes per row.
//  Audio: interleaved stereo float, -1..1, at Settings::sampleRate
//         (44100 or 48000: the rates every AAC encoder accepts).
// ============================================================================
#include <cstdint>
#include <memory>
#include <string>

namespace dali
{
class VideoEncoder
{
public:
    struct Settings
    {
        int width = 1920, height = 1080;     // even numbers
        int fps = 60;
        int videoBitrate = 20000000;         // bits per second
        int sampleRate = 48000;
        int audioBitrate = 192000;
    };

    virtual ~VideoEncoder() = default;

    /** utf8Path: the .mp4 to create (overwritten). */
    virtual bool open(const char* utf8Path, const Settings& settings, std::string& error) = 0;
    /** frameIndex: presentation time = frameIndex / fps. Must increase. */
    virtual bool writeVideo(const std::uint8_t* bgraTopDown, std::int64_t frameIndex) = 0;
    /** startFrame: presentation time = startFrame / sampleRate (in sample frames). */
    virtual bool writeAudio(const float* interleavedStereo, int numFrames, std::int64_t startFrame) = 0;
    /** Finalises the file (writes the index). Returns false with an error when the file is not usable. */
    virtual bool finish(std::string& error) = 0;

    /** nullptr when this platform has no encoder. */
    static std::unique_ptr<VideoEncoder> create();
};
} // namespace dali
