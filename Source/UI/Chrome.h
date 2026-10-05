#pragma once
// ============================================================================
//  Editor chrome
//   HeaderBar      logo · scene · audio source · output display +
//                  identify + GO LIVE · panel toggle · settings
//   MeterBar       Bass · Mid · High · Energy · Kick/Snare/Hat · Build · BPM ·
//                  FPS · CPU · signal status
//   SettingsWindow a real top-level window (the OpenGL preview is a native
//                  child window on Windows, so nothing may be drawn over it)
// ============================================================================
#include "ParamControls.h"
#include "DaliLookAndFeel.h"

namespace dali
{
/** Output-display chooser, shared by the header and the settings window. */
class DisplayCombo : public juce::ComboBox, private juce::Timer
{
public:
    explicit DisplayCombo(DaliVisualProcessor& p);
    void refresh();
private:
    void timerCallback() override;
    DaliVisualProcessor& proc;
    int lastCount = -1;
};

/** Audio source chooser (standalone). */
class SourceCombo : public juce::ComboBox, private juce::Timer
{
public:
    explicit SourceCombo(DaliVisualProcessor& p);
private:
    void timerCallback() override;
    DaliVisualProcessor& proc;
};

class HeaderBar : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    explicit HeaderBar(DaliVisualProcessor& p);
    ~HeaderBar() override;
    void paint(juce::Graphics&) override;
    void resized() override;

    std::function<void()> onSettings, onTogglePanel;
    void setPanelVisible(bool v) { panelBtn.setToggleState(v, juce::dontSendNotification); }

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override { updateLiveButton(); }
    void timerCallback() override { updateLiveButton(); }
    void updateLiveButton();

    DaliVisualProcessor& proc;
    ParamCombo scene;
    SourceCombo source;
    DisplayCombo display;
    juce::TextButton identify { "ID" },
                     live { "GO LIVE" }, panelBtn { "PANEL" }, settings { "SETTINGS" },
                     undoBtn { juce::String::fromUTF8("\xe2\x86\xb6") }, redoBtn { juce::String::fromUTF8("\xe2\x86\xb7") },
                     chaos { "CHAOS" };
    std::unique_ptr<juce::ButtonParameterAttachment> chaosAttachment;
    struct Caption { juce::String text; juce::Rectangle<int> area; };
    juce::Array<Caption> captions;
};

class MeterBar : public juce::Component, private juce::Timer
{
public:
    explicit MeterBar(DaliVisualProcessor& p) : proc(p) { startTimerHz(30); }
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override;
    DaliVisualProcessor& proc;
    float bass = 0, mid = 0, high = 0, energy = 0, kick = 0, snare = 0, hat = 0, build = 0, drop = 0;
    float bpm = 0, fps = 0, outFps = 0, cpu = 0, frameMs = 0, activity = 0;
    int source = 2, musicalState = 0;
    float stateTime = 0;
    float quality = 1.0f;
    bool output = false;
};

class SettingsPanel : public juce::Component, private juce::Timer
{
public:
    explicit SettingsPanel(DaliVisualProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
    void refresh();
private:
    void timerCallback() override;
    DaliVisualProcessor& proc;
    SectionLabel outHeader { "Fullscreen Output" }, audioHeader { "Audio Source" }, midiHeader { "MIDI" },
                 infoHeader { "System" };
    DisplayCombo display;
    SourceCombo source;
    juce::ComboBox resolution;
    juce::Label displayLabel, resolutionLabel, sourceLabel, sourceStatus, midiLast, info;
    juce::ToggleButton vsync { "V-Sync (lock to the display refresh rate)" },
                       previewWhileOutput { "Keep the preview running while live" },
                       noteScenes { "Notes C1-G1 select scenes 1-8" },
                       programScenes { "Program Change selects scenes" };
    juce::TextButton identify { "Identify Displays" }, openOutput { "GO LIVE" },
                     clearMidi { "Clear all MIDI mappings" };
};

class SettingsWindow : public juce::DocumentWindow
{
public:
    SettingsWindow(DaliVisualProcessor& p, juce::LookAndFeel& lnf, juce::Component* centreOn);
    void closeButtonPressed() override { setVisible(false); }
    void show(juce::Component* centreOn);
};
} // namespace dali
