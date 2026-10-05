#include "ModulationCore.h"
#include <algorithm>
#include <cmath>

namespace dali
{
float ModulationEngine::shapeCurve(float x, float curve) noexcept
{
    x = std::clamp(x, 0.0f, 1.0f);
    if (std::abs(curve) < 1e-3f) return x;
    // curve > 0 → logarithmic (fast rise), curve < 0 → exponential (slow rise)
    const float k = std::pow(4.0f, -curve);
    return std::pow(x, k);
}

void ModulationEngine::process(const ModSlotArray& slots, const ModSourceValues& src, float dt,
                               float* offsets, int numTargets) noexcept
{
    for (int t = 0; t < numTargets; ++t) offsets[t] = 0.0f;
    dt = std::clamp(dt, 0.0f, 0.25f);

    for (int i = 0; i < kMaxModSlots; ++i)
    {
        const ModSlot& s = slots[size_t(i)];
        if (!s.isActive() || s.source >= int(ModSource::count)) { lastOut[size_t(i)] = 0.0f; env[size_t(i)] = 0.0f; continue; }

        float x = std::clamp(src[ModSource(s.source)] * s.sensitivity, 0.0f, 1.0f);
        if (s.invert) x = 1.0f - x;

        // attack / release envelope
        float& e = env[size_t(i)];
        const float tc = (x > e ? s.attackMs : s.releaseMs) * 0.001f;
        e += (x - e) * (tc <= 1e-4f ? 1.0f : 1.0f - std::exp(-dt / tc));

        // extra smoothing
        float& sm = smooth[size_t(i)];
        const float st = s.smoothingMs * 0.001f;
        sm += (e - sm) * (st <= 1e-4f ? 1.0f : 1.0f - std::exp(-dt / st));

        float y = shapeCurve(sm, s.curve);
        y = s.min + (s.max - s.min) * y;
        if (s.bipolar) y = y * 2.0f - 1.0f;
        lastOut[size_t(i)] = y;

        if (s.target < numTargets) offsets[s.target] += s.amount * y;
    }
}
} // namespace dali
