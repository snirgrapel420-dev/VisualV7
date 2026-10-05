#include "OutputManager.h"
#include "../UI/VisualView.h"
#include "NativeWindow.h"

namespace dali
{
class OutputManager::OutputWindow : public juce::Component, private juce::Timer
{
public:
    OutputWindow(EngineState& s, const juce::Rectangle<int>& area, juce::Point<int> physicalCentre, std::function<void()> onClose)
        : view(s, RenderEngine::Role::Output), closeCallback(std::move(onClose)), target(physicalCentre)
    {
        setOpaque(true);
        addAndMakeVisible(view);
        view.setInterceptsMouseClicks(false, false);
        setMouseCursor(juce::MouseCursor::NoCursor);
        setWantsKeyboardFocus(true);
        setBounds(area);
        addToDesktop(0);                     // no title bar, no border
        setAlwaysOnTop(true);
        setVisible(true);
        fillMonitor();
        toFront(true);
        grabKeyboardFocus();
        startTimerHz(30);
    }

    void resized() override { view.setBounds(getLocalBounds()); }
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }

    // ESC pressed twice (within 0.8 s) ends the live output - also when the DAW has the focus.
    bool keyPressed(const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey)
        {
           #if ! JUCE_WINDOWS
            registerEscape();                // Windows counts ESC in timerCallback (works without focus)
           #endif
            return true;
        }
        return false;
    }
    void mouseDoubleClick(const juce::MouseEvent&) override { requestClose(); }

private:
    void fillMonitor()
    {
        // exact physical monitor bounds (fixes a shrunken window on a display with different scaling)
        if (auto* peer = getPeer())
            native::fillMonitorAt(peer->getNativeHandle(), target.x, target.y);
    }

    void registerEscape()
    {
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (now - lastEscape < 800.0) { requestClose(); lastEscape = 0.0; }
        else lastEscape = now;
    }

    void timerCallback() override
    {
        // re-apply the monitor fit for the first second (Windows may rescale after a DPI change)
        if (++ticks == 4 || ticks == 15 || ticks == 30) fillMonitor();

        const bool esc = native::isEscapeDown();
        if (esc && !escWasDown) registerEscape();
        escWasDown = esc;
    }

    void requestClose()
    {
        auto cb = closeCallback;
        juce::MessageManager::callAsync([cb] { if (cb) cb(); });   // never delete ourselves inside our own callback
    }

    VisualView view;
    std::function<void()> closeCallback;
    juce::Point<int> target;
    double lastEscape = 0.0;
    bool escWasDown = true;                  // ignore an ESC that is still held from before
    int ticks = 0;
};

class OutputManager::IdentifyWindow : public juce::Component
{
public:
    IdentifyWindow(int number, const juce::String& info, const juce::Rectangle<int>& displayArea)
        : text(juce::String(number)), sub(info)
    {
        setOpaque(false);
        const int size = juce::jmin(420, juce::jmin(displayArea.getWidth(), displayArea.getHeight()) / 2);
        setBounds(juce::Rectangle<int>(size, size).withCentre(displayArea.getCentre()));
        addToDesktop(juce::ComponentPeer::windowIsTemporary);
        setAlwaysOnTop(true);
        setVisible(true);
    }
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xee0b0a10));
        g.fillRoundedRectangle(r, 28.0f);
        g.setColour(juce::Colour(0xffb24dff));
        g.drawRoundedRectangle(r.reduced(3.0f), 26.0f, 6.0f);
        g.setFont(juce::Font(juce::FontOptions(r.getHeight() * 0.55f, juce::Font::bold)));
        g.drawText(text, r.withTrimmedBottom(r.getHeight() * 0.18f), juce::Justification::centred);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.setFont(juce::Font(juce::FontOptions(juce::jmax(12.0f, r.getHeight() * 0.06f))));
        g.drawText(sub, r.removeFromBottom(r.getHeight() * 0.22f), juce::Justification::centred);
    }
private:
    juce::String text, sub;
};

OutputManager::OutputManager(EngineState& s) : state(s) {}

void OutputManager::identifyDisplays()
{
    identifyWindows.clear();
    for (auto& d : getDisplays())
        identifyWindows.add(new IdentifyWindow(d.index + 1, d.name.fromFirstOccurrenceOf("  ", false, false).trim(), d.area));
    juce::Component::SafePointer<juce::Component> first(identifyWindows.isEmpty() ? nullptr : identifyWindows.getFirst());
    juce::Timer::callAfterDelay(2500, [this, first]
    {
        if (first != nullptr) identifyWindows.clear();       // only if this batch is still showing
    });
}

OutputManager::~OutputManager()
{
    identifyWindows.clear();
    window.reset();
    state.telemetry.outputActive = false;
}

juce::Array<OutputManager::DisplayInfo> OutputManager::getDisplays()
{
    juce::Array<DisplayInfo> result;
    const auto& displays = juce::Desktop::getInstance().getDisplays().displays;
    for (int i = 0; i < displays.size(); ++i)
    {
        const auto& d = displays.getReference(i);
        const auto r = d.totalArea;
        result.add({ i, "Display " + juce::String(i + 1) + "  "
                        + juce::String(juce::roundToInt(r.getWidth() * d.scale)) + " x "
                        + juce::String(juce::roundToInt(r.getHeight() * d.scale)) + (d.isMain ? "  (main)" : ""),
                     r, d.isMain });
    }
    return result;
}

void OutputManager::open(int displayIndex)
{
    const auto displays = getDisplays();
    if (displays.isEmpty()) return;
    if (displayIndex < 0) displayIndex = state.output.displayIndex.load();
    if (displayIndex < 0 || displayIndex >= displays.size()) displayIndex = displays.size() - 1;
    state.output.displayIndex = displayIndex;

    window.reset();
    const auto& d = displays.getReference(displayIndex);
    const auto physicalCentre = juce::Desktop::getInstance().getDisplays().logicalToPhysical(d.area.getCentre());
    window = std::make_unique<OutputWindow>(state, d.area, physicalCentre, [this] { close(); });
    state.telemetry.outputActive = true;
    sendChangeMessage();
}

void OutputManager::close()
{
    if (window == nullptr) return;
    window.reset();
    state.telemetry.outputActive = false;
    sendChangeMessage();
}

void OutputManager::addFrameSink(FrameSink* s)
{
    const juce::SpinLock::ScopedLockType sl(state.sinkLock);
    state.sinks.addIfNotAlreadyThere(s);
}

void OutputManager::removeFrameSink(FrameSink* s)
{
    const juce::SpinLock::ScopedLockType sl(state.sinkLock);
    state.sinks.removeFirstMatchingValue(s);
}
} // namespace dali
