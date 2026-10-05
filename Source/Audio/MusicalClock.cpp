#include "MusicalClock.h"
#include <algorithm>
#include <cmath>

namespace dali
{
void MusicalClock::update(double now, double dt, SyncSource mode, const HostTiming& host,
                          const DetectedTiming& det, double internalBpm, int syncDiv)
{
    dt = std::clamp(dt, 0.0, 0.25);
    const bool hostUsable = host.valid && host.bpm > 1.0 && (host.playing || mode == SyncSource::Host);
    const bool detUsable  = det.bpm > 1.0 && det.confidence > 0.08;

    ActiveSource want = ActiveSource::Internal;
    switch (mode)
    {
        case SyncSource::Auto:     want = hostUsable ? ActiveSource::Host : (detUsable ? ActiveSource::Detect : ActiveSource::Internal); break;
        case SyncSource::Host:     want = hostUsable ? ActiveSource::Host : ActiveSource::Internal; break;
        case SyncSource::Detect:   want = det.bpm > 1.0 ? ActiveSource::Detect : ActiveSource::Internal; break;
        case SyncSource::Internal: want = ActiveSource::Internal; break;
    }
    active = want;

    double barBeats = 4.0;
    if (active == ActiveSource::Host)
    {
        currentBpm = host.bpm;
        const double target = host.ppq + (host.playing ? std::max(0.0, now - host.stamp) * host.bpm / 60.0 : 0.0);
        // The host position is extrapolated from its time stamp, so it already
        // contains the elapsed time: follow it directly (loops/locates included).
        beats = target;
        barBeats = std::max(1.0, host.numerator * 4.0 / std::max(1, host.denominator));
        const double inBar = (beats - host.barStartPpq) / barBeats;
        barPh = float(inBar - std::floor(inBar));
    }
    else
    {
        currentBpm = (active == ActiveSource::Detect) ? det.bpm : std::clamp(internalBpm, 20.0, 400.0);
        beats += dt * currentBpm / 60.0;
        if (active == ActiveSource::Detect)
        {
            double target = det.beatPhase + std::max(0.0, now - det.stamp) * det.bpm / 60.0;
            double err = (target - std::floor(target)) - (beats - std::floor(beats));
            err -= std::round(err);
            beats += err * std::min(1.0, dt * 3.0);
        }
        const double inBar = beats / barBeats;
        barPh = float(inBar - std::floor(inBar));
    }

    const double div = syncDivisionBeats(syncDiv);
    const double s = beats / div;
    syncPh = float(s - std::floor(s));

    const std::uint64_t idx = std::uint64_t(std::max(0.0, std::floor(beats)));
    beatEvent = idx != lastBeatIndex;
    lastBeatIndex = idx;
    pulse = beatEvent ? 1.0f : pulse * float(std::exp(-dt * 7.0));
}
} // namespace dali
