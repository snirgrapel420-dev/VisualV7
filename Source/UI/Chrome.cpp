#include "Chrome.h"
#include "../Core/AppPrefs.h"
#include "../Output/OutputManager.h"

namespace dali
{
// =============================================================================
//  shared combos
// =============================================================================
DisplayCombo::DisplayCombo(DaliVisualProcessor& p) : proc(p)
{
    setTooltip("Which display the fullscreen output (GO LIVE) appears on");
    onChange = [this]
    {
        const int idx = getSelectedId() - 1;
        if (idx < 0) return;
        proc.engineState.output.displayIndex = idx;
        if (proc.output.isOpen()) proc.output.open(idx);        // move the live output right away
    };
    refresh();
    startTimer(1500);
}

void DisplayCombo::refresh()
{
    const auto displays = OutputManager::getDisplays();
    lastCount = displays.size();
    clear(juce::dontSendNotification);
    for (auto& d : displays) addItem(d.name, d.index + 1);
    int chosen = proc.engineState.output.displayIndex.load();
    if (chosen < 0 || chosen >= displays.size()) chosen = displays.size() - 1;   // default: last (usually external)
    setSelectedId(chosen + 1, juce::dontSendNotification);
}

void DisplayCombo::timerCallback()
{
    if (isPopupActive()) return;
    const int count = OutputManager::getDisplays().size();
    const int want = proc.engineState.output.displayIndex.load() + 1;
    if (count != lastCount || (want > 0 && want != getSelectedId())) refresh();   // monitor plugged in / changed elsewhere
}

SourceCombo::SourceCombo(DaliVisualProcessor& p) : proc(p)
{
    if (proc.canCaptureSystemAudio()) addItem("System Audio (what you hear)", 2);
    addItem(proc.isStandalone() ? "Audio Input (device)" : "Track Audio (host)", 1);
    setSelectedId(proc.getInputSource() + 1, juce::dontSendNotification);
    setEnabled(proc.isStandalone());
    onChange = [this] { proc.setInputSource(getSelectedId() - 1); };
    startTimer(500);
}

void SourceCombo::timerCallback()
{
    if (!isPopupActive() && getSelectedId() != proc.getInputSource() + 1)
        setSelectedId(proc.getInputSource() + 1, juce::dontSendNotification);
    setTooltip(proc.getInputSourceStatus());
}

// =============================================================================
//  HEADER
// =============================================================================
HeaderBar::HeaderBar(DaliVisualProcessor& p) : proc(p), scene(p, params::id::scene), source(p), display(p)
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &scene, &source, &display, &identify, &live, &panelBtn, &settings,
                                                                        &undoBtn, &redoBtn, &chaos })
        addAndMakeVisible(c);

    identify.onClick = [this] { proc.output.identifyDisplays(); };
    undoBtn.onClick = [this] { proc.undo(); };
    redoBtn.onClick = [this] { proc.redo(); };
    undoBtn.setTooltip("Undo (Ctrl+Z): resets, scene inits, cleared routes...");
    redoBtn.setTooltip("Redo (Ctrl+Y)");
    chaos.setClickingTogglesState(true);
    chaos.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffd81b60));
    chaos.setTooltip("CHAOS: everything to the most psychedelic level - full trip, deepest symmetry and trails, "
                     "the music read as its wildest. MIDI-learnable (right-click is not needed: map it in the host)");
    if (auto* prm = proc.apvts.getParameter(dali::params::id::chaosMode))
        chaosAttachment = std::make_unique<juce::ButtonParameterAttachment>(*prm, chaos);
    live.onClick = [this] { proc.output.toggle(); updateLiveButton(); };
    panelBtn.setClickingTogglesState(true);
    panelBtn.setToggleState(true, juce::dontSendNotification);
    panelBtn.onClick = [this] { if (onTogglePanel) onTogglePanel(); };
    settings.onClick = [this] { if (onSettings) onSettings(); };

    identify.setTooltip("Show the number of every display on its screen");
    live.setTooltip("Fullscreen output on the chosen display (key F). ESC or double-click ends it.");
    panelBtn.setTooltip("Show / hide the side panel (Tab) - bigger preview for performing");
    scene.setTooltip("Scene (keys 1-8)");

    proc.output.addChangeListener(this);
    proc.historyChanged.addChangeListener(this);
    updateLiveButton();
    startTimerHz(4);
}

HeaderBar::~HeaderBar()
{
    proc.historyChanged.removeChangeListener(this);
    proc.output.removeChangeListener(this);
}

void HeaderBar::updateLiveButton()
{
    undoBtn.setEnabled(proc.canUndo());
    redoBtn.setEnabled(proc.canRedo());
    const bool on = proc.output.isOpen();
    live.setToggleState(on, juce::dontSendNotification);
    live.setButtonText(on ? "LIVE  - STOP" : "GO LIVE");
    live.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe0304a));
}

void HeaderBar::paint(juce::Graphics& g)
{
    g.fillAll(colours::bg);
    g.setColour(colours::outline);
    g.fillRect(getLocalBounds().removeFromBottom(1));

    juce::ColourGradient grad(colours::accent, 14.0f, 0.0f, colours::accent2, 180.0f, 0.0f, false);
    g.setGradientFill(grad);
    g.setFont(juce::Font(juce::FontOptions(21.0f, juce::Font::bold)).withExtraKerningFactor(0.12f));
    g.drawText("DALI VISUAL", juce::Rectangle<int>(14, 8, 180, 26), juce::Justification::centredLeft);
    g.setColour(colours::textDim);
    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)).withExtraKerningFactor(0.3f));
    g.drawText("BY DALI AUDIO", juce::Rectangle<int>(15, 34, 180, 12), juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)).withExtraKerningFactor(0.15f));
    for (auto& c : captions)
    {
        g.setColour(colours::textDim);
        g.drawText(c.text, c.area, juce::Justification::centredLeft);
    }
    // separators between groups
    g.setColour(colours::outline);
    for (auto* c : { (juce::Component*) &source, (juce::Component*) &display, (juce::Component*) &panelBtn })
        g.fillRect(c->getX() - 9, 10, 1, getHeight() - 20);
}

void HeaderBar::resized()
{
    captions.clearQuick();
    auto r = getLocalBounds().reduced(10, 0);
    r.removeFromLeft(185);
    const int capH = 13, ctlY = 20, ctlH = 26;
    auto place = [&](juce::Component& c, int x, int w, const juce::String& caption = {})
    {
        c.setBounds(x, ctlY, w, ctlH);
        if (caption.isNotEmpty()) captions.add({ caption, juce::Rectangle<int>(x + 2, 5, w, capH) });
    };

    // right side (fixed)
    int x = r.getRight();
    x -= 86;  place(settings, x, 86);
    x -= 70;  place(panelBtn, x, 64);
    x -= 18 + 104; place(live, x, 104);
    x -= 40;  place(identify, x, 36, "");
    x -= 200; place(display, x, 196, "OUTPUT DISPLAY");
    const int sourceW = 188;
    x -= 18 + sourceW; place(source, x, sourceW, "AUDIO SOURCE");
    const int rightStart = x - 18;

    // left side (flexible): the scene chooser, then CHAOS and undo / redo
    const int lx = r.getX();
    const int sceneW = juce::jlimit(180, 300, rightStart - lx - 8 - 92 - 64);
    place(scene, lx, sceneW, "SCENE");
    place(chaos, lx + sceneW + 8, 84);
    place(undoBtn, lx + sceneW + 8 + 90, 30);
    place(redoBtn, lx + sceneW + 8 + 90 + 32, 30);
}

// =============================================================================
//  METERS
// =============================================================================
void MeterBar::timerCallback()
{
    const auto f = proc.analyzer.snapshot().features;
    const auto& t = proc.engineState.telemetry;
    activity = t.activity.load();
    auto fall = [](float cur, float v) { return v > cur ? v : cur * 0.86f + v * 0.14f; };
    const float a = activity;
    bass = fall(bass, f.bassEnv * a); mid = fall(mid, f.midEnv * a); high = fall(high, f.highEnv * a);
    energy = fall(energy, f.energy * a);
    kick = f.kick * a; snare = f.snare * a; hat = f.hat * a; build = f.build * a; drop = f.drop * a;
    musicalState = f.state; stateTime = f.stateTime;
    bpm = t.bpm.load(); fps = t.previewFps.load(); outFps = t.outputFps.load(); quality = t.quality.load();
    cpu = t.cpuLoad.load(); frameMs = t.frameMs.load(); source = t.clockSource.load();
    output = t.outputActive.load();
    repaint();
}

void MeterBar::paint(juce::Graphics& g)
{
    using namespace colours;
    g.fillAll(bg);
    g.setColour(outline);
    g.fillRect(getLocalBounds().removeFromTop(1));

    auto r = getLocalBounds().reduced(14, 8);
    const juce::Font labelFont(juce::FontOptions(9.5f, juce::Font::bold));
    const juce::Font valueFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));

    auto bar = [&](const char* name, float v, juce::Colour c, int w)
    {
        auto cell = r.removeFromLeft(w);
        r.removeFromLeft(10);
        g.setColour(textDim); g.setFont(labelFont);
        g.drawText(name, cell.removeFromLeft(40), juce::Justification::centredLeft);
        auto b = cell.withSizeKeepingCentre(cell.getWidth(), 7).toFloat();
        g.setColour(panel2); g.fillRoundedRectangle(b, 3.5f);
        g.setColour(c); g.fillRoundedRectangle(b.withWidth(b.getWidth() * juce::jlimit(0.0f, 1.0f, v)), 3.5f);
    };
    auto led = [&](const char* name, float v, juce::Colour c)
    {
        auto cell = r.removeFromLeft(54);
        g.setColour(textDim); g.setFont(labelFont);
        g.drawText(name, cell.removeFromLeft(36), juce::Justification::centredLeft);
        auto d = cell.withSizeKeepingCentre(12, 12).toFloat();
        g.setColour(panel2); g.fillEllipse(d);
        g.setColour(c.withAlpha(juce::jlimit(0.0f, 1.0f, v))); g.fillEllipse(d);
    };
    auto value = [&](const char* name, const juce::String& valueText, int w)
    {
        auto cell = r.removeFromLeft(w);
        g.setColour(textDim); g.setFont(labelFont);
        g.drawText(name, cell.removeFromLeft(30), juce::Justification::centredLeft);
        g.setColour(colours::text); g.setFont(valueFont);
        g.drawText(valueText, cell, juce::Justification::centredLeft);
    };

    bar("BASS", bass, accent, 98);
    bar("MID", mid, accent2, 98);
    bar("HIGH", high, modRing, 98);
    bar("ENERGY", energy, colours::text.withAlpha(0.8f), 98);
    led("KICK", kick, accent);
    led("SNARE", snare, accent2);
    led("HAT", hat, modRing);
    r.removeFromLeft(6);
    bar("BUILD", juce::jmax(build, drop), drop > 0.05f ? juce::Colour(0xffff5c7a) : learn, 110);

    static const char* stateNames[] = { "CALM", "BUILD", "PEAK", "CHAOS", "RELEASE" };
    if (activity > 0.05f) value("", juce::String(stateNames[juce::jlimit(0, 4, musicalState)]) + " " + juce::String(stateTime, 0) + "s", 112);
    else r.removeFromLeft(112);
    static const char* srcNames[] = { "HOST", "DETECT", "INT" };
    value("BPM", activity > 0.05f ? juce::String(bpm, 1) + " " + srcNames[juce::jlimit(0, 2, source)] : juce::String("--"), 118);
    value("FPS", (output ? juce::String(juce::roundToInt(outFps)) + " LIVE" : juce::String(juce::roundToInt(fps)))
                 + " Q" + juce::String(juce::roundToInt(quality * 100.0f)), 118);
    value("CPU", juce::String(cpu * 100.0f, 1) + "%", 76);

    if (activity < 0.05f)
    {
        g.setColour(learn);
        g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        const juce::String hint = proc.getInputSource() == DaliVisualProcessor::SystemAudio
                                      ? "NO SIGNAL - play something on this computer"
                                      : (proc.isStandalone() ? "NO SIGNAL - choose an input or System Audio"
                                                             : "NO SIGNAL - play the track");
        g.drawFittedText(hint, r, juce::Justification::centredRight, 1);
    }
}

// =============================================================================
//  SETTINGS
// =============================================================================
SettingsPanel::SettingsPanel(DaliVisualProcessor& p) : proc(p), display(p), source(p)
{
    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &outHeader, &audioHeader, &midiHeader, &infoHeader, &display, &source, &resolution, &displayLabel,
             &resolutionLabel, &sourceLabel, &sourceStatus, &midiLast, &info, &vsync, &previewWhileOutput,
             &noteScenes, &programScenes, &identify, &openOutput, &clearMidi, &resetAll, &restoreSession })
        addAndMakeVisible(c);

    displayLabel.setText("Output display", juce::dontSendNotification);
    resolutionLabel.setText("Render resolution", juce::dontSendNotification);
    sourceLabel.setText("Listen to", juce::dontSendNotification);
    for (auto* l : { &displayLabel, &resolutionLabel, &sourceLabel, &sourceStatus, &midiLast, &info })
    {
        l->setFont(juce::Font(juce::FontOptions(12.5f)));
        l->setColour(juce::Label::textColourId, colours::textDim);
    }
    info.setJustificationType(juce::Justification::topLeft);
    sourceStatus.setJustificationType(juce::Justification::topLeft);

    resolution.addItem("Adaptive  (recommended: always smooth)", 1);
    resolution.addItem("50 %  (fastest)", 2);
    resolution.addItem("75 %", 3);
    resolution.addItem("100 %  (native, fixed)", 4);
    resolution.addItem("150 %  (supersampled - very heavy)", 5);
    resolution.onChange = [this] { proc.engineState.output.renderScaleIndex = resolution.getSelectedId() - 1; };
    vsync.onClick = [this] { proc.engineState.output.vsync = vsync.getToggleState(); };
    previewWhileOutput.onClick = [this] { proc.engineState.output.previewWhileOutput = previewWhileOutput.getToggleState(); };
    noteScenes.onClick = [this] { proc.midi.noteSceneSwitching = noteScenes.getToggleState(); };
    programScenes.onClick = [this] { proc.midi.programChangeScenes = programScenes.getToggleState(); };
    identify.onClick = [this] { proc.output.identifyDisplays(); };
    openOutput.onClick = [this] { proc.output.toggle(); refresh(); };
    clearMidi.onClick = [this] { proc.midi.clearAll(); };
    resetAll.setTooltip("All parameters, modulation, effects and the image back to the start, first scene at its init (Ctrl+Z undoes)");
    resetAll.onClick = [this] { proc.resetEverything(); };
    restoreSession.setTooltip("Off: every launch starts fresh (display, render, MIDI mappings and audio source are always kept). "
                              "On: the last session's scene, modulation, effects and image come back too.");
    restoreSession.setToggleState(AppPrefs::restoreSession(), juce::dontSendNotification);
    restoreSession.onClick = [this] { AppPrefs::setRestoreSession(restoreSession.getToggleState()); };
    restoreSession.setVisible(proc.isStandalone());
    setSize(580, 680);
    refresh();
    startTimerHz(4);
}

void SettingsPanel::refresh()
{
    display.refresh();
    resolution.setSelectedId(proc.engineState.output.renderScaleIndex.load() + 1, juce::dontSendNotification);
    vsync.setToggleState(proc.engineState.output.vsync.load(), juce::dontSendNotification);
    previewWhileOutput.setToggleState(proc.engineState.output.previewWhileOutput.load(), juce::dontSendNotification);
    noteScenes.setToggleState(proc.midi.noteSceneSwitching.load(), juce::dontSendNotification);
    programScenes.setToggleState(proc.midi.programChangeScenes.load(), juce::dontSendNotification);
    openOutput.setButtonText(proc.output.isOpen() ? "STOP LIVE OUTPUT" : "GO LIVE");
}

void SettingsPanel::timerCallback()
{
    if (!isShowing()) return;
    openOutput.setButtonText(proc.output.isOpen() ? "STOP LIVE OUTPUT" : "GO LIVE");
    sourceStatus.setText(proc.getInputSourceStatus(), juce::dontSendNotification);
    const auto last = proc.midi.getLastMessageText();
    midiLast.setText("Last MIDI: " + (last.isNotEmpty() ? last : juce::String("-")), juce::dontSendNotification);
    juce::String renderer;
    { const juce::SpinLock::ScopedLockType sl(proc.engineState.telemetry.infoLock); renderer = proc.engineState.telemetry.rendererInfo; }
    juce::String s;
    s << "Renderer: " << (renderer.isNotEmpty() ? renderer : juce::String("starting...")) << "\n"
      << (proc.isStandalone() ? "Standalone - with 'Audio Input', choose the device under Options > Audio/MIDI Settings."
                              : "Plug-in - listens to the track it is inserted on; audio passes through unchanged.");
    {
        const auto& t = proc.engineState.telemetry;
        if (t.outWindowW.load() > 0)
            s << "\nLive output: window " << t.outWindowW.load() << "x" << t.outWindowH.load()
              << "  |  GL surface " << t.outSurfaceW.load() << "x" << t.outSurfaceH.load()
              << "  |  render " << t.outRenderW.load() << "x" << t.outRenderH.load()
              << ((t.outSurfaceW.load() < t.outWindowW.load() - 2 || t.outSurfaceH.load() < t.outWindowH.load() - 2)
                      ? "   <- surface smaller than the window" : "   (full screen)") << "\n";
    }
    info.setText(s, juce::dontSendNotification);
}

void SettingsPanel::paint(juce::Graphics& g) { g.fillAll(colours::panel); }

void SettingsPanel::resized()
{
    auto r = getLocalBounds().reduced(22, 16);
    auto row = [&](int h) { auto x = r.removeFromTop(h); return x; };

    outHeader.setBounds(row(24));
    auto a = row(30); displayLabel.setBounds(a.removeFromLeft(150)); identify.setBounds(a.removeFromRight(140).reduced(0, 2));
    a.removeFromRight(8); display.setBounds(a.reduced(0, 2));
    a = row(30); resolutionLabel.setBounds(a.removeFromLeft(150)); resolution.setBounds(a.reduced(0, 2));
    vsync.setBounds(row(26));
    previewWhileOutput.setBounds(row(26));
    openOutput.setBounds(row(34).withWidth(200).reduced(0, 3));
    r.removeFromTop(10);

    audioHeader.setBounds(row(24));
    a = row(30); sourceLabel.setBounds(a.removeFromLeft(150)); source.setBounds(a.reduced(0, 2));
    sourceStatus.setBounds(row(34).withTrimmedLeft(150));
    r.removeFromTop(8);

    midiHeader.setBounds(row(24));
    noteScenes.setBounds(row(26));
    programScenes.setBounds(row(26));
    midiLast.setBounds(row(22));
    a = row(32);
    clearMidi.setBounds(a.removeFromLeft(230).reduced(0, 2));
    a.removeFromLeft(10);
    resetAll.setBounds(a.removeFromLeft(170).reduced(0, 2));
    if (restoreSession.isVisible()) restoreSession.setBounds(row(26));
    r.removeFromTop(10);

    infoHeader.setBounds(row(24));
    info.setBounds(r);
}

SettingsWindow::SettingsWindow(DaliVisualProcessor& p, juce::LookAndFeel& lnf, juce::Component* centreOn)
    : juce::DocumentWindow("Dali Visual - Settings", colours::panel, juce::DocumentWindow::closeButton, true)
{
    setLookAndFeel(&lnf);
    setUsingNativeTitleBar(true);
    setContentOwned(new SettingsPanel(p), true);
    setResizable(false, false);
    show(centreOn);
}

void SettingsWindow::show(juce::Component* centreOn)
{
    if (auto* panel = dynamic_cast<SettingsPanel*>(getContentComponent())) panel->refresh();
    if (centreOn != nullptr) centreAroundComponent(centreOn, getWidth(), getHeight());
    setAlwaysOnTop(true);
    setVisible(true);
    toFront(true);
}
} // namespace dali
