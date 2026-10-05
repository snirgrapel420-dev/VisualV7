#pragma once
// ============================================================================
//  MusicalClock — resolves the musical timeline used by the visuals.
//  Priority (in Auto): Host transport (when playing) → detected tempo →
//  internal clock. Runs per rendered frame; framework independent.
// ============================================================================
#include <cstddef>
#include <cstdint>

namespace dali
{
enum class SyncSource { Auto = 0, Host, Detect, Internal };

/** Musical divisions, in beats (quarter notes). Index = 'syncDiv' parameter. */
inline double syncDivisionBeats(int index)
{
    static const double table[] = { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 4.0 / 3.0, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };
    const int n = int(sizeof(table) / sizeof(table[0]));
    return table[index < 0 ? 0 : (index >= n ? n - 1 : index)];
}
inline const char* const* syncDivisionNames()
{
    static const char* names[] = { "4 Bars", "2 Bars", "1 Bar", "1/2", "1/4", "1/8", "1/16", "1/2 T", "1/4 T", "1/8 T", "1/16 T" };
    return names;
}

struct HostTiming
{
    bool   valid = false, playing = false;
    double bpm = 0.0, ppq = 0.0, barStartPpq = 0.0;
    int    numerator = 4, denominator = 4;
    double stamp = 0.0;               // seconds (same clock as 'now' passed to update)
};

struct DetectedTiming
{
    double bpm = 0.0, confidence = 0.0, beatPhase = 0.0;
    double stamp = 0.0;               // seconds when beatPhase was valid
};

class MusicalClock
{
public:
    enum class ActiveSource { Host, Detect, Internal };

    void update(double now, double dt, SyncSource mode, const HostTiming& host,
                const DetectedTiming& det, double internalBpm, int syncDiv);

    double bpm() const noexcept          { return currentBpm; }
    double beatClock() const noexcept    { return beats; }
    float  beatPhase() const noexcept    { return phaseOf(beats); }
    float  barPhase() const noexcept     { return barPh; }
    float  syncPhase() const noexcept    { return syncPh; }
    float  beatPulse() const noexcept    { return pulse; }
    bool   beatHappened() const noexcept { return beatEvent; }
    ActiveSource source() const noexcept { return active; }
    std::uint64_t beatIndex() const noexcept { return lastBeatIndex; }

private:
    static float phaseOf(double b) { return float(b - double(std::int64_t(b >= 0 ? b : b - 1.0))); }

    double beats = 0.0, currentBpm = 120.0;
    float barPh = 0, syncPh = 0, pulse = 0;
    bool beatEvent = false;
    std::uint64_t lastBeatIndex = 0;
    ActiveSource active = ActiveSource::Internal;
};
} // namespace dali
