#pragma once
// ============================================================================
//  AudioAnalyzer — bridges audio producers and the analysis DSP.
//
//  Two independent wait-free SPSC rings, one per producer:
//    Host      audio thread (plug-in input / standalone input device)
//    External  system-audio loopback thread (standalone, Windows WASAPI)
//  The analysis thread consumes whichever source is selected and discards the
//  other, so the two producers never share a queue.
//
//  Any thread: snapshot() → latest AudioFeatures + publication time stamp
//  (a stale stamp means no audio is arriving at all).
// ============================================================================
#include <juce_core/juce_core.h>
#include "FeatureExtractor.h"
#include "SpscRing.h"
#include <atomic>

namespace dali
{
class AudioAnalyzer : private juce::Thread
{
public:
    enum class Source { Host = 0, External = 1 };

    struct Snapshot
    {
        AudioFeatures features;
        double stamp = 0.0;          // seconds (now()) when the features were published
    };

    AudioAnalyzer();
    ~AudioAnalyzer() override;

    /** Setup threads. Safe to call while running. */
    void prepare(double hostSampleRate);
    void setExternalSampleRate(double sr);
    void setSource(Source s) noexcept { source.store(int(s)); }
    Source getSource() const noexcept { return Source(source.load()); }
    void setSensitivity(float s) noexcept { sensitivity.store(s); }

    /** Host audio thread. Wait-free. 'right' may be nullptr (mono). */
    void push(const float* left, const float* right, int numSamples) noexcept;
    /** Loopback thread. Wait-free. */
    void pushExternal(const float* left, const float* right, int numSamples) noexcept;

    /** Any non-audio thread. */
    Snapshot snapshot() const;

    static double now() noexcept { return juce::Time::getMillisecondCounterHiRes() * 0.001; }

private:
    struct Frame { float l, r; };
    static void pushInto(SpscRing<Frame>& ring, const float* left, const float* right, int n) noexcept;
    void run() override;

    SpscRing<Frame> hostRing { 1u << 16 }, extRing { 1u << 16 };
    FeatureExtractor extractor;
    std::atomic<double> hostRate { 48000.0 }, extRate { 48000.0 };
    std::atomic<int> rateVersion { 0 };
    std::atomic<int> source { int(Source::Host) };
    std::atomic<float> sensitivity { 1.0f };

    mutable juce::SpinLock lock;
    Snapshot latest;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioAnalyzer)
};
} // namespace dali
