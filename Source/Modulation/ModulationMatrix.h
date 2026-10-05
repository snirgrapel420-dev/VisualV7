#pragma once
// ============================================================================
//  ModulationMatrix — the editable modulation configuration (16 slots).
//  "Any source → any target": targets are every modulatable parameter
//  (params::modTargets()). Edited on the message thread, copied by the
//  render engines each frame under a short spin lock.
//
//  ModulationSource / ModulationTarget describe the two ends of a route.
// ============================================================================
#include <juce_data_structures/juce_data_structures.h>
#include "ModulationCore.h"
#include "../Core/Parameters.h"

namespace dali
{
/** A modulation source (audio feature, clock, MIDI, random). */
struct ModulationSource
{
    ModSource id;
    static juce::StringArray allNames()
    {
        juce::StringArray a;
        for (int i = 0; i < int(ModSource::count); ++i) a.add(modSourceName(ModSource(i)));
        return a;
    }
};

/** A modulation target (a modulatable parameter). */
struct ModulationTarget
{
    int targetIndex;                                  // index into params::modTargets()
    int paramIndex() const  { return params::modTargets()[size_t(targetIndex)]; }
    juce::String paramId() const { return params::all()[size_t(paramIndex())].id; }
    juce::String name() const    { return params::all()[size_t(paramIndex())].name; }
    static int count()           { return int(params::modTargets().size()); }
    static int fromParamId(const juce::String& id) { return params::modTargetOf(params::indexOf(id)); }
};

class ModulationMatrix : public juce::ChangeBroadcaster
{
public:
    ModulationMatrix() { clear(); }

    ModSlotArray getSlots() const
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        return slots;
    }

    ModSlot getSlot(int i) const
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        return slots[size_t(juce::jlimit(0, kMaxModSlots - 1, i))];
    }

    void setSlot(int i, const ModSlot& s)
    {
        {
            const juce::SpinLock::ScopedLockType sl(lock);
            slots[size_t(juce::jlimit(0, kMaxModSlots - 1, i))] = s;
        }
        version.fetch_add(1);
        sendChangeMessage();
    }

    void setAll(const ModSlotArray& s)
    {
        {
            const juce::SpinLock::ScopedLockType sl(lock);
            slots = s;
        }
        version.fetch_add(1);
        sendChangeMessage();
    }

    void clear()
    {
        ModSlotArray s {};
        for (auto& x : s) { x = ModSlot {}; x.source = 0; x.target = -1; }
        setAll(s);
    }

    /** Adds a route into the first free slot; returns the slot index or -1. */
    int addRoute(ModSource src, int targetIndex, float amount)
    {
        auto s = getSlots();
        for (int i = 0; i < kMaxModSlots; ++i)
            if (s[size_t(i)].source == 0 || s[size_t(i)].target < 0)
            {
                ModSlot n; n.source = int(src); n.target = targetIndex; n.amount = amount;
                setSlot(i, n);
                return i;
            }
        return -1;
    }

    /** Returns true if any enabled slot targets this parameter index. */
    bool isParamModulated(int paramIndex) const
    {
        const int t = params::modTargetOf(paramIndex);
        if (t < 0) return false;
        const juce::SpinLock::ScopedLockType sl(lock);
        for (auto& s : slots) if (s.isActive() && s.target == t) return true;
        return false;
    }

    std::uint32_t getVersion() const noexcept { return version.load(); }

    // ---- serialisation (targets stored by parameter id → robust across versions)
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& v);
    static inline const juce::Identifier treeId { "ModMatrix" };

private:
    mutable juce::SpinLock lock;
    ModSlotArray slots {};
    std::atomic<std::uint32_t> version { 0 };
};
} // namespace dali
