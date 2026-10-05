#pragma once
// ============================================================================
//  EffectChain — user-defined processing order of the 20 GPU effects.
//  (On/amount/P2 are automatable parameters; the order is preset state.)
// ============================================================================
#include <juce_data_structures/juce_data_structures.h>
#include "Library.h"
#include <array>
#include <numeric>

namespace dali
{
class EffectChain : public juce::ChangeBroadcaster
{
public:
    static constexpr int kNumEffects = 20;
    using Order = std::array<int, kNumEffects>;

    EffectChain() { reset(); }

    Order getOrder() const { const juce::SpinLock::ScopedLockType sl(lock); return order; }

    void setOrder(const Order& o)
    {
        // accept only permutations
        std::array<bool, kNumEffects> seen {};
        for (int v : o) { if (v < 0 || v >= kNumEffects || seen[size_t(v)]) return; seen[size_t(v)] = true; }
        { const juce::SpinLock::ScopedLockType sl(lock); order = o; }
        sendChangeMessage();
    }

    void reset() { Order o; std::iota(o.begin(), o.end(), 0); setOrder(o); }

    /** Moves the effect at chain position 'pos' by delta (-1 up / +1 down). */
    void move(int pos, int delta)
    {
        auto o = getOrder();
        const int np = pos + delta;
        if (pos < 0 || pos >= kNumEffects || np < 0 || np >= kNumEffects) return;
        std::swap(o[size_t(pos)], o[size_t(np)]);
        setOrder(o);
    }

    juce::ValueTree toValueTree() const
    {
        juce::ValueTree v(treeId);
        juce::StringArray ids;
        for (int e : getOrder()) ids.add(effectLibrary()[size_t(e)].id);
        v.setProperty("order", ids.joinIntoString(","), nullptr);
        return v;
    }

    void fromValueTree(const juce::ValueTree& v)
    {
        if (!v.hasType(treeId)) { reset(); return; }
        auto ids = juce::StringArray::fromTokens(v.getProperty("order").toString(), ",", "");
        Order o; int n = 0;
        std::array<bool, kNumEffects> used {};
        for (auto& id : ids)
            for (int e = 0; e < kNumEffects; ++e)
                if (id == effectLibrary()[size_t(e)].id && !used[size_t(e)] && n < kNumEffects) { o[size_t(n++)] = e; used[size_t(e)] = true; }
        for (int e = 0; e < kNumEffects; ++e) if (!used[size_t(e)]) o[size_t(n++)] = e;   // new effects appended
        setOrder(o);
    }

    static inline const juce::Identifier treeId { "EffectChain" };

private:
    mutable juce::SpinLock lock;
    Order order {};
};
} // namespace dali
