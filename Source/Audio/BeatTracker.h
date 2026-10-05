#pragma once
// ============================================================================
//  BeatTracker — tempo (autocorrelation of an onset-detection function) and
//  beat phase (comb-filter alignment + soft phase-locked loop).
//  Fed once per analysis hop from the analysis thread. Framework independent.
// ============================================================================
#include <cstdint>
#include <vector>

namespace dali
{
class BeatTracker
{
public:
    void prepare(double hopRateHz);
    void reset();

    /** odf: onset strength for this hop (>= 0). Returns true when a beat occurs in this hop. */
    bool process(float odf, bool signalPresent);

    float bpm() const noexcept          { return float(currentBpm); }
    float confidence() const noexcept   { return float(conf); }
    float phase() const noexcept        { return float(phaseValue); }
    std::uint32_t beats() const noexcept { return beatCounter; }

    static constexpr double minBpm = 85.0, maxBpm = 175.0;

private:
    void estimateTempo();
    void alignPhase();

    double hopRate = 93.75;
    std::vector<float> odfHistory;     // ring buffer
    size_t writePos = 0, filled = 0;
    int hopsSinceEstimate = 0;

    std::vector<double> bpmHistory;
    double currentBpm = 0.0, conf = 0.0;
    double phaseValue = 0.0;           // 0..1
    std::uint32_t beatCounter = 0;
};
} // namespace dali
