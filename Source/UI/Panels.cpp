#include "Panels.h"
#include "../Render/Library.h"
#include "../Render/Palettes.h"
#include "BinaryData.h"

namespace dali
{
namespace
{
constexpr int kPad = 10, kKnobH = 64, kHeaderH = 22;

juce::Label& styleSmall(juce::Label& l, bool dim = true)
{
    l.setFont(juce::Font(juce::FontOptions(11.5f)));
    l.setColour(juce::Label::textColourId, dim ? colours::textDim : colours::text);
    return l;
}
}

// =============================================================================
//  SCENE
// =============================================================================
class ScenePanel::SceneTile : public juce::Button
{
public:
    SceneTile(int idx) : juce::Button(sceneLibrary()[size_t(idx)].name), index(idx)
    {
        int size = 0;
        const juce::String res = "thumb_" + juce::String(idx + 1).paddedLeft('0', 2) + "_jpg";
        if (const char* data = BinaryData::getNamedResource(res.toRawUTF8(), size))
            thumb = juce::ImageFileFormat::loadFrom(data, size_t(size));
        setTooltip(juce::String(sceneLibrary()[size_t(idx)].description)
                   + (idx < 10 ? "\n(key " + juce::String((idx + 1) % 10) + ")" : juce::String()));
    }
    void paintButton(juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced(2.0f);
        juce::Path clip; clip.addRoundedRectangle(r, 7.0f);
        g.saveState();
        g.reduceClipRegion(clip);
        if (thumb.isValid()) g.drawImage(thumb, r, juce::RectanglePlacement::fillDestination);
        else { g.setColour(colours::panel2); g.fillRect(r); }
        g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack, 0.0f, r.getBottom() - 34.0f,
                                               juce::Colours::black.withAlpha(0.85f), 0.0f, r.getBottom(), false));
        g.fillRect(r);
        if (!getToggleState()) { g.setColour(juce::Colours::black.withAlpha(over ? 0.12f : 0.32f)); g.fillRect(r); }
        g.restoreState();

        const bool on = getToggleState();
        g.setColour(on ? colours::accent : (over ? colours::outline.brighter(0.4f) : colours::outline));
        g.drawRoundedRectangle(r, 7.0f, on ? 2.5f : 1.0f);
        g.setColour(on ? juce::Colours::white : colours::text.withAlpha(0.85f));
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawFittedText(getName().fromFirstOccurrenceOf("  ", false, false).trim(),
                         r.removeFromBottom(22.0f).reduced(8.0f, 0.0f).toNearestInt(), juce::Justification::centredLeft, 1);
        g.setColour(on ? colours::accent : colours::textDim);
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(juce::String(index + 1), r.reduced(8.0f, 6.0f).toNearestInt(), juce::Justification::topLeft);
        if (!License::isSceneAllowed(index))
        {
            // DEMO: locked scene
            g.setColour(juce::Colours::black.withAlpha(0.45f));
            g.fillRoundedRectangle(r.reduced(1.0f), 7.0f);
            auto pill = juce::Rectangle<float>(r.getRight() - 46.0f, r.getY() + 6.0f, 40.0f, 15.0f);
            g.setColour(juce::Colour(0xffe0304a));
            g.fillRoundedRectangle(pill, 7.5f);
            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
            g.drawText("FULL", pill, juce::Justification::centred);
        }
        juce::ignoreUnused(down);
    }
private:
    const int index;
    juce::Image thumb;
};

ScenePanel::ScenePanel(DaliVisualProcessor& p)
    : proc(p),
      macroA(p, params::id::macroA), macroB(p, params::id::macroB), macroC(p, params::id::macroC), macroD(p, params::id::macroD),
      intensity(p, params::id::intensity, "Intensity"), speed(p, params::id::speed, "Motion"),
      drive(p, params::id::audioDrive, "Audio Drive"), idle(p, params::id::idleMotion, "Idle Motion"),
      dynamics(p, params::id::dynamics, "Build / Drop"),
      autoFx(p, params::id::autoFX, "Auto FX"),
      trip(p, params::id::tripAmount, "Trip"), smoothMotion(p, params::id::motionSmooth, "Smooth Motion"),
      kickResp(p, params::id::reactTransient, "Kick Impact"), bassResp(p, params::id::reactBass, "Bass Motion"),
      midResp(p, params::id::reactMid, "Mid Morph"), highResp(p, params::id::reactHigh, "High Detail"),
      transition(p, params::id::transitionTime, "Transition"),
      autoMode(p, params::id::autoPilot), autoBars(p, params::id::autoBars), autoOnDrop(p, params::id::autoOnDrop, "Change on drop"),
      sceneInitToggle(p, params::id::sceneInit, "Load scene init")
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &sceneHeader, &macroHeader, &motionHeader, &autoHeader,
             &description, &autoHint, &macroA, &macroB, &macroC, &macroD, &intensity, &speed, &drive, &idle, &dynamics, &autoFx, &responseHeader, &sceneInitToggle, &resetScene, &trip, &smoothMotion, &kickResp, &bassResp, &midResp, &highResp, &transition,
             &autoMode, &autoBars, &autoOnDrop })
        addAndMakeVisible(c);

    for (int i = 0; i < int(sceneLibrary().size()); ++i)
    {
        auto* t = tiles.add(new SceneTile(i));
        t->onClick = [this, i]
        {
            if (auto* prm = proc.apvts.getParameter(params::id::scene))
            {
                prm->beginChangeGesture();
                prm->setValueNotifyingHost(prm->convertTo0to1(float(i)));
                prm->endChangeGesture();
            }
        };
        addAndMakeVisible(t);
    }
    styleSmall(description);
    description.setJustificationType(juce::Justification::topLeft);
    styleSmall(autoHint).setText("Varies the scene with the music's phrases; 'Scenes' also switches scenes.",
                                 juce::dontSendNotification);
    drive.setTooltip("How much the music's energy drives the speed of motion (0 = constant speed)");
    idle.setTooltip("Motion without audio. 0 = the picture rests when nothing plays");
    dynamics.setTooltip("Build-ups drain colour and close in; drops hit with a burst of light");
    autoFx.setTooltip("Automatic camera and lens effects from the music: kick punch, chromatic split, speed blur, colour drift");
    sceneInitToggle.setTooltip("When a scene is chosen, load the values that show it at its best (undoable with Ctrl+Z)");
    resetScene.setTooltip("Bring this scene back to its own init values (undoable)");
    resetScene.onClick = [this] { proc.applySceneInit(); };
    trip.setTooltip("Psychedelic layer: symmetry, trails, warp and colour that grow with the music (calm = clean, peak/chaos = deep)");
    smoothMotion.setTooltip("How smoothly the camera and structures follow the music (high = fluid, low = every beat pushes)");
    kickResp.setTooltip("How hard the kick hits: camera punch, shockwaves, flashes");
    bassResp.setTooltip("How much the bass drives the big, slow motion (flight speed, breathing)");
    midResp.setTooltip("How much the mids morph shapes and patterns");
    highResp.setTooltip("How much the highs add sparkle and fine detail");
    transition.setTooltip("Length of the portal transition between scenes (also used by Auto Pilot)");
    proc.apvts.addParameterListener(params::id::scene, this);
    handleAsyncUpdate();
}

ScenePanel::~ScenePanel() { proc.apvts.removeParameterListener(params::id::scene, this); }

void ScenePanel::handleAsyncUpdate()
{
    const int s = juce::roundToInt(proc.apvts.getRawParameterValue(params::id::scene)->load());
    for (int i = 0; i < tiles.size(); ++i) tiles[i]->setToggleState(i == s, juce::dontSendNotification);
    const auto& info = sceneLibrary()[size_t(juce::jlimit(0, int(sceneLibrary().size()) - 1, s))];
    macroA.setLabel(info.macro[0]); macroB.setLabel(info.macro[1]);
    macroC.setLabel(info.macro[2]); macroD.setLabel(info.macro[3]);
    description.setText(info.description, juce::dontSendNotification);
    repaint();
}

int ScenePanel::preferredHeight(int width)
{
    const int tileH = juce::roundToInt((width - 2 * kPad) / 2 * 9.0 / 16.0);
    const int rows = (int(sceneLibrary().size()) + 1) / 2;
    return kPad + kHeaderH + rows * tileH + 40 + kPad + kHeaderH + kKnobH + kPad + kHeaderH + kKnobH * 2
           + kPad + kHeaderH + 30 + kKnobH * 2 + kKnobH
           + kPad + kHeaderH + 30 + 26 + 34 + kPad;
}

void ScenePanel::resized()
{
    auto r = getLocalBounds().reduced(kPad);
    sceneHeader.setBounds(r.removeFromTop(kHeaderH));
    const int tw = r.getWidth() / 2, th = juce::roundToInt(tw * 9.0 / 16.0);
    auto grid = r.removeFromTop(th * ((tiles.size() + 1) / 2));
    for (int i = 0; i < tiles.size(); ++i)
        tiles[i]->setBounds(grid.getX() + (i % 2) * tw, grid.getY() + (i / 2) * th, tw, th);
    description.setBounds(r.removeFromTop(40));
    r.removeFromTop(kPad);
    macroHeader.setBounds(r.removeFromTop(kHeaderH));
    layoutKnobGrid({ &macroA, &macroB, &macroC, &macroD }, r.removeFromTop(kKnobH), 4, kKnobH);
    r.removeFromTop(kPad);
    motionHeader.setBounds(r.removeFromTop(kHeaderH));
    layoutKnobGrid({ &intensity, &speed, &drive, &idle, &dynamics, &autoFx }, r.removeFromTop(kKnobH * 2), 3, kKnobH);
    r.removeFromTop(kPad);
    responseHeader.setBounds(r.removeFromTop(kHeaderH));
    {
        auto row = r.removeFromTop(30);
        resetScene.setBounds(row.removeFromRight(110).reduced(2));
        sceneInitToggle.setBounds(row);
    }
    layoutKnobGrid({ &trip, &smoothMotion, &kickResp, &bassResp, &midResp, &highResp }, r.removeFromTop(kKnobH * 2), 3, kKnobH);
    r.removeFromTop(kPad);
    autoHeader.setBounds(r.removeFromTop(kHeaderH));
    auto row = r.removeFromTop(30);
    autoMode.setBounds(row.removeFromLeft(row.getWidth() * 55 / 100).reduced(2));
    autoBars.setBounds(row.reduced(2));
    layoutKnobGrid({ &transition }, r.removeFromTop(kKnobH), 4, kKnobH);
    autoOnDrop.setBounds(r.removeFromTop(26));
    autoHint.setBounds(r.removeFromTop(34));
}

// =============================================================================
//  AUDIO
// =============================================================================
AudioPanel::AudioPanel(DaliVisualProcessor& p)
    : proc(p), source(p),
      sensitivity(p, params::id::sensitivity, "Sensitivity"), smoothing(p, params::id::smoothing, "Smoothing"),
      bass(p, params::id::reactBass, "Bass Motion"), mid(p, params::id::reactMid, "Mid"), high(p, params::id::reactHigh, "High"),
      transient(p, params::id::reactTransient, "Kick Impact"), internalBpm(p, params::id::internalBpm, "Internal BPM"),
      syncSource(p, params::id::syncSource), syncDiv(p, params::id::syncDiv)
{
    addAndMakeVisible(sourceHeader);
    addAndMakeVisible(source);
    addAndMakeVisible(sourceStatus);
    styleSmall(sourceStatus);
    sourceStatus.setJustificationType(juce::Justification::topLeft);
    for (juce::Component* c : std::initializer_list<juce::Component*> { &inputHeader, &reactHeader, &syncHeader, &sensitivity, &smoothing, &bass, &mid,
                     &high, &transient, &internalBpm, &syncSource, &syncDiv, &syncSourceLabel, &syncDivLabel, &readout })
        addAndMakeVisible(c);
    styleSmall(syncSourceLabel).setText("Clock source", juce::dontSendNotification);
    styleSmall(syncDivLabel).setText("Sync division (LFO sources)", juce::dontSendNotification);
    styleSmall(readout, false);
    readout.setJustificationType(juce::Justification::topLeft);
    readout.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain)));
    startTimerHz(10);
}

void AudioPanel::timerCallback()
{
    const auto f = proc.analyzer.snapshot().features;
    static const char* src[] = { "HOST", "DETECTED", "INTERNAL" };
    static const char* stateNames[] = { "CALM", "BUILD", "PEAK", "CHAOS", "RELEASE" };
    const auto& t = proc.engineState.telemetry;
    juce::String s;
    s << "Clock      " << juce::String(t.bpm.load(), 1) << " BPM  (" << src[juce::jlimit(0, 2, t.clockSource.load())] << ")\n"
      << "Detected   " << (f.bpm > 0 ? juce::String(f.bpm, 1) + " BPM" : juce::String("-"))
      << "   conf " << juce::String(juce::roundToInt(f.bpmConfidence * 100.0f)) << "%\n"
      << "Centroid   " << juce::String(f.centroid, 2) << "    Flux " << juce::String(f.flux, 2) << "\n"
      << "Hits       kick " << juce::String(f.kickCount) << "  snare " << juce::String(f.snareCount)
      << "  hat " << juce::String(f.hatCount) << "\n"
      << "Structure  build " << juce::String(juce::roundToInt(f.build * 100.0f)) << "%  drops " << juce::String(f.dropCount) << "\n"
      << "State      " << juce::String(stateNames[juce::jlimit(0, 4, f.state)]) << "  " << juce::String(f.stateTime, 0) << " s\n"
      << "Bands      sub " << juce::String(f.sub, 2) << "  lowmid " << juce::String(f.lowMid, 2) << "  highmid " << juce::String(f.highMid, 2) << "\n"
      << "Density    kick " << juce::String(f.kickDensity, 2) << "  onset " << juce::String(f.onsetDensity, 2)
      << "  dyn " << juce::String(f.dynamicRange, 2) << "\n"
      << "Width      " << juce::String(f.width, 2) << "    Pan  " << juce::String(f.pan, 2)
      << (t.activity.load() < 0.05f ? "\nInput      no signal" : "");
    sourceStatus.setText(proc.getInputSourceStatus(), juce::dontSendNotification);
    readout.setText(s, juce::dontSendNotification);
}

int AudioPanel::preferredHeight(int) { return kPad * 6 + kHeaderH * 4 + 30 + 34 + kKnobH * 3 + 50 + 50 + 176; }

void AudioPanel::resized()
{
    auto r = getLocalBounds().reduced(kPad);
    sourceHeader.setBounds(r.removeFromTop(kHeaderH));
    source.setBounds(r.removeFromTop(30).reduced(0, 2));
    sourceStatus.setBounds(r.removeFromTop(34));
    r.removeFromTop(kPad);
    inputHeader.setBounds(r.removeFromTop(kHeaderH));
    layoutKnobGrid({ &sensitivity, &smoothing }, r.removeFromTop(kKnobH), 3, kKnobH);
    readout.setBounds(r.removeFromTop(176));
    r.removeFromTop(kPad);
    reactHeader.setBounds(r.removeFromTop(kHeaderH));
    layoutKnobGrid({ &bass, &mid, &high, &transient }, r.removeFromTop(kKnobH), 4, kKnobH);
    r.removeFromTop(kPad);
    syncHeader.setBounds(r.removeFromTop(kHeaderH));
    auto row = r.removeFromTop(50);
    syncSourceLabel.setBounds(row.removeFromTop(18));
    syncSource.setBounds(row.removeFromTop(26).withWidth(r.getWidth()));
    row = r.removeFromTop(50);
    syncDivLabel.setBounds(row.removeFromTop(18));
    syncDiv.setBounds(row.removeFromTop(26).withWidth(r.getWidth()));
    layoutKnobGrid({ &internalBpm }, r.removeFromTop(kKnobH), 3, kKnobH);
}

// =============================================================================
//  MOD — one row per slot
// =============================================================================
class ModPanel::Row : public juce::Component
{
public:
    Row(DaliVisualProcessor& p, int slotIndex) : proc(p), index(slotIndex)
    {
        number.setText(juce::String(slotIndex + 1).paddedLeft('0', 2), juce::dontSendNotification);
        styleSmall(number);
        addAndMakeVisible(number);
        enabled.setTooltip("Enable slot");
        addAndMakeVisible(enabled);

        const auto names = ModulationSource::allNames();
        for (int i = 0; i < names.size(); ++i) source.addItem(names[i], i + 1);
        target.addItem("- target -", 1);
        juce::String lastGroup;
        for (int t = 0; t < ModulationTarget::count(); ++t)
        {
            const auto& def = params::all()[size_t(ModulationTarget { t }.paramIndex())];
            if (def.group != lastGroup) { target.addSectionHeading(def.group); lastGroup = def.group; }
            target.addItem(def.name, t + 2);
        }
        addAndMakeVisible(source);
        addAndMakeVisible(target);

        amount.setSliderStyle(juce::Slider::LinearHorizontal);
        amount.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        amount.setRange(-1.0, 1.0, 0.001);
        amount.setDoubleClickReturnValue(true, 0.0);
        amount.setPopupDisplayEnabled(true, true, nullptr);
        amount.setTooltip("Amount");
        addAndMakeVisible(amount);

        auto setupKnob = [this](juce::Slider& s, juce::Label& l, const char* name, double lo, double hi, double def, double skewMid)
        {
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            s.setRange(lo, hi, 0.001);
            if (skewMid > 0) s.setSkewFactorFromMidPoint(skewMid);
            s.setDoubleClickReturnValue(true, def);
            s.setPopupDisplayEnabled(true, true, nullptr);
            addAndMakeVisible(s);
            l.setText(name, juce::dontSendNotification);
            l.setJustificationType(juce::Justification::centred);
            styleSmall(l).setFont(juce::Font(juce::FontOptions(10.0f)));
            addAndMakeVisible(l);
            s.onValueChange = [this] { push(); };
        };
        setupKnob(minS, minL, "Min", 0, 1, 0, 0);
        setupKnob(maxS, maxL, "Max", 0, 1, 1, 0);
        setupKnob(smoothS, smoothL, "Smooth", 0, 1000, 30, 120);
        setupKnob(attackS, attackL, "Attack", 0, 2000, 5, 100);
        setupKnob(releaseS, releaseL, "Release", 0, 4000, 150, 300);
        setupKnob(curveS, curveL, "Curve", -1, 1, 0, 0);
        setupKnob(sensS, sensL, "Sens", 0, 4, 1, 1);
        minS.setTextValueSuffix(""); smoothS.setTextValueSuffix(" ms"); attackS.setTextValueSuffix(" ms"); releaseS.setTextValueSuffix(" ms");

        for (auto* b : { &bipolar, &invert })
        {
            b->setClickingTogglesState(true);
            addAndMakeVisible(b);
            b->onClick = [this] { push(); };
        }
        bipolar.setTooltip("Polarity: unipolar (0..1) / bipolar (-1..1)");
        invert.setTooltip("Invert the source");

        enabled.onClick = [this] { push(); };
        source.onChange = [this] { push(); };
        target.onChange = [this] { push(); };
        amount.onValueChange = [this] { push(); };
        refresh();
    }

    void refresh()
    {
        const juce::ScopedValueSetter<bool> svs(updating, true);
        const auto s = proc.matrix.getSlot(index);
        enabled.setToggleState(s.enabled, juce::dontSendNotification);
        source.setSelectedId(s.source + 1, juce::dontSendNotification);
        target.setSelectedId(s.target >= 0 ? s.target + 2 : 1, juce::dontSendNotification);
        amount.setValue(s.amount, juce::dontSendNotification);
        minS.setValue(s.min, juce::dontSendNotification);       maxS.setValue(s.max, juce::dontSendNotification);
        smoothS.setValue(s.smoothingMs, juce::dontSendNotification);
        attackS.setValue(s.attackMs, juce::dontSendNotification);
        releaseS.setValue(s.releaseMs, juce::dontSendNotification);
        curveS.setValue(s.curve, juce::dontSendNotification);   sensS.setValue(s.sensitivity, juce::dontSendNotification);
        bipolar.setToggleState(s.bipolar, juce::dontSendNotification);
        invert.setToggleState(s.invert, juce::dontSendNotification);
        const bool active = s.source > 0 && s.target >= 0;
        number.setText((s.fromScene && active ? "S" : "") + juce::String(index + 1).paddedLeft('0', 2), juce::dontSendNotification);
        number.setColour(juce::Label::textColourId, s.fromScene && active ? colours::accent : colours::textDim);
        number.setTooltip(s.fromScene && active ? "Scene route: replaced when the scene changes. Edit it to keep it." : "Your route: kept when the scene changes.");
        for (juce::Component* c : std::initializer_list<juce::Component*> { &amount, &minS, &maxS, &smoothS, &attackS, &releaseS, &curveS, &sensS, &bipolar, &invert })
            c->setAlpha(active ? 1.0f : 0.45f);
    }

    void push()
    {
        if (updating) return;
        ModSlot s;
        s.enabled = enabled.getToggleState();
        s.source = juce::jmax(0, source.getSelectedId() - 1);
        s.target = target.getSelectedId() >= 2 ? target.getSelectedId() - 2 : -1;
        s.amount = float(amount.getValue());
        s.min = float(minS.getValue()); s.max = float(maxS.getValue());
        s.smoothingMs = float(smoothS.getValue()); s.attackMs = float(attackS.getValue()); s.releaseMs = float(releaseS.getValue());
        s.curve = float(curveS.getValue()); s.sensitivity = float(sensS.getValue());
        s.bipolar = bipolar.getToggleState(); s.invert = invert.getToggleState();
        s.fromScene = false;                                 // edited by hand: it is yours now, scene changes keep it
        proc.matrix.setSlot(index, s);
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(colours::panel2.withAlpha(0.6f));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 6.0f);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(6, 4);
        auto top = r.removeFromTop(24);
        number.setBounds(top.removeFromLeft(22));
        enabled.setBounds(top.removeFromLeft(36));
        const int w = (top.getWidth() - 6) / 2;
        source.setBounds(top.removeFromLeft(w - 20));
        top.removeFromLeft(6);
        target.setBounds(top.removeFromLeft(w + 20));
        auto amt = r.removeFromTop(18);
        amount.setBounds(amt.withTrimmedLeft(22));
        auto bottom = r;
        auto toggles = bottom.removeFromRight(40);
        bipolar.setBounds(toggles.removeFromTop(bottom.getHeight() / 2).reduced(1));
        invert.setBounds(toggles.reduced(1));
        juce::Slider* ks[] = { &minS, &maxS, &smoothS, &attackS, &releaseS, &curveS, &sensS };
        juce::Label* ls[] = { &minL, &maxL, &smoothL, &attackL, &releaseL, &curveL, &sensL };
        const int kw = bottom.getWidth() / 7;
        for (int i = 0; i < 7; ++i)
        {
            auto cell = juce::Rectangle<int>(bottom.getX() + i * kw, bottom.getY(), kw, bottom.getHeight());
            ls[i]->setBounds(cell.removeFromBottom(12));
            ks[i]->setBounds(cell);
        }
    }

private:
    DaliVisualProcessor& proc;
    const int index;
    bool updating = false;
    juce::Label number;
    juce::ToggleButton enabled;
    juce::ComboBox source, target;
    juce::Slider amount, minS, maxS, smoothS, attackS, releaseS, curveS, sensS;
    juce::Label minL, maxL, smoothL, attackL, releaseL, curveL, sensL;
    juce::TextButton bipolar { juce::String::fromUTF8("\xc2\xb1") }, invert { "INV" };
};

ModPanel::ModPanel(DaliVisualProcessor& p) : proc(p)
{
    addAndMakeVisible(header);
    addAndMakeVisible(clearAll);
    clearAll.onClick = [this] { proc.pushUndo(); proc.matrix.clear(); };
    for (int i = 0; i < kMaxModSlots; ++i) addAndMakeVisible(rows.add(new Row(p, i)));
    proc.matrix.addChangeListener(this);
}

ModPanel::~ModPanel() { proc.matrix.removeChangeListener(this); }

void ModPanel::changeListenerCallback(juce::ChangeBroadcaster*) { for (auto* r : rows) r->refresh(); }

int ModPanel::preferredHeight(int) { return kPad * 2 + 28 + kMaxModSlots * 96; }

void ModPanel::resized()
{
    auto r = getLocalBounds().reduced(kPad);
    auto top = r.removeFromTop(24);
    clearAll.setBounds(top.removeFromRight(80));
    header.setBounds(top);
    r.removeFromTop(4);
    for (auto* row : rows) { row->setBounds(r.removeFromTop(92)); r.removeFromTop(4); }
}

// =============================================================================
//  FX
// =============================================================================
// what each effect does, in one line, and where it belongs - so the rack explains itself
namespace
{
struct FxGuide { const char* id; const char* category; juce::uint32 colour; const char* text; bool trip; };
const FxGuide fxGuide[] = {
    { "blur",       "TEXTURE",  0xff8a8fa3, "Softens the picture; Radial blurs outward from the centre.", false },
    { "glow",       "LIGHT",    0xffffc857, "Bright parts bleed light; Threshold sets what counts as bright.", false },
    { "feedback",   "FEEDBACK", 0xff59d1b5, "Each frame echoes into the next, zooming and turning: infinite tunnels.", false },
    { "kaleido",    "SYMMETRY", 0xffb388ff, "Mirrors the picture into kaleidoscope segments.", true },
    { "mirror",     "SYMMETRY", 0xffb388ff, "Mirrors left/right or top/bottom (Mode).", false },
    { "twist",      "DISTORT",  0xffff7a59, "Twists the centre like a whirlpool.", false },
    { "warp",       "DISTORT",  0xffff7a59, "Liquid wobble of the whole picture.", true },
    { "noise",      "TEXTURE",  0xff8a8fa3, "Film grain.", false },
    { "chromatic",  "GLITCH",   0xff4fc3f7, "Colours split at the edges like a cheap lens.", true },
    { "rgbsplit",   "GLITCH",   0xff4fc3f7, "Red, green and blue shift apart in one direction.", false },
    { "displace",   "DISTORT",  0xffff7a59, "Bright areas push the picture: embossed, rippled.", false },
    { "pixelate",   "GLITCH",   0xff4fc3f7, "Big pixels or LED dots.", false },
    { "posterize",  "COLOUR",   0xfff06292, "Reduces the number of colour steps: poster look.", false },
    { "invert",     "COLOUR",   0xfff06292, "Negative image.", false },
    { "contrast",   "COLOUR",   0xfff06292, "Harder or softer contrast.", false },
    { "brightness", "COLOUR",   0xfff06292, "Gain and lift.", false },
    { "saturation", "COLOUR",   0xfff06292, "More or less colour.", false },
    { "hueshift",   "COLOUR",   0xfff06292, "Rotates all colours; Rotate keeps them cycling.", true },
    { "vignette",   "LIGHT",    0xffffc857, "Darkens the edges, focuses the centre.", false },
    { "trails",     "FEEDBACK", 0xff59d1b5, "Motion leaves glowing trails behind it.", true },
};
const FxGuide& guideFor(const char* id)
{
    for (auto& g : fxGuide) if (juce::String(g.id) == id) return g;
    return fxGuide[0];
}
}

class FxPanel::Row : public juce::Component
{
public:
    Row(DaliVisualProcessor& p, int fxIndex)
        : proc(p), fx(fxIndex), info(effectLibrary()[size_t(fxIndex)]), guide(guideFor(info.id)),
          on(p, params::id::fxOn(info.id), info.name),
          amount(p, params::id::fxAmt(info.id), info.p1Name),
          p2(p, params::id::fxP2(info.id), info.p2Name)
    {
        for (juce::Component* c : std::initializer_list<juce::Component*> { &on, &amount, &p2, &up, &down, &text }) addAndMakeVisible(c);
        text.setText(guide.text, juce::dontSendNotification);
        text.setFont(juce::Font(juce::FontOptions(11.5f)));
        text.setColour(juce::Label::textColourId, colours::textDim);
        text.setJustificationType(juce::Justification::topLeft);
        text.setInterceptsMouseClicks(false, false);
        up.onClick   = [this] { move(-1); };
        down.onClick = [this] { move(+1); };
        up.setTooltip("Move earlier in the chain");
        down.setTooltip("Move later in the chain");
        on.setTooltip(juce::String(guide.text) + (guide.trip ? "  (Trip in the SCENE tab also drives this one automatically.)" : ""));
    }
    void move(int delta)
    {
        const auto order = proc.effectChain.getOrder();
        for (int i = 0; i < EffectChain::kNumEffects; ++i)
            if (order[size_t(i)] == fx) { proc.effectChain.move(i, delta); break; }
    }
    void paint(juce::Graphics& g) override
    {
        g.setColour(colours::panel2.withAlpha(0.6f));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 6.0f);
        // category pill (and a TRIP badge for the effects the Trip layer drives)
        auto pill = juce::Rectangle<float>(8.0f, getHeight() - 20.0f, 66.0f, 15.0f);
        g.setColour(juce::Colour(guide.colour).withAlpha(0.22f));
        g.fillRoundedRectangle(pill, 7.0f);
        g.setColour(juce::Colour(guide.colour));
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(guide.category, pill, juce::Justification::centred);
        if (guide.trip)
        {
            auto badge = pill.translated(pill.getWidth() + 6.0f, 0.0f).withWidth(40.0f);
            g.setColour(colours::accent.withAlpha(0.25f));
            g.fillRoundedRectangle(badge, 7.0f);
            g.setColour(colours::accent);
            g.drawText("TRIP", badge, juce::Justification::centred);
        }
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced(6, 3);
        auto arrows = r.removeFromRight(22);
        up.setBounds(arrows.removeFromTop(r.getHeight() / 2).reduced(1));
        down.setBounds(arrows.reduced(1));
        p2.setBounds(r.removeFromRight(62));
        amount.setBounds(r.removeFromRight(62));
        on.setBounds(r.removeFromTop(26));
        text.setBounds(r.withTrimmedBottom(18).withTrimmedLeft(2));
    }
private:
    DaliVisualProcessor& proc;
    const int fx;
    const EffectInfo& info;
    const FxGuide& guide;
    ParamToggle on;
    ParamKnob amount, p2;
    juce::Label text;
    juce::TextButton up { juce::String::fromUTF8("\xe2\x96\xb2") }, down { juce::String::fromUTF8("\xe2\x96\xbc") };
};

FxPanel::FxPanel(DaliVisualProcessor& p) : proc(p)
{
    addAndMakeVisible(header);
    addAndMakeVisible(resetOrder);
    addAndMakeVisible(allOff);
    addAndMakeVisible(intro);
    resetOrder.onClick = [this] { proc.pushUndo(); proc.effectChain.reset(); };
    allOff.setTooltip("Switch every effect off (Trip keeps working unless you turn Trip down in the SCENE tab)");
    allOff.onClick = [this]
    {
        proc.pushUndo();
        for (auto& e : effectLibrary())
            if (auto* prm = proc.apvts.getParameter(params::id::fxOn(e.id)))
            {
                prm->beginChangeGesture(); prm->setValueNotifyingHost(0.0f); prm->endChangeGesture();
            }
    };
    intro.setText("Switch an effect on and set its amount; they run top to bottom (arrows change the order). "
                  "Effects tagged TRIP are also added automatically by Trip in the SCENE tab, following the music.",
                  juce::dontSendNotification);
    intro.setFont(juce::Font(juce::FontOptions(12.0f)));
    intro.setColour(juce::Label::textColourId, colours::textDim);
    intro.setJustificationType(juce::Justification::topLeft);
    for (int i = 0; i < EffectChain::kNumEffects; ++i) addAndMakeVisible(rows.add(new Row(p, i)));
    proc.effectChain.addChangeListener(this);
}

FxPanel::~FxPanel() { proc.effectChain.removeChangeListener(this); }

int FxPanel::preferredHeight(int) { return kPad * 2 + 28 + 46 + EffectChain::kNumEffects * 76; }

void FxPanel::resized()
{
    auto r = getLocalBounds().reduced(kPad);
    auto top = r.removeFromTop(24);
    resetOrder.setBounds(top.removeFromRight(90));
    allOff.setBounds(top.removeFromRight(70).reduced(2, 0));
    header.setBounds(top);
    intro.setBounds(r.removeFromTop(44));
    r.removeFromTop(2);
    for (int e : proc.effectChain.getOrder()) { rows[e]->setBounds(r.removeFromTop(72)); r.removeFromTop(4); }
}

// =============================================================================
//  COLOR
// =============================================================================
class ColorPanel::Swatch : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    Swatch(DaliVisualProcessor& p, int idx) : proc(p), index(idx)
    {
        setTooltip(paletteNames()[idx]);
        startTimerHz(8);
    }
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced(2.0f);
        const auto pal = index == int(PaletteId::Custom)
            ? makeCustomPalette(*proc.apvts.getRawParameterValue(params::id::customHueA), *proc.apvts.getRawParameterValue(params::id::customHueB))
            : builtInPalette(index);
        juce::Image img(juce::Image::RGB, 64, 1, false);
        for (int x = 0; x < 64; ++x)
        {
            float c[3];
            for (int i = 0; i < 3; ++i)
                c[i] = juce::jlimit(0.0f, 1.0f, pal.a[i] + pal.b[i] * std::cos(juce::MathConstants<float>::twoPi * (pal.c[i] * x / 63.0f + pal.d[i])));
            img.setPixelAt(x, 0, juce::Colour::fromFloatRGBA(c[0], c[1], c[2], 1.0f));
        }
        juce::Path clip; clip.addRoundedRectangle(r.withTrimmedBottom(14.0f), 5.0f);
        g.saveState();
        g.reduceClipRegion(clip);
        g.drawImage(img, r.withTrimmedBottom(14.0f), juce::RectanglePlacement::stretchToFit);
        g.restoreState();
        g.setColour(selected ? colours::accent : colours::outline);
        g.drawRoundedRectangle(r.withTrimmedBottom(14.0f), 5.0f, selected ? 2.0f : 1.0f);
        g.setColour(selected ? colours::text : colours::textDim);
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawFittedText(paletteNames()[index], r.removeFromBottom(13.0f).toNearestInt(), juce::Justification::centred, 1);
    }
    void mouseUp(const juce::MouseEvent&) override
    {
        if (auto* prm = proc.apvts.getParameter(params::id::palette))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost(prm->convertTo0to1(float(index)));
            prm->endChangeGesture();
        }
    }
private:
    void timerCallback() override
    {
        const bool s = juce::roundToInt(proc.apvts.getRawParameterValue(params::id::palette)->load()) == index;
        if (s != selected || index == int(PaletteId::Custom)) { selected = s; repaint(); }
    }
    DaliVisualProcessor& proc;
    const int index;
    bool selected = false;
};

ColorPanel::ColorPanel(DaliVisualProcessor& p)
    : proc(p),
      hue(p, params::id::hue, "Hue"), saturation(p, params::id::saturation, "Saturation"),
      brightness(p, params::id::brightness, "Brightness"), contrast(p, params::id::contrast, "Contrast"),
      colorAmount(p, params::id::colorAmount, "Color Amount"), colorShift(p, params::id::colorShift, "Colour Family"),
      audioColor(p, params::id::audioColor, "Music > Colour"), bloom(p, params::id::bloom, "Bloom"),
      sharpen(p, params::id::sharpen, "Sharpness"),
      customA(p, params::id::customHueA, "Base Hue"),
      customB(p, params::id::customHueB, "Highlight Hue")
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &paletteHeader, &gradeHeader, &customHeader, &hue, &saturation, &brightness,
                     &contrast, &colorAmount, &colorShift, &audioColor, &bloom, &sharpen, &customA, &customB })
        addAndMakeVisible(c);
    for (int i = 0; i < int(PaletteId::count); ++i) addAndMakeVisible(swatches.add(new Swatch(p, i)));
}

ColorPanel::~ColorPanel() = default;

int ColorPanel::preferredHeight(int) { return kPad * 4 + kHeaderH * 3 + 3 * 46 + kKnobH * 2 + kKnobH; }

void ColorPanel::resized()
{
    // the colour of every scene first; the palette below only colours the Image Reactor
    auto r = getLocalBounds().reduced(kPad);
    gradeHeader.setBounds(r.removeFromTop(kHeaderH));
    layoutKnobGrid({ &colorShift, &audioColor, &hue, &saturation, &brightness, &contrast, &bloom, &sharpen },
                   r.removeFromTop(kKnobH * 2), 4, kKnobH);
    r.removeFromTop(kPad);
    paletteHeader.setBounds(r.removeFromTop(kHeaderH));
    auto grid = r.removeFromTop(3 * 46);
    const int sw = grid.getWidth() / 3;
    for (int i = 0; i < swatches.size(); ++i)
        swatches[i]->setBounds(grid.getX() + (i % 3) * sw, grid.getY() + (i / 3) * 46, sw, 46);
    r.removeFromTop(kPad);
    customHeader.setBounds(r.removeFromTop(kHeaderH));
    layoutKnobGrid({ &colorAmount, &customA, &customB }, r.removeFromTop(kKnobH), 4, kKnobH);
}

// =============================================================================
//  IMAGE — Image Reactor: the image itself becomes the visual (scene 17)
// =============================================================================
ImagePanel::ImagePanel(DaliVisualProcessor& p)
    : proc(p),
      overlay(p, params::id::tplEnable, "Overlay on current scene"), mirror(p, "tplMirror", "Mirror"),
      kaleido(p, "tplKaleido", "Kaleidoscope"), overlayMode(p, params::id::tplMode), blend(p, params::id::tplBlend)
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &sourceHeader, &modeHeader, &controlHeader,
             &overlayHeader, &hint, &status, &loadBtn, &clearBtn, &showBtn, &saveTpl, &loadTpl, &resetTpl, &routesBtn,
             &overlay, &mirror, &kaleido, &overlayMode, &blend })
        addAndMakeVisible(c);

    styleSmall(hint).setText("Drop a photo or logo anywhere on the plug-in: it becomes the visual itself, "
                             "moved and coloured by the sound.", juce::dontSendNotification);
    hint.setJustificationType(juce::Justification::topLeft);
    styleSmall(status);
    status.setJustificationType(juce::Justification::centred);
    status.setText(proc.image.getStatus(), juce::dontSendNotification);

    // the 8 visual modes as quick buttons (switch them live during a set)
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(params::id::imgMode)))
        for (int i = 0; i < choiceParam->choices.size(); ++i)
        {
            auto* b = modeButtons.add(new juce::TextButton(choiceParam->choices[i]));
            b->onClick = [this, i]
            {
                showImageScene();
                if (auto* prm = proc.apvts.getParameter(params::id::imgMode))
                {
                    prm->beginChangeGesture();
                    prm->setValueNotifyingHost(prm->convertTo0to1(float(i)));
                    prm->endChangeGesture();
                }
            };
            addAndMakeVisible(b);
        }

    const std::pair<const char*, const char*> controls[] = {
        { "tplScale", "Zoom" }, { "tplRotation", "Rotation" }, { "tplMotion", "Motion" }, { "tplSymCount", "Symmetry" },
        { "tplWarp", "Warp" }, { "tplTwist", "Twist" }, { "tplFeedback", "Trails" }, { "tplDetail", "Detail" },
        { "tplDepth", "Depth" }, { "tplColorExtract", "Palette Tint" }, { "tplEdge", "Outline" },
        { "tplDistortion", "Chroma" }, { "tplReact", "Reactivity" } };
    for (auto& [id, label] : controls) addAndMakeVisible(knobs.add(new ParamKnob(p, id, label)));
    knobs[9]->setTooltip("0 = the image's own colours, 1 = recoloured by the palette");
    knobs[12]->setTooltip("How strongly the image follows the sound");

    const std::pair<const char*, const char*> overlayControls[] = {
        { "tplMix", "Mix" }, { "tplSymmetry", "Symmetry" }, { "tplNoise", "Noise" }, { "tplRecursion", "Recursion" },
        { "tplThreshold", "Threshold" }, { "tplLuminance", "Luminance" }, { "tplColorAmount", "Color" },
        { "tplComplexity", "Complexity" } };
    for (auto& [id, label] : overlayControls) addAndMakeVisible(overlayKnobs.add(new ParamKnob(p, id, label)));

    showBtn.setClickingTogglesState(false);
    showBtn.setTooltip("Switch to scene 17 - the image itself as the visual");
    showBtn.onClick = [this] { showImageScene(); };
    loadBtn.onClick  = [this] { chooseImage(); };
    clearBtn.onClick = [this] { proc.image.clear(); };
    saveTpl.onClick  = [this] { saveTemplate(); };
    loadTpl.onClick  = [this] { loadTemplate(); };
    resetTpl.onClick = [this] { proc.pushUndo(); proc.templates.resetParameters(); };
    routesBtn.onClick = [this] { TemplateGenerator::addReactiveRoutes(proc.matrix); };
    saveTpl.setTooltip("Save image + settings as a .dvtemplate");
    loadTpl.setTooltip("Load a .dvtemplate");
    resetTpl.setTooltip("Reset the image controls");
    routesBtn.setTooltip("Adds sound routes: Bass > Zoom, Kick > Symmetry, Mid > Warp, Hi-Hat > Outline, Snare > Trails, Centroid > Rotation");
    proc.image.addChangeListener(this);
    startTimerHz(6);
}

ImagePanel::~ImagePanel() { proc.image.removeChangeListener(this); }

void ImagePanel::showImageScene()
{
    if (!proc.requireFeature(Feature::Image)) return;
    // coming from another scene: start clean so nothing of that preset is applied to the image
    const bool alreadyShowing = juce::roundToInt(proc.apvts.getRawParameterValue(params::id::scene)->load()) == kImageSceneIndex;
    if (!alreadyShowing) proc.applyImageReactorLook(false);
}

void ImagePanel::timerCallback()
{
    const bool showing = juce::roundToInt(proc.apvts.getRawParameterValue(params::id::scene)->load()) == kImageSceneIndex;
    showBtn.setToggleState(showing, juce::dontSendNotification);
    showBtn.setButtonText(showing ? "IMAGE VISUAL IS LIVE" : "SHOW IMAGE VISUAL");
    const int mode = juce::roundToInt(proc.apvts.getRawParameterValue(params::id::imgMode)->load());
    for (int i = 0; i < modeButtons.size(); ++i) modeButtons[i]->setToggleState(i == mode && showing, juce::dontSendNotification);
}

int ImagePanel::preferredHeight(int)
{
    return kPad * 6 + kHeaderH * 4 + 36 + 140 + 20 + 30 + 36 + 3 * 30 + ((13 + 3) / 4) * kKnobH
           + 30 + 30 + 2 * kKnobH + 32;
}

void ImagePanel::resized()
{
    auto r = getLocalBounds().reduced(kPad);
    sourceHeader.setBounds(r.removeFromTop(kHeaderH));
    hint.setBounds(r.removeFromTop(36));
    dropZone = r.removeFromTop(140);
    status.setBounds(r.removeFromTop(20));
    auto row = r.removeFromTop(30);
    loadBtn.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2));
    clearBtn.setBounds(row.reduced(2));
    showBtn.setBounds(r.removeFromTop(36).reduced(2, 3));
    r.removeFromTop(kPad);

    modeHeader.setBounds(r.removeFromTop(kHeaderH));
    const int rowsM = (modeButtons.size() + 2) / 3;
    auto grid = r.removeFromTop(rowsM * 30);
    const int bw = grid.getWidth() / 3;
    for (int i = 0; i < modeButtons.size(); ++i)
        modeButtons[i]->setBounds(juce::Rectangle<int>(grid.getX() + (i % 3) * bw, grid.getY() + (i / 3) * 30, bw, 30).reduced(2));
    r.removeFromTop(kPad);

    controlHeader.setBounds(r.removeFromTop(kHeaderH));
    juce::Array<juce::Component*> ks;
    for (auto* k : knobs) ks.add(k);
    r.removeFromTop(layoutKnobGrid(ks, r.withHeight(((knobs.size() + 3) / 4) * kKnobH), 4, kKnobH));
    r.removeFromTop(kPad);

    overlayHeader.setBounds(r.removeFromTop(kHeaderH));
    row = r.removeFromTop(30);
    overlay.setBounds(row.removeFromLeft(row.getWidth() / 2));
    mirror.setBounds(row.removeFromLeft(row.getWidth() / 2));
    kaleido.setBounds(row);
    row = r.removeFromTop(30);
    overlayMode.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2));
    blend.setBounds(row.reduced(2));
    juce::Array<juce::Component*> oks;
    for (auto* k : overlayKnobs) oks.add(k);
    r.removeFromTop(layoutKnobGrid(oks, r.withHeight(2 * kKnobH), 4, kKnobH));
    r.removeFromTop(kPad);

    row = r.removeFromTop(32);
    const int w4 = row.getWidth() / 4;
    saveTpl.setBounds(row.removeFromLeft(w4).reduced(2));
    loadTpl.setBounds(row.removeFromLeft(w4).reduced(2));
    resetTpl.setBounds(row.removeFromLeft(w4).reduced(2));
    routesBtn.setBounds(row.reduced(2));
}

void ImagePanel::paint(juce::Graphics& g)
{
    auto z = dropZone.toFloat().reduced(2.0f);
    g.setColour(colours::panel2);
    g.fillRoundedRectangle(z, 8.0f);
    juce::Path border; border.addRoundedRectangle(z, 8.0f);
    const float dash[] = { 6.0f, 4.0f };
    juce::Path dashed;
    juce::PathStrokeType(1.2f).createDashedStroke(dashed, border, dash, 2);
    g.setColour(proc.image.isBusy() ? colours::learn : colours::accent.withAlpha(0.7f));
    g.fillPath(dashed);

    const auto thumb = proc.image.getThumbnail();
    if (thumb.isValid())
        g.drawImage(thumb, z.reduced(10.0f), juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    else
    {
        g.setColour(colours::textDim);
        g.setFont(juce::Font(juce::FontOptions(13.0f)));
        g.drawFittedText("Drop a photo or logo here\n(or anywhere on the plug-in)", dropZone, juce::Justification::centred, 2);
    }
}

void ImagePanel::mouseUp(const juce::MouseEvent& e)
{
    if (dropZone.contains(e.getPosition())) chooseImage();
}

void ImagePanel::chooseImage()
{
    if (!proc.requireFeature(Feature::Image)) return;
    chooser = std::make_unique<juce::FileChooser>("Choose an image", juce::File(), "*.png;*.jpg;*.jpeg;*.gif;*.bmp");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f.existsAsFile() && proc.image.loadFile(f)) proc.applyImageReactorLook(true);
        });
}

void ImagePanel::saveTemplate()
{
    chooser = std::make_unique<juce::FileChooser>("Save template", TemplateGenerator::defaultFolder(),
                                                  juce::String("*") + TemplateGenerator::fileExtension);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
        [this](const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f != juce::File())
            {
                f.getParentDirectory().createDirectory();
                if (!proc.templates.saveToFile(f))
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Template", "Could not save the template.");
            }
        });
}

void ImagePanel::loadTemplate()
{
    if (!proc.requireFeature(Feature::Image)) return;
    chooser = std::make_unique<juce::FileChooser>("Load template", TemplateGenerator::defaultFolder(),
                                                  juce::String("*") + TemplateGenerator::fileExtension);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f.existsAsFile())
            {
                if (proc.templates.loadFromFile(f)) showImageScene();
                else juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Template", "Not a Dali Visual template.");
            }
        });
}
} // namespace dali
