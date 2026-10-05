#pragma once
// ============================================================================
//  ModulationCore — sources, slot definition and the per-frame modulation
//  engine. Framework independent (unit tested). The JUCE-side
//  ModulationMatrix owns the editable configuration and serialisation.
//
//  Per slot:  source → sensitivity → invert → attack/release → smoothing →
//             curve → [min..max] window → polarity → × amount → target offset
//  Offsets are added to the target parameter's normalised (0..1) value.
// ============================================================================
#include <array>
#include <cstddef>
#include <cstdint>

namespace dali
{
enum class ModSource : int
{
    None = 0, Bass, Mid, High, Energy, Kick, Transient, Onset, Beat, BeatPhase, BarPhase,
    SyncLFO, SyncSaw, SyncSquare, Centroid, Flux, StereoWidth, StereoEnergy, StereoPan,
    RMS, Peak, MidiTrigger, RandomStep, Snare, HiHat, Build, Drop,
    Sub, LowMid, HighMid, BassSlow, EnergySlow, KickDensity, OnsetDensity, DynamicRange,
    StateCalm, StateBuild, StatePeak, StateChaos, StateRelease, count
};

inline const char* modSourceName(ModSource s)
{
    static const char* names[] = { "-", "Bass", "Mid", "High", "Energy", "Kick", "Transient", "Onset", "Beat",
        "Beat Phase", "Bar Phase", "Sync LFO", "Sync Saw", "Sync Square", "Centroid", "Flux", "Stereo Width",
        "Stereo Energy", "Stereo Pan", "RMS", "Peak", "MIDI Trigger", "Random Step", "Snare", "Hi-Hat", "Build", "Drop",
        "Sub", "Low Mid", "High Mid", "Bass (slow)", "Energy (slow)", "Kick Density", "Onset Density", "Dynamic Range",
        "State: Calm", "State: Build", "State: Peak", "State: Chaos", "State: Release" };
    const int i = int(s);
    return (i >= 0 && i < int(ModSource::count)) ? names[i] : "?";
}

struct ModSourceValues
{
    std::array<float, size_t(ModSource::count)> v {};
    float& operator[](ModSource s) noexcept { return v[size_t(s)]; }
    float  operator[](ModSource s) const noexcept { return v[size_t(s)]; }
};

struct ModSlot
{
    bool  enabled     = true;
    int   source      = 0;        // ModSource
    int   target      = -1;       // index into the modulation target list
    float amount      = 0.5f;     // -1..1
    float min         = 0.0f;     // output window (0..1)
    float max         = 1.0f;
    float smoothingMs = 30.0f;
    float attackMs    = 5.0f;
    float releaseMs   = 150.0f;
    float curve       = 0.0f;     // -1 (exponential) .. +1 (logarithmic)
    bool  bipolar     = false;    // polarity: false = 0..1, true = -1..1
    bool  invert      = false;
    float sensitivity = 1.0f;     // 0..4

    bool isActive() const noexcept { return enabled && source > 0 && target >= 0 && amount != 0.0f; }
};

static constexpr int kMaxModSlots = 16;
using ModSlotArray = std::array<ModSlot, kMaxModSlots>;

class ModulationEngine
{
public:
    /** Resets envelope state (e.g. after a preset load). */
    void reset() noexcept { env.fill(0.0f); smooth.fill(0.0f); }

    /** Evaluates all slots; writes per-target offsets (zeroed first). */
    void process(const ModSlotArray& slots, const ModSourceValues& src, float dtSeconds,
                 float* offsets, int numTargets) noexcept;

    /** Current post-processing value (0..1 or -1..1) of a slot — for UI metering. */
    float slotValue(int i) const noexcept { return (i >= 0 && i < kMaxModSlots) ? lastOut[size_t(i)] : 0.0f; }

    static float shapeCurve(float x, float curve) noexcept;

private:
    std::array<float, kMaxModSlots> env {}, smooth {}, lastOut {};
};
} // namespace dali
