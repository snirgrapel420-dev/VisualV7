#pragma once
// ============================================================================
//  Side-panel pages: SCENE · AUDIO · MOD · FX · COLOR · IMAGE
// ============================================================================
#include "ParamControls.h"
#include "DaliLookAndFeel.h"
#include "Chrome.h"

namespace dali
{
class PanelBase : public juce::Component
{
public:
    virtual int preferredHeight(int width) = 0;
};

/** Hosts a PanelBase in a vertical scroller. */
class ScrollPanel : public juce::Component
{
public:
    explicit ScrollPanel(std::unique_ptr<PanelBase> c) : content(std::move(c))
    {
        viewport.setViewedComponent(content.get(), false);
        viewport.setScrollBarsShown(true, false);
        viewport.setScrollBarThickness(8);
        addAndMakeVisible(viewport);
    }
    void resized() override
    {
        viewport.setBounds(getLocalBounds());
        const int w = getWidth() - viewport.getScrollBarThickness();
        content->setSize(w, juce::jmax(getHeight(), content->preferredHeight(w)));
    }
    void paint(juce::Graphics& g) override { g.fillAll(colours::panel); }
    PanelBase& getContent() { return *content; }
private:
    std::unique_ptr<PanelBase> content;
    juce::Viewport viewport;
};

// ---------------------------------------------------------------------------------------------
class ScenePanel : public PanelBase, private juce::AudioProcessorValueTreeState::Listener, private juce::AsyncUpdater
{
public:
    explicit ScenePanel(DaliVisualProcessor& p);
    ~ScenePanel() override;
    int preferredHeight(int width) override;
    void resized() override;
private:
    class SceneTile;
    void parameterChanged(const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override;
    DaliVisualProcessor& proc;
    SectionLabel sceneHeader { "Scenes" }, macroHeader { "Scene Controls" }, motionHeader { "Motion & Energy" },
                 responseHeader { "Music Response" }, autoHeader { "Auto Pilot" };
    juce::OwnedArray<SceneTile> tiles;
    juce::Label description, autoHint;
    ParamKnob macroA, macroB, macroC, macroD, intensity, speed, drive, idle, dynamics, autoFx;
    ParamKnob trip, smoothMotion, kickResp, bassResp, midResp, highResp, transition;
    ParamCombo autoMode, autoBars;
    ParamToggle autoOnDrop;
    ParamToggle sceneInitToggle;
    juce::TextButton resetScene { "Reset Scene" };
};

// ---------------------------------------------------------------------------------------------
class AudioPanel : public PanelBase, private juce::Timer
{
public:
    explicit AudioPanel(DaliVisualProcessor& p);
    int preferredHeight(int width) override;
    void resized() override;
private:
    void timerCallback() override;
    DaliVisualProcessor& proc;
    SectionLabel sourceHeader { "Audio Source" }, inputHeader { "Analysis" }, reactHeader { "Reaction" }, syncHeader { "Sync" };
    SourceCombo source;
    juce::Label sourceStatus;
    ParamKnob sensitivity, smoothing, bass, mid, high, transient, internalBpm;
    ParamCombo syncSource, syncDiv;
    juce::Label syncSourceLabel, syncDivLabel, readout;
};

// ---------------------------------------------------------------------------------------------
class ModPanel : public PanelBase, private juce::ChangeListener
{
public:
    explicit ModPanel(DaliVisualProcessor& p);
    ~ModPanel() override;
    int preferredHeight(int width) override;
    void resized() override;
private:
    class Row;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    DaliVisualProcessor& proc;
    SectionLabel header { "Modulation Matrix  |  any source > any target" };
    juce::TextButton clearAll { "Clear All" };
    juce::OwnedArray<Row> rows;
};

// ---------------------------------------------------------------------------------------------
class FxPanel : public PanelBase, private juce::ChangeListener
{
public:
    explicit FxPanel(DaliVisualProcessor& p);
    ~FxPanel() override;
    int preferredHeight(int width) override;
    void resized() override;
private:
    class Row;
    void changeListenerCallback(juce::ChangeBroadcaster*) override { resized(); }
    DaliVisualProcessor& proc;
    SectionLabel header { "Effects Rack" };
    juce::TextButton resetOrder { "Reset Order" }, allOff { "All Off" };
    juce::Label intro;
    juce::OwnedArray<Row> rows;       // indexed by effect id (library order)
};

// ---------------------------------------------------------------------------------------------
class ColorPanel : public PanelBase
{
public:
    explicit ColorPanel(DaliVisualProcessor& p);
    ~ColorPanel() override;
    int preferredHeight(int width) override;
    void resized() override;
private:
    class Swatch;
    DaliVisualProcessor& proc;
    SectionLabel paletteHeader { "Image Reactor Palette" }, gradeHeader { "Colour  (all scenes)" }, customHeader { "Custom Palette (Image Reactor)" };
    juce::OwnedArray<Swatch> swatches;
    ParamKnob hue, saturation, brightness, contrast, colorAmount, colorShift, audioColor, bloom, sharpen, customA, customB;
};

// ---------------------------------------------------------------------------------------------
class ImagePanel : public PanelBase, private juce::ChangeListener, private juce::Timer
{
public:
    explicit ImagePanel(DaliVisualProcessor& p);
    ~ImagePanel() override;
    int preferredHeight(int width) override;
    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override { repaint(); status.setText(proc.image.getStatus(), juce::dontSendNotification); }
    void timerCallback() override;
    void chooseImage();
    void saveTemplate();
    void loadTemplate();
    void showImageScene();

    DaliVisualProcessor& proc;
    SectionLabel sourceHeader { "Image Reactor" }, modeHeader { "Visual Mode" }, controlHeader { "Image Controls" },
                 overlayHeader { "Overlay On Another Scene (advanced)" };
    juce::Label hint, status;
    juce::Rectangle<int> dropZone;
    juce::TextButton loadBtn { "Load Image" }, clearBtn { "Clear" }, showBtn { "SHOW IMAGE VISUAL" },
                     saveTpl { "Save" }, loadTpl { "Load" }, resetTpl { "Reset" }, routesBtn { "Audio Routes" };
    juce::OwnedArray<juce::TextButton> modeButtons;
    juce::OwnedArray<ParamKnob> knobs, overlayKnobs;
    ParamToggle overlay, mirror, kaleido;
    ParamCombo overlayMode, blend;
    std::unique_ptr<juce::FileChooser> chooser;
};
} // namespace dali
