#pragma once
// ============================================================================
//  EngineState — everything the render engines, the UI and the processor
//  share. Owned by the processor; outlives every editor / output window.
//
//  Threading contract
//   • Audio thread writes: host timing (try-lock, never blocks), CPU load.
//   • Analysis thread writes: AudioAnalyzer snapshot.
//   • Message thread writes: parameters (APVTS), modulation matrix, effect
//     order, image template, output settings.
//   • GL threads read all of the above (short spin-locked copies) and write
//     display telemetry (fps, bpm, modulated values) through atomics.
// ============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "../Audio/AudioAnalyzer.h"
#include "../Audio/MusicalClock.h"
#include "../Modulation/ModulationMatrix.h"
#include "../Render/EffectChain.h"
#include "../Image/ImageProcessor.h"
#include <array>
#include <atomic>
#include <memory>

namespace dali
{
class FrameSink;

struct HostTimingShared
{
    void writeFromAudioThread(const HostTiming& t) noexcept
    {
        const juce::SpinLock::ScopedTryLockType tl(lock);
        if (tl.isLocked()) timing = t;          // skip this block rather than wait
    }
    HostTiming read() const
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        return timing;
    }
private:
    mutable juce::SpinLock lock;
    HostTiming timing;
};

// One timeline for every render engine (preview + fullscreen output): the leading engine (the
// output while it is live, otherwise the preview) integrates the music-driven clocks, the other
// follows — so both windows show the same picture.
struct SharedTimeline
{
    std::atomic<double> sceneTime { 0 }, templateMotion { 0 };
    std::atomic<double> bassTime { 0 }, midTime { 0 }, highTime { 0 }, levelTime { 0 };
    std::atomic<float>  colorDrift { 0 }, randomStep { 0 };
};

struct OutputSettings
{
    // 0 = ADAPTIVE (default): resolution and raymarch detail follow the frame rate automatically
    // 1 = 50 %, 2 = 75 %, 3 = 100 %, 4 = 150 % (supersampled; very heavy with raymarched scenes)
    std::atomic<int>   renderScaleIndex { 0 };
    std::atomic<bool>  vsync { true };
    std::atomic<bool>  previewWhileOutput { true };
    std::atomic<int>   displayIndex { -1 };         // -1 = last (usually external) display
    static constexpr int kAdaptive = 0;
    static float scaleFor(int idx) noexcept
    {
        static const float s[] = { 1.0f, 0.5f, 0.75f, 1.0f, 1.5f };
        return s[idx < 0 ? 0 : (idx > 4 ? 4 : idx)];
    }
};

struct Telemetry
{
    std::atomic<float> previewFps { 0 }, outputFps { 0 }, frameMs { 0 };
    std::atomic<float> quality { 1.0f };            // adaptive quality of the main picture (0..1)
    std::atomic<int> outWindowW { 0 }, outWindowH { 0 }, outSurfaceW { 0 }, outSurfaceH { 0 }, outRenderW { 0 }, outRenderH { 0 };
    std::atomic<float> bpm { 0 };
    std::atomic<int>   clockSource { 2 };            // MusicalClock::ActiveSource
    std::atomic<float> beatPulse { 0 };
    std::atomic<float> cpuLoad { 0 };                // audio callback load 0..1
    std::atomic<bool>  outputActive { false };
    std::atomic<float> activity { 0 };               // 0 = silence / no signal, 1 = music playing
    std::atomic<std::int64_t> barCount { 0 };        // musical bars elapsed (drives Auto Pilot)
    std::atomic<std::int64_t> variationKey { 0 };    // Auto Pilot phrase counter shared by every engine
    juce::String       rendererInfo;                 // written once by GL thread under lock
    juce::SpinLock     infoLock;
};

struct EngineState
{
    EngineState(juce::AudioProcessorValueTreeState& s, AudioAnalyzer& a, ModulationMatrix& m,
                 EffectChain& fx, ImageProcessor& img)
        : apvts(s), analyzer(a), matrix(m), effects(fx), image(img)
    {
        for (auto& d : params::all()) paramPtrs.push_back(apvts.getParameter(d.id));
        for (auto& v : modulated) v.store(-1.0f);
    }

    juce::RangedAudioParameter* param(int index) const noexcept
    {
        return (index >= 0 && index < int(paramPtrs.size())) ? paramPtrs[size_t(index)] : nullptr;
    }

    juce::AudioProcessorValueTreeState& apvts;
    AudioAnalyzer&    analyzer;
    ModulationMatrix& matrix;
    EffectChain&      effects;
    ImageProcessor&   image;

    std::vector<juce::RangedAudioParameter*> paramPtrs;   // index = params::all() order
    HostTimingShared host;
    OutputSettings   output;
    SharedTimeline   timeline;
    Telemetry        telemetry;

    // MIDI trigger source (written by MidiMapper on the message thread)
    std::atomic<std::uint32_t> midiTriggerCount { 0 };
    std::atomic<float>         midiTriggerVelocity { 0 };

    // Modulated (effective) normalised value per parameter index, for UI rings. -1 = not modulated.
    static constexpr int kMaxParams = 256;
    std::array<std::atomic<float>, kMaxParams> modulated;

    // Frame sinks (Spout / Syphon / NDI adapters) — registered on the message thread.
    juce::SpinLock sinkLock;
    juce::Array<FrameSink*> sinks;
};
} // namespace dali
