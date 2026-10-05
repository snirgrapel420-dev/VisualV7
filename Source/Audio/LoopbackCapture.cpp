#if defined(_WIN32)
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>          // full header: COM (objbase) and mmsystem are needed
 #include <objbase.h>
 #include <mmsystem.h>
 #include <mmreg.h>
 #include <mmdeviceapi.h>
 #include <audioclient.h>
#endif

#include "LoopbackCapture.h"
#include <cstring>
#include <vector>

namespace dali
{
LoopbackCapture::LoopbackCapture(AudioAnalyzer& a) : juce::Thread("DaliVisual Loopback"), analyzer(a)
{
    setStatus(isSupported() ? "System audio: stopped"
                            : "System audio capture is Windows-only. On macOS route the output through "
                              "BlackHole (free) and choose it as the audio input.");
}

LoopbackCapture::~LoopbackCapture() { stop(); }

void LoopbackCapture::start()
{
    if (!isSupported() || isThreadRunning()) return;
    startThread(juce::Thread::Priority::high);
}

void LoopbackCapture::stop()
{
    if (isThreadRunning()) stopThread(3000);
    if (isSupported()) setStatus("System audio: stopped");
}

#if defined(_WIN32)
// -----------------------------------------------------------------------------
namespace
{
template <typename T> struct ComPtr
{
    T* p = nullptr;
    ~ComPtr() { reset(); }
    void reset() { if (p != nullptr) { p->Release(); p = nullptr; } }
    T** operator&() { reset(); return &p; }
    T* operator->() const { return p; }
    explicit operator bool() const { return p != nullptr; }
};

juce::String defaultRenderId(IMMDeviceEnumerator* en)
{
    ComPtr<IMMDevice> dev;
    if (FAILED(en->GetDefaultAudioEndpoint(eRender, eConsole, &dev)) || !dev) return {};
    LPWSTR id = nullptr;
    juce::String s;
    if (SUCCEEDED(dev->GetId(&id)) && id != nullptr) { s = juce::String(id); CoTaskMemFree(id); }
    return s;
}
}

bool LoopbackCapture::isSupported() noexcept { return true; }

void LoopbackCapture::run()
{
    const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    while (!threadShouldExit())
        if (!captureSession()) wait(1000);             // device missing / lost: retry
    if (SUCCEEDED(init)) CoUninitialize();
}

bool LoopbackCapture::captureSession()
{
    ComPtr<IMMDeviceEnumerator> en;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&en))))
    { setStatus("System audio: device enumerator unavailable"); return false; }

    ComPtr<IMMDevice> dev;
    if (FAILED(en->GetDefaultAudioEndpoint(eRender, eConsole, &dev)) || !dev)
    { setStatus("System audio: no output device"); return false; }
    const juce::String deviceId = defaultRenderId(en.p);

    ComPtr<IAudioClient> client;
    if (FAILED(dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&client))))
    { setStatus("System audio: cannot open the output device"); return false; }

    WAVEFORMATEX* fmt = nullptr;
    if (FAILED(client->GetMixFormat(&fmt)) || fmt == nullptr)
    { setStatus("System audio: unknown device format"); return false; }

    WORD tag = fmt->wFormatTag;
    if (tag == WAVE_FORMAT_EXTENSIBLE && fmt->cbSize >= 22)
        tag = WORD(reinterpret_cast<WAVEFORMATEXTENSIBLE*>(fmt)->SubFormat.Data1);   // {0000000X-...}: X = format tag
    const bool isFloat = tag == WAVE_FORMAT_IEEE_FLOAT && fmt->wBitsPerSample == 32;
    const bool isPcm16 = tag == WAVE_FORMAT_PCM && fmt->wBitsPerSample == 16;
    const bool isPcm24 = tag == WAVE_FORMAT_PCM && fmt->wBitsPerSample == 24;
    const bool isPcm32 = tag == WAVE_FORMAT_PCM && fmt->wBitsPerSample == 32;
    const int  channels = juce::jmax(1, int(fmt->nChannels));
    const int  rate = int(fmt->nSamplesPerSec);
    const int  frameBytes = int(fmt->nBlockAlign);

    if (!(isFloat || isPcm16 || isPcm24 || isPcm32))
    { CoTaskMemFree(fmt); setStatus("System audio: unsupported sample format"); return false; }

    const REFERENCE_TIME bufferDuration = 200000;       // 20 ms (100 ns units)
    const HRESULT hrInit = client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
                                              bufferDuration, 0, fmt, nullptr);
    CoTaskMemFree(fmt);
    if (FAILED(hrInit)) { setStatus("System audio: loopback not permitted by the device"); return false; }

    ComPtr<IAudioCaptureClient> capture;
    if (FAILED(client->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&capture))))
    { setStatus("System audio: capture service unavailable"); return false; }

    analyzer.setExternalSampleRate(double(rate));
    if (FAILED(client->Start())) { setStatus("System audio: cannot start capture"); return false; }
    setStatus("System audio: default output device, " + juce::String(rate) + " Hz, "
              + juce::String(channels) + " ch");

    std::vector<float> L, R;
    auto sampleAt = [&](const BYTE* frame, int ch) -> float
    {
        const int bytes = frameBytes / channels;
        const BYTE* p = frame + ch * bytes;
        if (isFloat) { float v; std::memcpy(&v, p, 4); return v; }
        if (isPcm16) { std::int16_t v; std::memcpy(&v, p, 2); return float(v) / 32768.0f; }
        if (isPcm24) { const std::int32_t v = std::int32_t(std::uint32_t(p[2]) << 24 | std::uint32_t(p[1]) << 16 | std::uint32_t(p[0]) << 8) >> 8;
                       return float(v) / 8388608.0f; }
        std::int32_t v; std::memcpy(&v, p, 4); return float(double(v) / 2147483648.0);
    };

    bool ok = true;
    double lastDeviceCheck = juce::Time::getMillisecondCounterHiRes();
    while (!threadShouldExit())
    {
        wait(5);
        UINT32 packet = 0;
        if (FAILED(capture->GetNextPacketSize(&packet))) { ok = false; break; }     // device invalidated
        while (packet > 0)
        {
            BYTE* data = nullptr; UINT32 frames = 0; DWORD flags = 0;
            if (FAILED(capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr))) { ok = false; break; }
            L.resize(frames); R.resize(frames);
            if ((flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0 || data == nullptr)
            {
                std::fill(L.begin(), L.end(), 0.0f); std::fill(R.begin(), R.end(), 0.0f);
            }
            else
            {
                for (UINT32 i = 0; i < frames; ++i)
                {
                    const BYTE* frame = data + size_t(i) * size_t(frameBytes);
                    L[i] = sampleAt(frame, 0);
                    R[i] = channels > 1 ? sampleAt(frame, 1) : L[i];
                }
            }
            if (frames > 0) analyzer.pushExternal(L.data(), R.data(), int(frames));
            capture->ReleaseBuffer(frames);
            if (FAILED(capture->GetNextPacketSize(&packet))) { ok = false; break; }
        }
        if (!ok) break;

        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        if (nowMs - lastDeviceCheck > 1000.0)                 // follow default-device changes
        {
            lastDeviceCheck = nowMs;
            if (defaultRenderId(en.p) != deviceId) break;
        }
    }
    client->Stop();
    if (!ok) setStatus("System audio: device lost, reconnecting ...");
    return ok;
}

#else
// -----------------------------------------------------------------------------
bool LoopbackCapture::isSupported() noexcept { return false; }
void LoopbackCapture::run() {}
bool LoopbackCapture::captureSession() { return false; }
#endif
} // namespace dali
