#include "PluginEditor.h"

DaliVisualEditor::DaliVisualEditor(DaliVisualProcessor& p)
    : AudioProcessorEditor(&p), proc(p),
      header(p), preview(p.engineState, dali::RenderEngine::Role::Preview), meters(p)
{
    setLookAndFeel(&lnf);

    addAndMakeVisible(header);
    addAndMakeVisible(preview);
    addAndMakeVisible(tabs);
    addAndMakeVisible(meters);

    auto add = [this](const juce::String& name, std::unique_ptr<dali::PanelBase> panel)
    {
        tabs.addTab(name, dali::colours::panel, new dali::ScrollPanel(std::move(panel)), true);
    };
    add("SCENE", std::make_unique<dali::ScenePanel>(p));
    add("AUDIO", std::make_unique<dali::AudioPanel>(p));
    add("MOD",   std::make_unique<dali::ModPanel>(p));
    add("FX",    std::make_unique<dali::FxPanel>(p));
    add("COLOR", std::make_unique<dali::ColorPanel>(p));
    add("IMAGE", std::make_unique<dali::ImagePanel>(p));
    tabs.setTabBarDepth(32);
    tabs.setOutline(0);

    header.onSettings = [this] { showSettings(); };
    header.onTogglePanel = [this] { setPanelVisible(!panelVisible); };
    preview.onDoubleClick = [this] { proc.output.toggle(); };

    setWantsKeyboardFocus(true);
    setResizable(true, true);
    setResizeLimits(1200, 700, 3840, 2160);
    setSize(1440, 860);
}

DaliVisualEditor::~DaliVisualEditor()
{
    settingsWindow = nullptr;          // uses our LookAndFeel: destroy it first
    setLookAndFeel(nullptr);
}

void DaliVisualEditor::showSettings()
{
    if (settingsWindow == nullptr) settingsWindow = std::make_unique<dali::SettingsWindow>(proc, lnf, this);
    else if (settingsWindow->isVisible()) settingsWindow->setVisible(false);
    else settingsWindow->show(this);
}

void DaliVisualEditor::setPanelVisible(bool v)
{
    panelVisible = v;
    tabs.setVisible(v);
    header.setPanelVisible(v);
    resized();
}

void DaliVisualEditor::paint(juce::Graphics& g)
{
    g.fillAll(dali::colours::bg);
}

void DaliVisualEditor::resized()
{
    auto r = getLocalBounds();
    header.setBounds(r.removeFromTop(54));
    meters.setBounds(r.removeFromBottom(38));
    if (panelVisible) tabs.setBounds(r.removeFromRight(juce::jlimit(380, 470, getWidth() / 3)));
    preview.setBounds(r.reduced(panelVisible ? 8 : 0));
}

bool DaliVisualEditor::keyPressed(const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z'))
    {
        if (k.getModifiers().isShiftDown()) proc.redo(); else proc.undo();
        return true;
    }
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Y' || k.getKeyCode() == 'y')) { proc.redo(); return true; }
    const int code = k.getKeyCode();
    if (code == 'F' || code == 'f') { proc.output.toggle(); return true; }
    if (k == juce::KeyPress::tabKey) { setPanelVisible(!panelVisible); return true; }
    if (k == juce::KeyPress::escapeKey && proc.output.isOpen()) { proc.output.close(); return true; }
    if (code >= '0' && code <= '9')                       // 1-9 = scenes 1-9, 0 = scene 10
    {
        const int sceneIndex = code == '0' ? 9 : code - '1';
        if (auto* prm = proc.apvts.getParameter(dali::params::id::scene))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost(prm->convertTo0to1(float(sceneIndex)));
            prm->endChangeGesture();
        }
        return true;
    }
    return false;
}

bool DaliVisualEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files) if (dali::ImageProcessor::isSupportedFile(juce::File(f))) return true;
    return false;
}

void DaliVisualEditor::filesDropped(const juce::StringArray& files, int, int)
{
    for (auto& path : files)
    {
        const juce::File f(path);
        if (dali::ImageProcessor::isSupportedFile(f) && proc.image.loadFile(f))
        {
            // the image becomes the visual itself, starting clean (nothing inherited from the last preset)
            proc.applyImageReactorLook(true);
            if (!panelVisible) setPanelVisible(true);
            tabs.setCurrentTabIndex(ImageTab);
            return;
        }
    }
}
