#include "VideoRecorder.h"
#include "VideoEncoder.h"
#include "../Core/AppPrefs.h"
#include <cmath>
#include <cstring>

namespace dali
{
namespace
{
int evenUp(double v) { const int i = int(std::lround(v)); return i + (i & 1); }

/** Frame size for a format / quality. srcAspect is only used by 'Screen'. */
void frameDims(int format, int quality, double srcAspect, int& w, int& h)
{
    int shortSide = quality == VideoRecorder::Quality720 ? 720 : (quality == VideoRecorder::Quality4K ? 2160 : 1080);
    double a = srcAspect;
    if (format == VideoRecorder::FormatLandscape) a = 16.0 / 9.0;
    if (format == VideoRecorder::FormatPortrait)  a = 9.0 / 16.0;
    if (format == VideoRecorder::FormatSquare)    a = 1.0;
    a = juce::jlimit(0.25, 4.0, std::isfinite(a) && a > 0.0 ? a : 16.0 / 9.0);
    if (a < 1.0 && shortSide > 1440) shortSide = 1440;              // tall 4K (2160x3840) is beyond common H.264 encoders
    if (a >= 1.0) { h = shortSide; w = evenUp(shortSide * a); }
    else          { w = shortSide; h = evenUp(shortSide / a); }
    const int longest = juce::jmax(w, h);
    if (longest > 3840)                                                 // ultra-wide screens: keep within 4K
    {
        const double k = 3840.0 / longest;
        w = evenUp(w * k); h = evenUp(h * k);
    }
}

int bitrateFor(int w, int h, int fps)
{
    // generous: the visuals are full of fine, fast-moving detail
    const double bps = double(w) * double(h) * double(fps) * 0.16;
    return int(juce::jlimit(8.0e6, 90.0e6, bps));
}

// ---------------------------------------------------------------------------------------------
// Streams any input rate to 44.1 / 48 kHz (the AAC rates): integer multiples are decimated with a
// windowed-sinc low-pass (no aliasing), anything else is interpolated linearly.
class StreamResampler
{
public:
    void prepare(double inRate, int& outRate)
    {
        const int in = int(std::lround(inRate));
        factor = 1; linear = false; ratio = 1.0; pos = 0.0;
        if (in == 44100 || in == 48000)       { outRate = in; }
        else if (in > 0 && in % 48000 == 0)   { outRate = 48000; factor = in / 48000; }
        else if (in > 0 && in % 44100 == 0)   { outRate = 44100; factor = in / 44100; }
        else                                  { outRate = 48000; linear = in > 0; ratio = in > 0 ? double(in) / 48000.0 : 1.0; }

        taps.clear();
        if (factor > 1)
        {
            const int n = 24 * factor + 1;
            const double fc = 0.45 / factor;                                  // normalised cut-off (of the input rate)
            double sum = 0.0;
            for (int i = 0; i < n; ++i)
            {
                const double x = i - (n - 1) * 0.5;
                const double sinc = x == 0.0 ? 2.0 * fc : std::sin(2.0 * juce::MathConstants<double>::pi * fc * x) / (juce::MathConstants<double>::pi * x);
                const double win = 0.42 - 0.5 * std::cos(2.0 * juce::MathConstants<double>::pi * i / (n - 1))
                                        + 0.08 * std::cos(4.0 * juce::MathConstants<double>::pi * i / (n - 1));
                taps.push_back(float(sinc * win));
                sum += sinc * win;
            }
            for (auto& t : taps) t = float(t / sum);
        }
        history.assign(taps.empty() ? 2 : taps.size(), { 0.0f, 0.0f });
        phase = 0;
        prev = { 0.0f, 0.0f };
    }

    /** in: interleaved stereo. Appends interleaved output to 'out'. */
    void process(const float* in, int frames, std::vector<float>& out)
    {
        if (factor == 1 && !linear)
        {
            out.insert(out.end(), in, in + size_t(frames) * 2);
            return;
        }
        for (int i = 0; i < frames; ++i)
        {
            const std::pair<float, float> s { in[2 * i], in[2 * i + 1] };
            if (linear)
            {
                // output samples that fall between prev (t=-1) and s (t=0)
                while (pos <= 1.0)
                {
                    const float t = float(pos);
                    out.push_back(prev.first + (s.first - prev.first) * t);
                    out.push_back(prev.second + (s.second - prev.second) * t);
                    pos += ratio;
                }
                pos -= 1.0;
                prev = s;
            }
            else
            {
                std::memmove(history.data(), history.data() + 1, (history.size() - 1) * sizeof(history[0]));
                history.back() = s;
                if (++phase >= factor)
                {
                    phase = 0;
                    float l = 0.0f, r = 0.0f;
                    for (size_t k = 0; k < taps.size(); ++k) { l += taps[k] * history[k].first; r += taps[k] * history[k].second; }
                    out.push_back(l); out.push_back(r);
                }
            }
        }
    }

private:
    int factor = 1, phase = 0;
    bool linear = false;
    double ratio = 1.0, pos = 0.0;
    std::vector<float> taps;
    std::vector<std::pair<float, float>> history;
    std::pair<float, float> prev { 0.0f, 0.0f };
};
} // namespace

// =============================================================================
//  settings
// =============================================================================
VideoRecorder::Settings VideoRecorder::loadSettings()
{
    juce::PropertiesFile f(AppPrefs::options());
    Settings s;
    s.format  = juce::jlimit(0, 3, f.getIntValue("recFormat", FormatScreen));
    s.quality = juce::jlimit(0, 2, f.getIntValue("recQuality", Quality1080));
    s.fps     = f.getIntValue("recFps", 60) == 30 ? 30 : 60;
    const auto folder = f.getValue("recFolder");
    if (folder.isNotEmpty() && juce::File::isAbsolutePath(folder)) s.folder = juce::File(folder);
    return s;
}

void VideoRecorder::saveSettings(const Settings& s)
{
    juce::PropertiesFile f(AppPrefs::options());
    f.setValue("recFormat", s.format);
    f.setValue("recQuality", s.quality);
    f.setValue("recFps", s.fps);
    f.setValue("recFolder", s.folder == juce::File() ? juce::String() : s.folder.getFullPathName());
    f.saveIfNeeded();
}

juce::File VideoRecorder::defaultFolder()
{
    auto base = juce::File::getSpecialLocation(juce::File::userMoviesDirectory);
    if (!base.isDirectory()) base = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    return base.getChildFile("Dali Visual");
}

juce::File VideoRecorder::folderFor(const Settings& s)
{
    return s.folder != juce::File() ? s.folder : defaultFolder();
}

bool VideoRecorder::isSupported()
{
   #if JUCE_WINDOWS || JUCE_MAC || DALI_RECORDER_TEST
    return true;
   #else
    return false;
   #endif
}

// =============================================================================
VideoRecorder::VideoRecorder() : juce::Thread("Dali Visual recorder") {}

VideoRecorder::~VideoRecorder()
{
    if (recording.load()) stop();
    waitForThreadToExit(60000);                   // let it finish the file (a valid .mp4 needs its index)
    cancelPendingUpdate();
}

bool VideoRecorder::start(const Settings& s, juce::String& error)
{
    if (recording.load()) return true;
    if (isThreadRunning()) { error = "The previous recording is still being saved - one moment."; return false; }
    if (!isSupported()) { error = "Recording is not available on this system."; return false; }

    const auto folder = folderFor(s);
    if (!folder.isDirectory() && !folder.createDirectory())
    {
        error = "Cannot create the folder " + folder.getFullPathName();
        return false;
    }
    file = folder.getNonexistentChildFile("Dali Visual " + juce::Time::getCurrentTime().formatted("%Y-%m-%d %H-%M-%S"), ".mp4", false);
    current = s;

    // the audio ring is drained here (the tap is idle while not recording; the writer is not running)
    { Frame tmp[1024]; while (audioRing.pop(tmp, 1024) > 0) {} }
    tapRate.store(0.0);

    sizeLocked.store(false);
    lockedW.store(0); lockedH.store(0);
    if (s.format != FormatScreen)
    {
        int w = 0, h = 0;
        frameDims(s.format, s.quality, 16.0 / 9.0, w, h);
        lockedW.store(w); lockedH.store(h);
        sizeLocked.store(true);
    }
    lastSlot.store(-1);
    stopTime.store(0.0);
    ++gen;
    {
        const std::lock_guard<std::mutex> lg(resultLock);
        lastResult = {};
    }
    startTime.store(nowSeconds());
    finishing.store(false);
    recording.store(true);
    startThread(juce::Thread::Priority::high);
    return true;
}

void VideoRecorder::stop()
{
    if (!recording.load()) return;
    stopTime.store(nowSeconds());
    finishing.store(true);
    recording.store(false);                         // the writer drains, pads and finishes the file
}

double VideoRecorder::elapsedSeconds() const
{
    if (!recording.load()) return 0.0;
    return nowSeconds() - startTime.load();
}

VideoRecorder::Result VideoRecorder::getLastResult() const
{
    const std::lock_guard<std::mutex> lg(resultLock);
    return lastResult;
}

// =============================================================================
//  audio threads
// =============================================================================
void VideoRecorder::tapAudio(const float* left, const float* right, int numSamples, double sampleRate) noexcept
{
    if (!recording.load(std::memory_order_relaxed) || numSamples <= 0) return;
    if (tapRate.load(std::memory_order_relaxed) <= 0.0) tapRate.store(sampleRate);
    Frame tmp[256];
    int pos = 0;
    while (pos < numSamples)
    {
        const int n = juce::jmin(256, numSamples - pos);
        for (int i = 0; i < n; ++i) tmp[i] = { left[pos + i], right != nullptr ? right[pos + i] : left[pos + i] };
        audioRing.push(tmp, size_t(n));
        pos += n;
    }
}

// =============================================================================
//  GL thread
// =============================================================================
bool VideoRecorder::frameSize(int srcW, int srcH, int& width, int& height) noexcept
{
    if (!recording.load()) return false;
    if (!sizeLocked.load())
    {
        int w = 0, h = 0;
        frameDims(current.format, current.quality, double(srcW) / double(juce::jmax(1, srcH)), w, h);
        lockedW.store(w); lockedH.store(h);
        sizeLocked.store(true);
    }
    width = lockedW.load(); height = lockedH.load();
    return width > 0 && height > 0;
}

bool VideoRecorder::frameDue(std::int64_t& slot) noexcept
{
    if (!recording.load()) return false;
    const double pos = (nowSeconds() - startTime.load()) * current.fps;
    const std::int64_t next = lastSlot.load() + 1;
    if (pos < double(next) - 0.5) return false;                 // too early for the next frame
    // stay on consecutive frames while the render clock is within a frame of the video clock
    slot = (pos - double(next) < 1.0) ? next : std::int64_t(std::floor(pos + 0.5));
    lastSlot.store(slot);
    return true;
}

bool VideoRecorder::deliverFrame(const std::uint8_t* src, int width, int height, std::int64_t slot) noexcept
{
    if (!recording.load()) return false;
    const std::lock_guard<std::mutex> lg(poolLock);
    if (frameBytes == 0 || size_t(width) * size_t(height) * 4 != frameBytes || freeFrames.empty()) return false;
    auto* dst = freeFrames.back();
    freeFrames.pop_back();
    std::memcpy(dst, src, frameBytes);
    readyFrames.emplace_back(dst, slot);
    return true;
}

// =============================================================================
//  writer thread
// =============================================================================
void VideoRecorder::publish(const Result& r)
{
    {
        const std::lock_guard<std::mutex> lg(resultLock);
        lastResult = r;
    }
    triggerAsyncUpdate();
}

void VideoRecorder::handleAsyncUpdate()
{
    finishing.store(false);
    finished.sendChangeMessage();
}

void VideoRecorder::run()
{
    Result result;
    result.file = file;
    const int fps = current.fps;

    // ---- 1. frame size (known once the publishing engine has rendered a frame) ----------------------
    while (!sizeLocked.load() && recording.load()) wait(5);
    if (!sizeLocked.load())
    {
        result.error = "No picture was rendered - keep the Dali Visual window or the live output open while recording.";
        publish(result);
        return;
    }
    const int W = lockedW.load(), H = lockedH.load();
    result.width = W; result.height = H;
    {
        const std::lock_guard<std::mutex> lg(poolLock);
        const size_t bytes = size_t(W) * size_t(H) * 4;
        const int count = W * H > 4000000 ? 4 : 6;
        storage.clear(); freeFrames.clear(); readyFrames.clear();
        for (int i = 0; i < count; ++i)
        {
            storage.emplace_back(new std::uint8_t[bytes]);
            freeFrames.push_back(storage.back().get());
        }
        frameBytes = bytes;
    }

    // ---- 2. audio rate (from the first audio that arrives; silence if none does) ---------------------
    const double rateWait = nowSeconds();
    while (tapRate.load() <= 0.0 && recording.load() && nowSeconds() - rateWait < 0.6) wait(5);
    StreamResampler resampler;
    int outRate = 48000;
    resampler.prepare(tapRate.load() > 0.0 ? tapRate.load() : 48000.0, outRate);

    // ---- 3. encoder --------------------------------------------------------------------------------
    auto encoder = VideoEncoder::create();
    std::string err;
    VideoEncoder::Settings es;
    es.width = W; es.height = H; es.fps = fps;
    es.videoBitrate = bitrateFor(W, H, fps);
    es.sampleRate = outRate;
    const bool opened = encoder != nullptr && encoder->open(file.getFullPathName().toRawUTF8(), es, err);

    std::int64_t nextSlot = 0, audioPos = 0;
    bool limitHit = false;
    std::uint8_t* last = nullptr;
    std::vector<std::pair<std::uint8_t*, std::int64_t>> batch;
    std::vector<float> in(2048 * 2), out;
    bool writeFailed = false;

    auto giveBack = [this](std::uint8_t* p)
    {
        const std::lock_guard<std::mutex> lg(poolLock);
        freeFrames.push_back(p);
    };

    if (!opened)
    {
        result.error = juce::String("Could not start the video encoder: ") + (err.empty() ? "not available" : err.c_str());
        recording.store(false);
    }

    while (opened)
    {
        if (current.maxSeconds > 0.0 && recording.load() && nowSeconds() - startTime.load() >= current.maxSeconds)
        {
            limitHit = true;
            stop();                                                  // the demo's time limit
        }
        const bool stopping = !recording.load();

        // ---- video: constant frame rate -------------------------------------------------------------
        batch.clear();
        {
            const std::lock_guard<std::mutex> lg(poolLock);
            batch.assign(readyFrames.begin(), readyFrames.end());
            readyFrames.clear();
        }
        for (auto& [buf, slot] : batch)
        {
            if (slot < nextSlot) { giveBack(buf); continue; }
            const auto* fill = last != nullptr ? last : buf;             // the very first frame also covers the start
            for (std::int64_t s = nextSlot; s < slot && s < nextSlot + 2 * fps; ++s)
                writeFailed |= !encoder->writeVideo(fill, s);
            writeFailed |= !encoder->writeVideo(buf, slot);
            nextSlot = slot + 1;
            if (last != nullptr) giveBack(last);
            last = buf;
        }

        // ---- audio --------------------------------------------------------------------------------
        for (;;)
        {
            Frame tmp[2048];
            const size_t n = audioRing.pop(tmp, 2048);
            if (n == 0) break;
            for (size_t i = 0; i < n; ++i) { in[2 * i] = tmp[i].l; in[2 * i + 1] = tmp[i].r; }
            out.clear();
            resampler.process(in.data(), int(n), out);
            const int frames = int(out.size() / 2);
            if (frames > 0) { writeFailed |= !encoder->writeAudio(out.data(), frames, audioPos); audioPos += frames; }
        }

        // keep the sound on the wall clock: write silence while nothing arrives (stopped transport,
        // a silent loopback device), so the picture never runs ahead of the audio
        const double elapsed = (stopping ? stopTime.load() : nowSeconds()) - startTime.load();
        const std::int64_t target = std::int64_t((elapsed - (stopping ? 0.0 : 0.04)) * outRate);
        if (target > audioPos && (stopping || target - audioPos > std::int64_t(0.25 * outRate)))
        {
            std::vector<float> zeros(2048 * 2, 0.0f);
            while (audioPos < target)
            {
                const int frames = int(juce::jmin<std::int64_t>(2048, target - audioPos));
                writeFailed |= !encoder->writeAudio(zeros.data(), frames, audioPos);
                audioPos += frames;
            }
        }

        if (stopping)
        {
            // the picture lasts as long as the sound
            const std::int64_t endSlot = std::int64_t(std::ceil(double(audioPos) / outRate * fps));
            if (last != nullptr)
                for (std::int64_t s = nextSlot; s < endSlot && s < nextSlot + 2 * fps; ++s)
                    writeFailed |= !encoder->writeVideo(last, s);
            break;
        }
        wait(4);
    }

    if (opened)
    {
        std::string finishError;
        const bool ok = encoder->finish(finishError);
        result.seconds = double(audioPos) / outRate;
        result.limitReached = limitHit;
        result.ok = ok && last != nullptr;
        if (!ok) result.error = finishError.empty() ? juce::String("The video file could not be finished.") : juce::String(finishError.c_str());
        else if (last == nullptr) result.error = "No picture was captured.";
        else if (writeFailed) result.error = "Some frames could not be encoded (the computer was too busy) - the file is usable.";
    }
    encoder.reset();
    if (!result.ok && last == nullptr && file.existsAsFile()) file.deleteFile();     // nothing usable was written

    {
        const std::lock_guard<std::mutex> lg(poolLock);
        frameBytes = 0;
        freeFrames.clear(); readyFrames.clear(); storage.clear();
    }
    publish(result);
}
} // namespace dali
