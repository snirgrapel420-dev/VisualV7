#pragma once
// ============================================================================
//  VideoRecorder — records the visual output together with the audio it reacts
//  to into an .mp4 (H.264 + AAC), in real time.
//
//  Threads
//    message   start() / stop() / settings / result
//    audio     tapAudio()  — the analyzer forwards exactly the audio it analyses
//              (track audio in the plug-in, input or system audio in the standalone);
//              wait-free, never allocates
//    GL        the engine that publishes the output (fullscreen when live, else the
//              preview) renders a dedicated full-resolution frame and reads it back
//              asynchronously (RecordCapture); buffers come from a fixed pool here
//    writer    this class's own thread: constant-frame-rate video (late frames are
//              repeated, early ones dropped), audio kept on the wall clock (silence
//              is written while no audio arrives, e.g. a stopped transport)
// ============================================================================
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "../Audio/SpscRing.h"
#include "../Audio/AudioAnalyzer.h"
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

namespace dali
{
class VideoRecorder : public AudioTap, private juce::Thread, private juce::AsyncUpdater
{
public:
    enum Format  { FormatScreen = 0, FormatLandscape, FormatPortrait, FormatSquare };
    enum Quality { Quality720 = 0, Quality1080, Quality4K };

    struct Settings
    {
        int format  = FormatScreen;     // Screen = the aspect of what is on screen
        int quality = Quality1080;      // the short side: 720 / 1080 / 2160
        int fps     = 60;               // 30 or 60
        juce::File folder;              // empty = default (Videos / Movies / "Dali Visual")
        double maxSeconds = 0.0;        // > 0: stops by itself after this long (demo); not stored
    };
    static Settings loadSettings();
    static void saveSettings(const Settings&);
    static juce::File defaultFolder();
    static juce::File folderFor(const Settings&);
    static bool isSupported();          // an encoder exists on this platform

    VideoRecorder();
    ~VideoRecorder() override;

    // ---- message thread ------------------------------------------------------------------
    bool start(const Settings& settings, juce::String& error);
    void stop();                                            // finishes the file in the background
    bool isRecording() const noexcept   { return recording.load(); }
    bool isFinishing() const noexcept   { return finishing.load(); }
    double elapsedSeconds() const;
    struct Result { bool ok = false; juce::File file; juce::String error; double seconds = 0.0; int width = 0, height = 0;
                    bool limitReached = false; };
    double getMaxSeconds() const noexcept { return current.maxSeconds; }
    Result getLastResult() const;
    /** Broadcast (message thread) when a recording has been finished or has failed. */
    juce::ChangeBroadcaster finished;

    // ---- audio threads ---------------------------------------------------------------------
    void tapAudio(const float* left, const float* right, int numSamples, double sampleRate) noexcept override;

    // ---- GL thread (the publishing engine) -----------------------------------------------------
    /** True while frames are wanted. Locks the frame size on the first call of a recording:
        'Screen' takes the aspect of srcW x srcH. Returns the frame size. */
    bool frameSize(int srcW, int srcH, int& width, int& height) noexcept;
    /** True when the next video frame is due now (constant frame rate); 'slot' = its frame index. */
    bool frameDue(std::int64_t& slot) noexcept;
    /** Copies a finished frame (BGRA, top-down, width*4 bytes per row) to the writer.
        False when it was not taken (writer behind or recording over): the writer repeats the last picture. */
    bool deliverFrame(const std::uint8_t* bgraTopDown, int width, int height, std::int64_t slot) noexcept;
    /** Changes whenever a new recording starts (lets the GL side drop stale readbacks). */
    int generation() const noexcept { return gen.load(); }

private:
    void run() override;
    void handleAsyncUpdate() override;
    void publish(const Result& r);
    static double nowSeconds() noexcept { return juce::Time::getMillisecondCounterHiRes() * 0.001; }

    struct Frame { float l, r; };
    SpscRing<Frame> audioRing { 1u << 18 };                   // ~5 s at 48 kHz
    std::atomic<double> tapRate { 0.0 };

    std::atomic<bool> recording { false }, finishing { false }, sizeLocked { false };
    std::atomic<int> gen { 0 };
    std::atomic<double> startTime { 0.0 }, stopTime { 0.0 };
    std::atomic<int> lockedW { 0 }, lockedH { 0 };
    std::atomic<std::int64_t> lastSlot { -1 };

    Settings current;
    juce::File file;

    // frame pool: the GL thread copies into a free buffer under the lock (the writer only pops/pushes
    // pointers under it); allocated by the writer once the size is known, freed when it finishes
    std::mutex poolLock;
    std::vector<std::unique_ptr<std::uint8_t[]>> storage;
    std::vector<std::uint8_t*> freeFrames;
    std::deque<std::pair<std::uint8_t*, std::int64_t>> readyFrames;
    size_t frameBytes = 0;

    mutable std::mutex resultLock;
    Result lastResult;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VideoRecorder)
};
} // namespace dali
