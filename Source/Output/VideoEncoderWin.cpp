// ============================================================================
//  VideoEncoder — Windows: Media Foundation Sink Writer → MP4 (H.264 + AAC).
//  (macOS: VideoEncoderMac.mm. Other platforms: no encoder.)
// ============================================================================
#include "VideoEncoder.h"

#if defined(_WIN32)
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <mfapi.h>
 #include <mfidl.h>
 #include <mfreadwrite.h>
 #include <mferror.h>
 #include <codecapi.h>
 #include <cstdio>
 #include <cstring>
 #include <vector>

 #pragma comment(lib, "mfplat.lib")
 #pragma comment(lib, "mfreadwrite.lib")
 #pragma comment(lib, "mfuuid.lib")
 #pragma comment(lib, "ole32.lib")

namespace dali
{
namespace
{
template <typename T> void safeRelease(T*& p) { if (p != nullptr) { p->Release(); p = nullptr; } }

std::string hrText(const std::string& what, HRESULT hr)
{
    char code[32];
    std::snprintf(code, sizeof(code), "0x%08lX", static_cast<unsigned long>(hr));
    return what + " failed (" + code + ")";
}

std::wstring widen(const char* utf8)
{
    const int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    std::wstring w(size_t(n > 0 ? n : 1), L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, utf8, -1, w.data(), n);
    return w;
}

class MediaFoundationEncoder final : public VideoEncoder
{
public:
    ~MediaFoundationEncoder() override { shutdown(); }

    bool open(const char* utf8Path, const Settings& s, std::string& error) override
    {
        settings = s;
        const HRESULT co = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        comInitialised = SUCCEEDED(co);                                   // RPC_E_CHANGED_MODE: COM already usable
        const HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
        if (FAILED(hr)) { error = hrText("MFStartup", hr); return false; }
        mfStarted = true;

        // the GPU encoder first; some drivers refuse a format the software path accepts
        if (openWriter(utf8Path, true, error)) return true;
        safeRelease(writer);
        std::string softwareError;
        if (openWriter(utf8Path, false, softwareError)) { error.clear(); return true; }
        safeRelease(writer);
        return false;
    }

private:
    bool openWriter(const char* utf8Path, bool hardware, std::string& error)
    {
        const Settings& s = settings;
        IMFAttributes* attr = nullptr;
        HRESULT hr = MFCreateAttributes(&attr, 1);
        if (SUCCEEDED(hr)) hr = attr->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, hardware ? TRUE : FALSE);
        const auto path = widen(utf8Path);
        if (SUCCEEDED(hr)) hr = MFCreateSinkWriterFromURL(path.c_str(), nullptr, attr, &writer);
        safeRelease(attr);
        if (FAILED(hr)) { error = hrText("Creating the video file", hr); return false; }

        // ---- video: H.264 out, RGB32 (BGRA, top-down) in -------------------------------------
        IMFMediaType* vOut = nullptr;
        hr = MFCreateMediaType(&vOut);
        if (SUCCEEDED(hr)) hr = vOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        if (SUCCEEDED(hr)) hr = vOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
        if (SUCCEEDED(hr)) hr = vOut->SetUINT32(MF_MT_AVG_BITRATE, UINT32(s.videoBitrate));
        if (SUCCEEDED(hr)) hr = vOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
        if (SUCCEEDED(hr)) hr = vOut->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_High);
        if (SUCCEEDED(hr)) hr = MFSetAttributeSize(vOut, MF_MT_FRAME_SIZE, UINT32(s.width), UINT32(s.height));
        if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(vOut, MF_MT_FRAME_RATE, UINT32(s.fps), 1);
        if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(vOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
        if (SUCCEEDED(hr)) hr = writer->AddStream(vOut, &videoStream);
        safeRelease(vOut);
        if (FAILED(hr)) { error = hrText("H.264 video stream", hr); return false; }

        IMFMediaType* vIn = nullptr;
        hr = MFCreateMediaType(&vIn);
        if (SUCCEEDED(hr)) hr = vIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        if (SUCCEEDED(hr)) hr = vIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        if (SUCCEEDED(hr)) hr = vIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
        if (SUCCEEDED(hr)) hr = vIn->SetUINT32(MF_MT_DEFAULT_STRIDE, UINT32(s.width * 4));      // positive = top-down
        if (SUCCEEDED(hr)) hr = MFSetAttributeSize(vIn, MF_MT_FRAME_SIZE, UINT32(s.width), UINT32(s.height));
        if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(vIn, MF_MT_FRAME_RATE, UINT32(s.fps), 1);
        if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(vIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
        if (SUCCEEDED(hr)) hr = writer->SetInputMediaType(videoStream, vIn, nullptr);
        safeRelease(vIn);
        if (FAILED(hr)) { error = hrText("Video input format (" + std::to_string(s.width) + "x" + std::to_string(s.height) + ")", hr); return false; }

        // ---- audio: AAC out, 16-bit PCM in ---------------------------------------------------
        IMFMediaType* aOut = nullptr;
        hr = MFCreateMediaType(&aOut);
        if (SUCCEEDED(hr)) hr = aOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        if (SUCCEEDED(hr)) hr = aOut->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
        if (SUCCEEDED(hr)) hr = aOut->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
        if (SUCCEEDED(hr)) hr = aOut->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, UINT32(s.sampleRate));
        if (SUCCEEDED(hr)) hr = aOut->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2);
        if (SUCCEEDED(hr)) hr = aOut->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 24000);   // 192 kbit/s
        if (SUCCEEDED(hr)) hr = writer->AddStream(aOut, &audioStream);
        safeRelease(aOut);
        if (FAILED(hr)) { error = hrText("AAC audio stream", hr); return false; }

        IMFMediaType* aIn = nullptr;
        hr = MFCreateMediaType(&aIn);
        if (SUCCEEDED(hr)) hr = aIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        if (SUCCEEDED(hr)) hr = aIn->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        if (SUCCEEDED(hr)) hr = aIn->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
        if (SUCCEEDED(hr)) hr = aIn->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, UINT32(s.sampleRate));
        if (SUCCEEDED(hr)) hr = aIn->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2);
        if (SUCCEEDED(hr)) hr = aIn->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4);
        if (SUCCEEDED(hr)) hr = aIn->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, UINT32(s.sampleRate * 4));
        if (SUCCEEDED(hr)) hr = writer->SetInputMediaType(audioStream, aIn, nullptr);
        safeRelease(aIn);
        if (FAILED(hr)) { error = hrText("Audio input format", hr); return false; }

        hr = writer->BeginWriting();
        if (FAILED(hr)) { error = hrText("Starting the encoder", hr); return false; }
        writing = true;
        return true;
    }

public:

    bool writeVideo(const std::uint8_t* bgra, std::int64_t frameIndex) override
    {
        if (!writing) return false;
        const DWORD bytes = DWORD(settings.width) * DWORD(settings.height) * 4u;
        IMFMediaBuffer* buffer = nullptr;
        HRESULT hr = MFCreateMemoryBuffer(bytes, &buffer);
        BYTE* dst = nullptr;
        if (SUCCEEDED(hr)) hr = buffer->Lock(&dst, nullptr, nullptr);
        if (SUCCEEDED(hr))
        {
            hr = MFCopyImage(dst, LONG(settings.width * 4), bgra, LONG(settings.width * 4), DWORD(settings.width * 4), DWORD(settings.height));
            buffer->Unlock();
        }
        if (SUCCEEDED(hr)) hr = buffer->SetCurrentLength(bytes);
        const LONGLONG t0 = LONGLONG(frameIndex) * 10000000LL / settings.fps;
        const LONGLONG t1 = LONGLONG(frameIndex + 1) * 10000000LL / settings.fps;
        hr = writeSample(hr, buffer, videoStream, t0, t1 - t0);
        safeRelease(buffer);
        return SUCCEEDED(hr);
    }

    bool writeAudio(const float* interleaved, int numFrames, std::int64_t startFrame) override
    {
        if (!writing || numFrames <= 0) return writing;
        pcm.resize(size_t(numFrames) * 2);
        for (size_t i = 0; i < pcm.size(); ++i)
        {
            float v = interleaved[i];
            v = v < -1.0f ? -1.0f : (v > 1.0f ? 1.0f : v);
            pcm[i] = std::int16_t(v * 32767.0f);
        }
        const DWORD bytes = DWORD(pcm.size() * sizeof(std::int16_t));
        IMFMediaBuffer* buffer = nullptr;
        HRESULT hr = MFCreateMemoryBuffer(bytes, &buffer);
        BYTE* dst = nullptr;
        if (SUCCEEDED(hr)) hr = buffer->Lock(&dst, nullptr, nullptr);
        if (SUCCEEDED(hr)) { std::memcpy(dst, pcm.data(), bytes); buffer->Unlock(); }
        if (SUCCEEDED(hr)) hr = buffer->SetCurrentLength(bytes);
        const LONGLONG t0 = LONGLONG(startFrame) * 10000000LL / settings.sampleRate;
        const LONGLONG t1 = LONGLONG(startFrame + numFrames) * 10000000LL / settings.sampleRate;
        hr = writeSample(hr, buffer, audioStream, t0, t1 - t0);
        safeRelease(buffer);
        return SUCCEEDED(hr);
    }

    bool finish(std::string& error) override
    {
        bool ok = true;
        if (writer != nullptr && writing)
        {
            const HRESULT hr = writer->Finalize();
            if (FAILED(hr)) { error = hrText("Finishing the video file", hr); ok = false; }
        }
        writing = false;
        shutdown();
        return ok;
    }

private:
    HRESULT writeSample(HRESULT hr, IMFMediaBuffer* buffer, DWORD stream, LONGLONG time, LONGLONG duration)
    {
        IMFSample* sample = nullptr;
        if (SUCCEEDED(hr)) hr = MFCreateSample(&sample);
        if (SUCCEEDED(hr)) hr = sample->AddBuffer(buffer);
        if (SUCCEEDED(hr)) hr = sample->SetSampleTime(time);
        if (SUCCEEDED(hr)) hr = sample->SetSampleDuration(duration);
        if (SUCCEEDED(hr)) hr = writer->WriteSample(stream, sample);
        safeRelease(sample);
        return hr;
    }

    void shutdown()
    {
        safeRelease(writer);
        if (mfStarted) { MFShutdown(); mfStarted = false; }
        if (comInitialised) { CoUninitialize(); comInitialised = false; }
    }

    Settings settings;
    IMFSinkWriter* writer = nullptr;
    DWORD videoStream = 0, audioStream = 0;
    bool comInitialised = false, mfStarted = false, writing = false;
    std::vector<std::int16_t> pcm;
};
} // namespace

std::unique_ptr<VideoEncoder> VideoEncoder::create() { return std::make_unique<MediaFoundationEncoder>(); }
} // namespace dali

#elif ! defined(__APPLE__)
namespace dali
{
std::unique_ptr<VideoEncoder> VideoEncoder::create() { return nullptr; }
}
#endif
