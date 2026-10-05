#pragma once
// ============================================================================
//  MIDIMapper — MIDI control of the instrument.
//   • MIDI Learn: any parameter ← any CC (right-click a control → MIDI Learn)
//   • Scene switching: Program Change 0-7, or notes C1-G1 (36-43)
//   • MIDI Trigger: every note-on fires the "MIDI Trigger" modulation source
//  The audio thread only pushes raw messages into a wait-free FIFO; mapping is
//  applied on the message thread (60 Hz) with proper host notification.
// ============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Audio/SpscRing.h"
#include "../Core/EngineState.h"
#include <map>

namespace dali
{
class MidiMapper : public juce::ChangeBroadcaster, private juce::Timer
{
public:
    MidiMapper(juce::AudioProcessorValueTreeState& apvts, EngineState& state);
    ~MidiMapper() override;

    /** Audio thread. Wait-free. */
    void pushFromAudioThread(const juce::MidiMessage& m) noexcept;

    // ---- learn / mapping (message thread) -------------------------------------------------
    void startLearn(const juce::String& paramId);
    void cancelLearn();
    juce::String getLearnTarget() const { return learnTarget; }
    void clearMapping(const juce::String& paramId);
    void clearAll();
    juce::String describeMapping(const juce::String& paramId) const;    // e.g. "CC 21 / Ch 1", or ""

    std::atomic<bool> noteSceneSwitching { true };
    std::atomic<bool> programChangeScenes { true };

    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& v);
    static inline const juce::Identifier treeId { "MidiMap" };

    /** Last received message, for the Settings panel. */
    juce::String getLastMessageText() const { return lastMessage; }

private:
    struct Raw { std::uint8_t b0, b1, b2; };
    void timerCallback() override;
    void handle(const Raw& r);
    void setParam(const juce::String& id, float normalised);
    static int key(int channel, int cc) { return (channel << 8) | cc; }

    juce::AudioProcessorValueTreeState& apvts;
    EngineState& state;
    SpscRing<Raw> fifo { 1024 };
    std::map<int, juce::String> ccMap;     // key(channel, cc) → parameter id
    juce::String learnTarget, lastMessage;
};
} // namespace dali
