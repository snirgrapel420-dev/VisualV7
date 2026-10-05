#include "MidiMapper.h"
#include "../Render/Library.h"

namespace dali
{
MidiMapper::MidiMapper(juce::AudioProcessorValueTreeState& s, EngineState& st) : apvts(s), state(st)
{
    startTimerHz(60);
}

MidiMapper::~MidiMapper() { stopTimer(); }

void MidiMapper::pushFromAudioThread(const juce::MidiMessage& m) noexcept
{
    if (m.getRawDataSize() < 1 || m.getRawDataSize() > 3) return;     // ignore sysex
    const auto* d = m.getRawData();
    const Raw r { d[0], m.getRawDataSize() > 1 ? d[1] : std::uint8_t(0), m.getRawDataSize() > 2 ? d[2] : std::uint8_t(0) };
    fifo.push(&r, 1);
}

void MidiMapper::timerCallback()
{
    Raw buf[64];
    size_t n;
    while ((n = fifo.pop(buf, 64)) > 0)
        for (size_t i = 0; i < n; ++i) handle(buf[i]);
}

void MidiMapper::setParam(const juce::String& id, float normalised)
{
    if (auto* p = apvts.getParameter(id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, normalised));
        p->endChangeGesture();
    }
}

void MidiMapper::handle(const Raw& r)
{
    const int type = r.b0 & 0xF0, channel = (r.b0 & 0x0F) + 1;
    const int numScenes = int(sceneLibrary().size());
    auto selectScene = [&](int s)
    {
        if (auto* p = apvts.getParameter(params::id::scene))
            setParam(params::id::scene, p->convertTo0to1(float(juce::jlimit(0, numScenes - 1, s))));
    };

    if (type == 0xB0)                                           // Control Change
    {
        lastMessage = "CC " + juce::String(r.b1) + " = " + juce::String(r.b2) + "  (Ch " + juce::String(channel) + ")";
        if (learnTarget.isNotEmpty())
        {
            for (auto it = ccMap.begin(); it != ccMap.end();)    // one CC per parameter
                it = (it->second == learnTarget) ? ccMap.erase(it) : std::next(it);
            ccMap[key(channel, r.b1)] = learnTarget;
            learnTarget.clear();
            sendChangeMessage();
        }
        auto it = ccMap.find(key(channel, r.b1));
        if (it != ccMap.end()) setParam(it->second, r.b2 / 127.0f);
    }
    else if (type == 0xC0)                                      // Program Change
    {
        lastMessage = "Program " + juce::String(r.b1) + "  (Ch " + juce::String(channel) + ")";
        if (programChangeScenes.load() && r.b1 < numScenes) selectScene(r.b1);
    }
    else if (type == 0x90 && r.b2 > 0)                          // Note On
    {
        lastMessage = "Note " + juce::MidiMessage::getMidiNoteName(r.b1, true, true, 3) + " vel " + juce::String(r.b2)
                    + "  (Ch " + juce::String(channel) + ")";
        state.midiTriggerVelocity.store(r.b2 / 127.0f);
        state.midiTriggerCount.fetch_add(1);
        if (noteSceneSwitching.load() && r.b1 >= 36 && r.b1 < 36 + numScenes) selectScene(r.b1 - 36);
    }
}

void MidiMapper::startLearn(const juce::String& id) { learnTarget = id; sendChangeMessage(); }
void MidiMapper::cancelLearn() { learnTarget.clear(); sendChangeMessage(); }

void MidiMapper::clearMapping(const juce::String& id)
{
    for (auto it = ccMap.begin(); it != ccMap.end();)
        it = (it->second == id) ? ccMap.erase(it) : std::next(it);
    sendChangeMessage();
}

void MidiMapper::clearAll() { ccMap.clear(); learnTarget.clear(); sendChangeMessage(); }

juce::String MidiMapper::describeMapping(const juce::String& id) const
{
    for (auto& [k, v] : ccMap)
        if (v == id) return "CC " + juce::String(k & 0xFF) + " / Ch " + juce::String(k >> 8);
    return {};
}

juce::ValueTree MidiMapper::toValueTree() const
{
    juce::ValueTree v(treeId);
    v.setProperty("noteScenes", noteSceneSwitching.load(), nullptr);
    v.setProperty("programScenes", programChangeScenes.load(), nullptr);
    for (auto& [k, id] : ccMap)
    {
        juce::ValueTree m("Map");
        m.setProperty("channel", k >> 8, nullptr);
        m.setProperty("cc", k & 0xFF, nullptr);
        m.setProperty("param", id, nullptr);
        v.appendChild(m, nullptr);
    }
    return v;
}

void MidiMapper::fromValueTree(const juce::ValueTree& v)
{
    if (!v.hasType(treeId)) return;
    ccMap.clear();
    noteSceneSwitching = (bool) v.getProperty("noteScenes", true);
    programChangeScenes = (bool) v.getProperty("programScenes", true);
    for (auto m : v)
    {
        const juce::String id = m.getProperty("param").toString();
        if (apvts.getParameter(id) != nullptr)
            ccMap[key(int(m.getProperty("channel", 1)), int(m.getProperty("cc", 0)))] = id;
    }
    sendChangeMessage();
}
} // namespace dali
