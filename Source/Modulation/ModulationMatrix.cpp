#include "ModulationMatrix.h"

namespace dali
{
juce::ValueTree ModulationMatrix::toValueTree() const
{
    juce::ValueTree v(treeId);
    const auto s = getSlots();
    for (int i = 0; i < kMaxModSlots; ++i)
    {
        const auto& m = s[size_t(i)];
        if (m.source <= 0 || m.target < 0) continue;
        juce::ValueTree c("Slot");
        c.setProperty("index", i, nullptr);
        c.setProperty("enabled", m.enabled, nullptr);
        c.setProperty("source", modSourceName(ModSource(m.source)), nullptr);
        c.setProperty("target", ModulationTarget { m.target }.paramId(), nullptr);
        c.setProperty("amount", m.amount, nullptr);
        c.setProperty("min", m.min, nullptr);
        c.setProperty("max", m.max, nullptr);
        c.setProperty("smoothing", m.smoothingMs, nullptr);
        c.setProperty("attack", m.attackMs, nullptr);
        c.setProperty("release", m.releaseMs, nullptr);
        c.setProperty("curve", m.curve, nullptr);
        c.setProperty("bipolar", m.bipolar, nullptr);
        c.setProperty("invert", m.invert, nullptr);
        c.setProperty("sensitivity", m.sensitivity, nullptr);
        v.appendChild(c, nullptr);
    }
    return v;
}

void ModulationMatrix::fromValueTree(const juce::ValueTree& v)
{
    ModSlotArray s {};
    for (auto& x : s) { x = ModSlot {}; x.source = 0; x.target = -1; }
    if (v.hasType(treeId))
    {
        const auto names = ModulationSource::allNames();
        for (auto c : v)
        {
            const int i = c.getProperty("index", -1);
            if (i < 0 || i >= kMaxModSlots) continue;
            ModSlot m;
            m.enabled     = c.getProperty("enabled", true);
            m.source      = juce::jmax(0, names.indexOf(c.getProperty("source").toString()));
            m.target      = ModulationTarget::fromParamId(c.getProperty("target").toString());
            m.amount      = c.getProperty("amount", 0.5f);
            m.min         = c.getProperty("min", 0.0f);
            m.max         = c.getProperty("max", 1.0f);
            m.smoothingMs = c.getProperty("smoothing", 30.0f);
            m.attackMs    = c.getProperty("attack", 5.0f);
            m.releaseMs   = c.getProperty("release", 150.0f);
            m.curve       = c.getProperty("curve", 0.0f);
            m.bipolar     = c.getProperty("bipolar", false);
            m.invert      = c.getProperty("invert", false);
            m.sensitivity = c.getProperty("sensitivity", 1.0f);
            s[size_t(i)] = m;
        }
    }
    setAll(s);
}
} // namespace dali
