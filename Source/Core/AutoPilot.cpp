#include "AutoPilot.h"
#include "../Render/Library.h"

namespace dali
{
void AutoPilot::timerCallback()
{
    // Auto Pilot keeps its OWN musical time (beats = elapsed time x BPM / 60). It used to read a bar
    // counter that every render engine (preview AND output, each with its own clock) overwrote with a
    // different value: the "bar" jumped back and forth and every jump switched the scene.
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const double dt = lastTime > 0.0 ? juce::jlimit(0.0, 0.5, now - lastTime) : 0.0;
    lastTime = now;

    const auto drops = state.analyzer.snapshot().features.dropCount;
    const int mode = juce::roundToInt(apvts.getRawParameterValue(params::id::autoPilot)->load());
    auto* p = apvts.getParameter(params::id::scene);
    if (p == nullptr) return;
    const int current = juce::roundToInt(p->convertFrom0to1(p->getValue()));

    // a scene chosen by hand restarts the count
    if (current != lastScene) { beats = 0.0; lastScene = current; }

    if (mode == 0 || state.telemetry.activity.load() < 0.5f) { lastDrops = drops; if (mode == 0) beats = varBeats = 0.0; return; }

    const double bpm = juce::jlimit(60.0, 200.0, double(state.telemetry.bpm.load() > 1.0f ? state.telemetry.bpm.load() : 128.0f));
    static const int barsTable[] = { 2, 4, 8, 16, 32 };
    const int bars = barsTable[juce::jlimit(0, 4, juce::roundToInt(apvts.getRawParameterValue(params::id::autoBars)->load()))];
    const bool onDrop = apvts.getRawParameterValue(params::id::autoOnDrop)->load() > 0.5f;
    const bool dropHappened = drops != lastDrops;
    lastDrops = drops;

    // VARIATIONS (modes 1 and 2): one shared phrase key for every engine
    varBeats += dt * bpm / 60.0;
    if (varBeats >= double(bars) * 4.0 || (onDrop && dropHappened)) { varBeats = 0.0; ++state.telemetry.variationKey; }

    if (mode != 2) { beats = 0.0; return; }
    beats += dt * bpm / 60.0;
    const bool dropNow = onDrop && dropHappened && beats >= 4.0;          // at least one bar on every scene
    if (beats < double(bars) * 4.0 && !dropNow) return;

    // the journey moves between the 3D scenes; the Image Reactor is never chosen automatically
    const int n = int(sceneLibrary().size());
    juce::Array<int> pool;
    for (int i = 0; i < n; ++i)
        if (i != current && i != kImageSceneIndex) pool.add(i);
    if (pool.isEmpty()) return;
    const int next = pool[random.nextInt(pool.size())];
    beats = 0.0;
    lastScene = next;
    p->beginChangeGesture();
    p->setValueNotifyingHost(p->convertTo0to1(float(next)));
    p->endChangeGesture();
}
} // namespace dali
