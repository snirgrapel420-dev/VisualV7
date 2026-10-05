#pragma once
// ============================================================================
//  DALI VISUAL — editor.
//   ┌── Header: logo · scene · preset · audio source · output display · GO LIVE ──┐
//   │                                            │ SCENE AUDIO MOD FX COLOR IMAGE │
//   │           live preview (OpenGL)            │          side panel            │
//   ├──── Bass · Mid · High · Energy · Kick/Snare/Hat · Build · BPM · FPS · CPU ───┤
//  Nothing is ever drawn on top of the preview: on Windows the OpenGL view is a
//  native child window, so dialogs (Settings) are real top-level windows.
//  Keys: 1-8 scenes · F go live · Tab hide/show panel · ESC stop output.
// ============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/DaliLookAndFeel.h"
#include "UI/VisualView.h"
#include "UI/Panels.h"
#include "UI/Chrome.h"

class DaliVisualEditor : public juce::AudioProcessorEditor,
                         public juce::FileDragAndDropTarget
{
public:
    explicit DaliVisualEditor(DaliVisualProcessor&);
    ~DaliVisualEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int, int) override;

private:
    enum Tab { SceneTab, AudioTab, ModTab, FxTab, ColorTab, ImageTab };
    void showSettings();
    void setPanelVisible(bool v);

    DaliVisualProcessor& proc;
    dali::DaliLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 700 };
    dali::HeaderBar header;
    dali::VisualView preview;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    dali::MeterBar meters;
    std::unique_ptr<dali::SettingsWindow> settingsWindow;
    bool panelVisible = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DaliVisualEditor)
};
