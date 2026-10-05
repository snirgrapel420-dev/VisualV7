#pragma once
// ============================================================================
//  OutputManager — fullscreen visual output on any connected display.
//  The output window is borderless, covers the chosen display completely and
//  shows only the visuals (no UI). ESC or double-click closes it.
//  Owned by the processor, so the output keeps running while the plug-in
//  editor is closed. Also manages the FrameSink registry (Spout / Syphon).
// ============================================================================
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Core/EngineState.h"
#include "FrameSink.h"

namespace dali
{
class OutputManager : public juce::ChangeBroadcaster
{
public:
    explicit OutputManager(EngineState& state);
    ~OutputManager() override;

    struct DisplayInfo { int index; juce::String name; juce::Rectangle<int> area; bool isMain; };
    static juce::Array<DisplayInfo> getDisplays();

    /** Message thread. index -1 = the stored choice, falling back to the last (usually external) display. */
    void open(int displayIndex = -1);
    void close();
    void toggle() { isOpen() ? close() : open(); }

    /** Shows a large number on every connected display for ~2.5 s (like a projector setup). */
    void identifyDisplays();
    bool isOpen() const noexcept { return window != nullptr; }

    /** Message thread. Sinks must outlive their registration. */
    void addFrameSink(FrameSink* s);
    void removeFrameSink(FrameSink* s);

private:
    class OutputWindow;
    class IdentifyWindow;
    juce::OwnedArray<juce::Component> identifyWindows;
    EngineState& state;
    std::unique_ptr<OutputWindow> window;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputManager)
};
} // namespace dali
