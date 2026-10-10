#include "Chrome.h"
#include "../Core/AppPrefs.h"
#include "../Output/OutputManager.h"
#include "../Output/VideoRecorder.h"
#include "../Render/Library.h"

namespace dali
{
// =============================================================================
//  license dialog
// =============================================================================
void showLicenseDialog(DaliVisualProcessor& proc)
{
    if (License::isFull())
    {
        juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                                         .withIconType(juce::MessageBoxIconType::InfoIcon)
                                         .withTitle("Dali Visual - Full version")
                                         .withMessage("Activated with serial " + License::maskedSerial() + ".\n\nThank you for supporting DALI AUDIO!")
                                         .withButton("OK")
                                         .withButton("Remove License..."),
                                     [&proc](int result)
                                     {
                                         if (result != 0) return;                      // last button = 0
                                         juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                                                                          .withIconType(juce::MessageBoxIconType::QuestionIcon)
                                                                          .withTitle("Remove license")
                                                                          .withMessage("Remove the serial from this computer? Dali Visual goes back to the demo.")
                                                                          .withButton("Remove")
                                                                          .withButton("Cancel"),
                                                                      [&proc](int r)
                                                                      {
                                                                          if (r != 1) return;
                                                                          License::deactivate();
                                                                          proc.licenseChanged.sendChangeMessage();
                                                                      });
                                     });
        return;
    }

    // DEMO / TRIAL: serial entry, the 7-day trial (once per computer), buy, or carry on
    juce::String message;
    const auto mode = License::mode();
    if (mode == License::Mode::Trial)
        message << "Free trial: " << License::trialDaysLeft() << (License::trialDaysLeft() == 1 ? " day left." : " days left.")
                << " Everything is unlocked; the DALI AUDIO watermark appears on the picture.\n\n";
    else if (License::canStartTrial())
        message << "Try the full version free for " << License::trialDays << " days (the DALI AUDIO watermark stays on the picture),"
                << " or enter your serial number.\n\n";
    else
        message << "Your free trial has ended. Enter your serial number to unlock the full version.\n\n";
    message << License::demoSummary();

    auto* w = new juce::AlertWindow("Dali Visual", message, juce::MessageBoxIconType::NoIcon);
    w->addTextEditor("serial", {}, "Serial (DALI-XXXX-XXXX-XXXX)");
    w->addButton("Activate", 1, juce::KeyPress(juce::KeyPress::returnKey));
    if (mode == License::Mode::Demo && License::canStartTrial())
        w->addButton("Start " + juce::String(License::trialDays) + "-Day Free Trial", 3);
    w->addButton("Buy...", 2);
    w->addButton(mode == License::Mode::Trial ? "Continue Trial" : "Continue Demo", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    w->setAlwaysOnTop(true);
    w->enterModalState(true, juce::ModalCallbackFunction::create([&proc, w](int result)
    {
        if (result == 2) { juce::URL(License::buyUrl).launchInDefaultBrowser(); return; }
        if (result == 3)
        {
            const bool ok = License::startTrial();
            proc.licenseChanged.sendChangeMessage();
            juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                                             .withIconType(ok ? juce::MessageBoxIconType::InfoIcon : juce::MessageBoxIconType::WarningIcon)
                                             .withTitle(ok ? "Trial started" : "Trial not available")
                                             .withMessage(ok ? "Everything is unlocked for " + juce::String(License::trialDays)
                                                                   + " days: all scenes, GO LIVE, REC and your own images.\n"
                                                                   "The DALI AUDIO watermark appears on the picture during the trial."
                                                             : juce::String("The free trial was already used on this computer."))
                                             .withButton("OK"),
                                         [](int) {});
            return;
        }
        if (result != 1) return;
        const auto serial = w->getTextEditorContents("serial");
        if (License::activate(serial))
        {
            proc.licenseChanged.sendChangeMessage();
            juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                                             .withIconType(juce::MessageBoxIconType::InfoIcon)
                                             .withTitle("Activated")
                                             .withMessage("Dali Visual is now the full version - every scene, GO LIVE, unlimited REC, your own images and no watermark.")
                                             .withButton("OK"),
                                         [](int) {});
        }
        else
        {
            juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                                             .withIconType(juce::MessageBoxIconType::WarningIcon)
                                             .withTitle("Invalid serial")
                                             .withMessage("This serial number is not valid. Check it and try again\n(letters and digits only; the dashes are optional).")
                                             .withButton("Try Again")
                                             .withButton("Cancel"),
                                         [&proc](int r) { if (r == 1) showLicenseDialog(proc); });
        }
    }), true);
}

void showLaunchLicenseDialog(DaliVisualProcessor& proc)
{
    static bool shown = false;                         // once per process (a DAW may open many editors)
    if (shown || License::isFull()) return;
    shown = true;
    showLicenseDialog(proc);
}

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
                                                                        &undoBtn, &redoBtn, &chaos, &rec })
        addAndMakeVisible(c);

    addChildComponent(demoBadge);
    demoBadge.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe0304a));
    demoBadge.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    demoBadge.setTooltip("Demo version - click to enter your serial number");
    demoBadge.onClick = [this] { showLicenseDialog(proc); };
    proc.lockedFeatureHit.addChangeListener(this);
    proc.licenseChanged.addChangeListener(this);

    rec.setClickingTogglesState(false);
    rec.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe0304a));
    rec.setTooltip("Record the visuals together with the audio they react to into an .mp4 video "
                   "(format, resolution and folder: SETTINGS). Click again to stop.");
    rec.onClick = [this] { toggleRecording(); };
    rec.setEnabled(VideoRecorder::isSupported());
    proc.recorder.finished.addChangeListener(this);

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
    updateRecButton();
    updateLicenseState();
    startTimerHz(4);
}

HeaderBar::~HeaderBar()
{
    proc.recorder.finished.removeChangeListener(this);
    proc.lockedFeatureHit.removeChangeListener(this);
    proc.licenseChanged.removeChangeListener(this);
    proc.historyChanged.removeChangeListener(this);
    proc.output.removeChangeListener(this);
}

void HeaderBar::changeListenerCallback(juce::ChangeBroadcaster* broadcaster)
{
    if (broadcaster == &proc.lockedFeatureHit) { showLockedNotice(); return; }
    if (broadcaster == &proc.licenseChanged)   { updateLicenseState(); return; }
    if (broadcaster != &proc.recorder.finished) { updateLiveButton(); return; }

    updateRecButton();
    const auto r = proc.recorder.getLastResult();
    const auto file = r.file;
    if (r.ok)
    {
        const int secs = juce::roundToInt(r.seconds);
        juce::String msg;
        msg << "Saved " << juce::String(secs / 60) << ":" << juce::String(secs % 60).paddedLeft('0', 2)
            << "  (" << r.width << " x " << r.height << ")\n\n" << file.getFullPathName();
        if (r.error.isNotEmpty()) msg << "\n\n" << r.error;
        if (r.limitReached)
            msg << "\n\nDemo recordings stop after " << juce::roundToInt(r.seconds) << " seconds and carry the DALI AUDIO logo. "
                << "The full version records without a limit and without the watermark.";
        juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                                         .withIconType(juce::MessageBoxIconType::InfoIcon)
                                         .withTitle("Recording saved")
                                         .withMessage(msg)
                                         .withButton("Show in Folder")
                                         .withButton("OK"),
                                     [file](int result) { if (result == 1) file.revealToUser(); });
    }
    else
    {
        juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                                         .withIconType(juce::MessageBoxIconType::WarningIcon)
                                         .withTitle("Recording failed")
                                         .withMessage(r.error.isNotEmpty() ? r.error : juce::String("The recording could not be saved."))
                                         .withButton("OK"),
                                     [](int) {});
    }
}

void HeaderBar::showLockedNotice()
{
    const auto f = proc.getLastLockedFeature();
    juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                                     .withIconType(juce::MessageBoxIconType::InfoIcon)
                                     .withTitle("Full version")
                                     .withMessage(License::featureName(f) + " is available in the full version of Dali Visual"
                                                  + (License::canStartTrial() ? juce::String(" - or free for " + juce::String(License::trialDays) + " days with the trial.")
                                                                              : juce::String("."))
                                                  + "\n\n" + License::demoSummary())
                                     .withButton(License::canStartTrial() ? "Free Trial / Serial..." : "Enter Serial...")
                                     .withButton("Buy...")
                                     .withButton("OK"),
                                 [this](int result)
                                 {
                                     if (result == 1) showLicenseDialog(proc);
                                     if (result == 2) juce::URL(License::buyUrl).launchInDefaultBrowser();
                                 });
}

void HeaderBar::updateLicenseState()
{
    const auto mode = License::mode();
    const int key = int(mode) * 100 + License::trialDaysLeft();
    if (key == licenseShown) return;
    licenseShown = key;
    demoBadge.setVisible(mode != License::Mode::Full);
    if (mode == License::Mode::Trial)
    {
        demoBadge.setButtonText("TRIAL " + juce::String(License::trialDaysLeft()) + "d");
        demoBadge.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe08a1e));
        demoBadge.setTooltip("Free trial - " + License::statusText() + ". Click to enter your serial number.");
    }
    else
    {
        demoBadge.setButtonText("DEMO");
        demoBadge.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe0304a));
        demoBadge.setTooltip("Demo version - click for the free trial or your serial number");
    }
    // scene chooser: locked scenes are greyed out in the demo
    for (int i = 0; i < scene.getNumItems(); ++i)
    {
        const int id = scene.getItemId(i);
        const bool allowed = License::isSceneAllowed(id - 1);
        scene.setItemEnabled(id, allowed);
        auto text = sceneLibrary()[size_t(juce::jlimit(0, int(sceneLibrary().size()) - 1, id - 1))].name;
        scene.changeItemText(id, allowed ? juce::String(text) : juce::String(text) + "   (full version)");
    }
    if (auto* top = getTopLevelComponent()) top->repaint();       // scene tiles show / drop their locks
}

void HeaderBar::toggleRecording()
{
    if (proc.recorder.isRecording()) { proc.recorder.stop(); updateRecButton(); return; }
    if (!proc.requireFeature(Feature::Recording)) return;
    juce::String error;
    auto recSettings = VideoRecorder::loadSettings();
    recSettings.maxSeconds = License::recordingLimitSeconds();            // demo: 30 s
    if (!proc.recorder.start(recSettings, error))
        juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                                         .withIconType(juce::MessageBoxIconType::WarningIcon)
                                         .withTitle("Cannot record")
                                         .withMessage(error)
                                         .withButton("OK"),
                                     [](int) {});
    updateRecButton();
}

void HeaderBar::updateRecButton()
{
    auto& r = proc.recorder;
    if (r.isRecording())
    {
        const int secs = int(r.elapsedSeconds());
        rec.setToggleState(true, juce::dontSendNotification);
        const int limit = int(r.getMaxSeconds());
        rec.setButtonText(juce::String::fromUTF8("\xe2\x96\xa0 ") + juce::String(secs / 60).paddedLeft('0', 2) + ":"
                          + juce::String(secs % 60).paddedLeft('0', 2)
                          + (limit > 0 ? " / " + juce::String(limit) + "s" : juce::String()));
        rec.setEnabled(true);
    }
    else if (r.isFinishing())
    {
        rec.setToggleState(false, juce::dontSendNotification);
        rec.setButtonText("SAVING...");
        rec.setEnabled(false);
    }
    else
    {
        rec.setToggleState(false, juce::dontSendNotification);
        rec.setButtonText(juce::String::fromUTF8("\xe2\x97\x8f REC"));
        rec.setEnabled(VideoRecorder::isSupported());
    }
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
    x -= 90;  place(rec, x, 84);
    x -= 40;  place(identify, x, 36, "");
    const bool narrow = getWidth() < 1500;
    const int displayW = narrow ? 156 : 196;
    x -= displayW + 4; place(display, x, displayW, "OUTPUT DISPLAY");
    const int sourceW = narrow ? 150 : 188;
    x -= 18 + sourceW; place(source, x, sourceW, "AUDIO SOURCE");
    const int rightStart = x - 18;

    // left side (flexible): the scene chooser, then CHAOS and undo / redo
    const int lx = r.getX();
    const int sceneW = juce::jlimit(180, 300, rightStart - lx - 8 - 92 - 64);
    place(scene, lx, sceneW, "SCENE");
    place(chaos, lx + sceneW + 8, 84);
    place(undoBtn, lx + sceneW + 8 + 90, 30);
    place(redoBtn, lx + sceneW + 8 + 90 + 32, 30);
    demoBadge.setBounds(122, 32, 56, 16);
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
             &noteScenes, &programScenes, &identify, &openOutput, &clearMidi, &resetAll, &restoreSession,
             &recHeader, &recFormat, &recQuality, &recFps, &recLabel, &recFolderLabel, &recFolder, &recChoose, &recOpen, &licenseBtn })
        addAndMakeVisible(c);

    recLabel.setText("Format", juce::dontSendNotification);
    recFolderLabel.setText("Save to", juce::dontSendNotification);
    recFormat.addItem("As on screen", 1);
    recFormat.addItem("16:9  landscape (YouTube)", 2);
    recFormat.addItem("9:16  vertical (Reels / Stories / TikTok)", 3);
    recFormat.addItem("1:1  square", 4);
    recQuality.addItem("720p", 1);
    recQuality.addItem("1080p  (Full HD)", 2);
    recQuality.addItem("4K  (heavy)", 3);
    recFps.addItem("30 fps", 30);
    recFps.addItem("60 fps", 60);
    recFormat.setTooltip("The picture of the video. A fixed format is shown in the preview while recording "
                         "(what you see is what is recorded); the live output keeps its screen and is cropped to it.");
    recQuality.setTooltip("Recording resolution (the short side). Rendered at this size, independent of the window.");
    for (auto* c : { &recFormat, &recQuality, &recFps })
        c->onChange = [this] { saveRecordingSettings(); };
    recChoose.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser>("Folder for recordings", VideoRecorder::folderFor(VideoRecorder::loadSettings()));
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                             [this](const juce::FileChooser& fc)
                             {
                                 const auto dir = fc.getResult();
                                 if (dir == juce::File()) return;
                                 auto st = VideoRecorder::loadSettings();
                                 st.folder = dir;
                                 VideoRecorder::saveSettings(st);
                                 refresh();
                             });
    };
    recOpen.onClick = []
    {
        const auto dir = VideoRecorder::folderFor(VideoRecorder::loadSettings());
        dir.createDirectory();
        dir.startAsProcess();
    };

    displayLabel.setText("Output display", juce::dontSendNotification);
    resolutionLabel.setText("Render resolution", juce::dontSendNotification);
    sourceLabel.setText("Listen to", juce::dontSendNotification);
    for (auto* l : { &displayLabel, &resolutionLabel, &sourceLabel, &sourceStatus, &midiLast, &info, &recLabel, &recFolderLabel, &recFolder })
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
    licenseBtn.onClick = [this] { showLicenseDialog(proc); };
    licenseBtn.setTooltip("Activate, show or remove your serial number");
    restoreSession.setTooltip("Off: every launch starts fresh (display, render, MIDI mappings and audio source are always kept). "
                              "On: the last session's scene, modulation, effects and image come back too.");
    restoreSession.setToggleState(AppPrefs::restoreSession(), juce::dontSendNotification);
    restoreSession.onClick = [this] { AppPrefs::setRestoreSession(restoreSession.getToggleState()); };
    restoreSession.setVisible(proc.isStandalone());
    setSize(580, 740);
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

    const auto rs = VideoRecorder::loadSettings();
    recFormat.setSelectedId(rs.format + 1, juce::dontSendNotification);
    recQuality.setSelectedId(rs.quality + 1, juce::dontSendNotification);
    recFps.setSelectedId(rs.fps, juce::dontSendNotification);
    recFolder.setText(VideoRecorder::folderFor(rs).getFullPathName(), juce::dontSendNotification);
    recFolder.setTooltip(recFolder.getText());
}

void SettingsPanel::saveRecordingSettings()
{
    auto st = VideoRecorder::loadSettings();
    st.format  = juce::jmax(0, recFormat.getSelectedId() - 1);
    st.quality = juce::jmax(0, recQuality.getSelectedId() - 1);
    st.fps     = recFps.getSelectedId() == 30 ? 30 : 60;
    VideoRecorder::saveSettings(st);
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
    s << "License: " << License::statusText() << "\n";
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
    a.removeFromLeft(10);
    licenseBtn.setBounds(a.reduced(0, 2));
    if (restoreSession.isVisible()) restoreSession.setBounds(row(26));
    r.removeFromTop(10);

    recHeader.setBounds(row(24));
    a = row(30); recLabel.setBounds(a.removeFromLeft(150));
    recFps.setBounds(a.removeFromRight(80).reduced(0, 2)); a.removeFromRight(6);
    recQuality.setBounds(a.removeFromRight(120).reduced(0, 2)); a.removeFromRight(6);
    recFormat.setBounds(a.reduced(0, 2));
    a = row(30); recFolderLabel.setBounds(a.removeFromLeft(150));
    recOpen.setBounds(a.removeFromRight(64).reduced(0, 2)); a.removeFromRight(6);
    recChoose.setBounds(a.removeFromRight(84).reduced(0, 2)); a.removeFromRight(6);
    recFolder.setBounds(a);
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
