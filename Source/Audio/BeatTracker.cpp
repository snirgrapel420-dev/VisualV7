#include "BeatTracker.h"
#include <algorithm>
#include <cmath>

namespace dali
{
void BeatTracker::prepare(double hopRateHz)
{
    hopRate = hopRateHz > 1.0 ? hopRateHz : 93.75;
    odfHistory.assign(size_t(std::ceil(hopRate * 8.0)), 0.0f);  // 8 s window
    reset();
}

void BeatTracker::reset()
{
    std::fill(odfHistory.begin(), odfHistory.end(), 0.0f);
    writePos = 0; filled = 0; hopsSinceEstimate = 0;
    bpmHistory.clear();
    currentBpm = 0.0; conf = 0.0; phaseValue = 0.0;
}

bool BeatTracker::process(float odf, bool signalPresent)
{
    if (odfHistory.empty()) prepare(hopRate);
    odfHistory[writePos] = signalPresent ? std::max(0.0f, odf) : 0.0f;
    writePos = (writePos + 1) % odfHistory.size();
    filled = std::min(filled + 1, odfHistory.size());

    if (++hopsSinceEstimate >= int(hopRate * 0.5))      // re-estimate twice per second
    {
        hopsSinceEstimate = 0;
        if (filled > size_t(hopRate * 3.0)) { estimateTempo(); alignPhase(); }
        if (!signalPresent) conf *= 0.8;
    }

    if (currentBpm <= 0.0) return false;
    phaseValue += currentBpm / 60.0 / hopRate;
    if (phaseValue >= 1.0)
    {
        phaseValue -= std::floor(phaseValue);
        ++beatCounter;
        return true;
    }
    return false;
}

void BeatTracker::estimateTempo()
{
    const size_t N = filled;
    std::vector<float> x(N);
    // chronological copy, mean removed
    double mean = 0;
    for (size_t i = 0; i < N; ++i)
    {
        const size_t idx = (writePos + odfHistory.size() - N + i) % odfHistory.size();
        x[i] = odfHistory[idx]; mean += x[i];
    }
    mean /= double(N);
    double energy = 0;
    for (auto& v : x) { v = float(v - mean); energy += double(v) * v; }
    if (energy < 1e-9) { conf *= 0.9; return; }

    auto acf = [&](double lag) -> double
    {
        const int l0 = int(std::floor(lag)); const double fr = lag - l0;
        double s0 = 0, s1 = 0;
        for (size_t i = size_t(l0) + 1; i < N; ++i)
        {
            s0 += double(x[i]) * x[i - size_t(l0)];
            s1 += double(x[i]) * x[i - size_t(l0) - 1];
        }
        return ((1.0 - fr) * s0 + fr * s1) / energy;
    };

    const double lagMin = 60.0 * hopRate / maxBpm, lagMax = 60.0 * hopRate / minBpm;
    double bestScore = -1e9, bestLag = 0;
    for (double lag = lagMin; lag <= lagMax; lag += 0.25)
    {
        const double b = 60.0 * hopRate / lag;
        const double prior = std::exp(-0.5 * std::pow(std::log2(b / 135.0) / 0.6, 2.0));
        const double score = (acf(lag) + 0.5 * acf(lag * 2.0) + 0.25 * acf(lag * 4.0)) * (0.6 + 0.4 * prior);
        if (score > bestScore) { bestScore = score; bestLag = lag; }
    }
    if (bestLag <= 0) return;

    double estimate = 60.0 * hopRate / bestLag;
    const double c = std::clamp(bestScore / 1.75, 0.0, 1.0);

    bpmHistory.push_back(estimate);
    if (bpmHistory.size() > 9) bpmHistory.erase(bpmHistory.begin());
    std::vector<double> sorted = bpmHistory;
    std::sort(sorted.begin(), sorted.end());
    const double median = sorted[sorted.size() / 2];

    if (currentBpm <= 0.0) currentBpm = median;
    else if (std::abs(median - currentBpm) > 3.0) currentBpm = median;           // tempo change
    else currentBpm += (median - currentBpm) * 0.35;                              // refine
    conf = conf * 0.6 + c * 0.4;
}

void BeatTracker::alignPhase()
{
    if (currentBpm <= 0.0) return;
    const double period = 60.0 * hopRate / currentBpm;   // hops per beat
    const size_t N = std::min(filled, size_t(period * 6.0));
    if (N < size_t(period * 2.0)) return;

    // comb: find offset (in hops, looking back from 'now') with strongest periodic onset energy
    const int steps = std::max(8, int(period));
    double best = -1, bestOffset = 0;
    for (int s = 0; s < steps; ++s)
    {
        const double off = period * s / steps;
        double acc = 0;
        for (double k = off; k < double(N); k += period)
        {
            const size_t back = size_t(k);
            const size_t idx = (writePos + odfHistory.size() - 1 - back) % odfHistory.size();
            acc += odfHistory[idx];
        }
        if (acc > best) { best = acc; bestOffset = off; }
    }
    // last beat happened 'bestOffset' hops ago → phase now = bestOffset / period
    const double target = bestOffset / period;
    double err = target - phaseValue;
    err -= std::round(err);                       // wrap to [-0.5, 0.5]
    phaseValue += err * 0.5;
    phaseValue -= std::floor(phaseValue);
}
} // namespace dali
