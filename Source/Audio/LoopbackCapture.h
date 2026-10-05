#pragma once
// ============================================================================
//  LoopbackCapture — "what you hear": captures the audio that the computer is
//  playing on its default output device and feeds it to the analyzer.
//
//  Windows: WASAPI shared-mode loopback on the default render endpoint. Follows
//           default-device changes and recovers from device loss automatically.
//           When nothing is playing WASAPI delivers no packets, so the visuals
//           come to rest — they only move with what actually comes out.
//  macOS:   not available (Apple provides no loopback API for this use); route
//           the output through a loopback driver such as BlackHole and choose it
//           as the audio input instead.
// ============================================================================
#include <juce_core/juce_core.h>
#include "AudioAnalyzer.h"

namespace dali
{
class LoopbackCapture : private juce::Thread
{
public:
    explicit LoopbackCapture(AudioAnalyzer& analyzer);
    ~LoopbackCapture() override;

    static bool isSupported() noexcept;

    void start();
    void stop();
    bool isActive() const noexcept { return isThreadRunning(); }
    juce::String getStatus() const { const juce::SpinLock::ScopedLockType sl(lock); return status; }

private:
    void run() override;
    bool captureSession();                     // one device session; false = error, retry later
    void setStatus(const juce::String& s) { const juce::SpinLock::ScopedLockType sl(lock); status = s; }

    AudioAnalyzer& analyzer;
    mutable juce::SpinLock lock;
    juce::String status;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoopbackCapture)
};
} // namespace dali
