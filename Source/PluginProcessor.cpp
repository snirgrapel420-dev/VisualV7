#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Core/AppPrefs.h"
#include "Core/StandaloneSupport.h"

DaliVisualProcessor::DaliVisualProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", dali::params::createLayout()),
      engineState(apvts, analyzer, matrix, effectChain, image),
      midi(apvts, engineState),
      output(engineState),
      templates(apvts, image)
{
    apvts.addParameterListener(dali::params::id::scene, this);
    lastSceneSeen = juce::roundToInt(apvts.getRawParameterValue(dali::params::id::scene)->load());
    sensitivityParam = apvts.getRawParameterValue(dali::params::id::sensitivity);

    // Scenes drive themselves from the music; the modulation matrix starts empty (advanced use).

    // Standalone on Windows listens to what the computer plays by default.
    setInputSource(canCaptureSystemAudio() ? SystemAudio : AudioInput);

    engineState.recorder = &recorder;
    analyzer.setTap(&recorder);

    // Standalone: JUCE mutes the audio input by default (feedback protection). Our output is always
    // silent, so open it - otherwise 'Audio Input' (the only source on a Mac) delivers pure silence.
    // Deferred: the standalone wrapper finishes loading its own settings after creating the processor.
    if (isStandalone())
        for (int delayMs : { 300, 1500, 4000 })
            juce::Timer::callAfterDelay(delayMs, [] { dali::standalone::unmuteInput(); });
}

void DaliVisualProcessor::setInputSource(int source)
{
    const int s = (source == SystemAudio && canCaptureSystemAudio()) ? SystemAudio : AudioInput;
    inputSource.store(s);
    if (s == SystemAudio)
    {
        analyzer.setSource(dali::AudioAnalyzer::Source::External);
        loopback.start();
    }
    else
    {
        loopback.stop();
        analyzer.setSource(dali::AudioAnalyzer::Source::Host);
    }
}

void DaliVisualProcessor::applyImageReactorLook(bool resetImageControls)
{
    pushUndo();
    auto set = [this](const juce::String& id, float realValue)
    {
        if (auto* p = apvts.getParameter(id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(realValue));
            p->endChangeGesture();
        }
    };

    if (resetImageControls) templates.resetParameters();          // image controls back to defaults (mode = Pulse)
    set(dali::params::id::tplEnable, 0.0f);                        // not an overlay: the image IS the scene
    set("tplColorExtract", 0.0f);                                  // the image's own colours
    for (auto& e : dali::effectLibrary()) set(dali::params::id::fxOn(e.id), 0.0f);   // nothing from the last preset
    for (const juce::String& id : { dali::params::id::macroA, dali::params::id::macroB, dali::params::id::macroC, dali::params::id::macroD })
        set(id, 0.5f);
    set(dali::params::id::bloom, 0.25f);
    set(dali::params::id::autoPilot, 0.0f);
    matrix.clear();
    dali::TemplateGenerator::addReactiveRoutes(matrix);            // sound-driven image routes only
    set(dali::params::id::scene, float(dali::kImageSceneIndex));
}

// ---- undo / redo ---------------------------------------------------------------------------
void DaliVisualProcessor::pushUndo()
{
    undoStack.push_back(captureState(false));
    if (undoStack.size() > 40) undoStack.erase(undoStack.begin());
    redoStack.clear();
    historyChanged.sendChangeMessage();
}

bool DaliVisualProcessor::undo()
{
    if (undoStack.empty()) return false;
    redoStack.push_back(captureState(false));
    const auto st = undoStack.back();
    undoStack.pop_back();
    applyState(st);
    lastSceneSeen = juce::roundToInt(apvts.getRawParameterValue(dali::params::id::scene)->load());
    historyChanged.sendChangeMessage();
    return true;
}

bool DaliVisualProcessor::redo()
{
    if (redoStack.empty()) return false;
    undoStack.push_back(captureState(false));
    const auto st = redoStack.back();
    redoStack.pop_back();
    applyState(st);
    lastSceneSeen = juce::roundToInt(apvts.getRawParameterValue(dali::params::id::scene)->load());
    historyChanged.sendChangeMessage();
    return true;
}

// ---- every scene starts at its own best values -------------------------------------------------
void DaliVisualProcessor::parameterChanged(const juce::String&, float)
{
    triggerAsyncUpdate();          // (may be called from the audio thread: apply on the message thread)
}

void DaliVisualProcessor::handleAsyncUpdate()
{
    const int scene = juce::roundToInt(apvts.getRawParameterValue(dali::params::id::scene)->load());
    if (scene == lastSceneSeen) return;
    lastSceneSeen = scene;
    if (restoring > 0 || apvts.getRawParameterValue(dali::params::id::sceneInit)->load() < 0.5f) return;
    applySceneInit();
}

void DaliVisualProcessor::applySceneInit(bool undoable)
{
    if (undoable) pushUndo();
    const juce::ScopedValueSetter<int> guard(restoring, restoring + 1);
    const int scene = juce::roundToInt(apvts.getRawParameterValue(dali::params::id::scene)->load());
    const auto& in = dali::sceneInit(scene);
    auto set = [this](const juce::String& id, float realValue)
    {
        if (auto* p = apvts.getParameter(id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(realValue));
            p->endChangeGesture();
        }
    };
    namespace id = dali::params::id;
    set(id::macroA, in.a); set(id::macroB, in.b); set(id::macroC, in.c); set(id::macroD, in.d);
    set(id::tripAmount, in.trip); set(id::intensity, in.intensity); set(id::speed, in.speed * 2.0f);   // Motion is a x0..x3 multiplier: 0.5 -> x1.0
    set(id::bloom, in.bloom); set(id::motionSmooth, in.smooth);

    // the scene's own MODULATION: which role drives what, visible and editable in the MOD tab.
    // Grammar (every scene): KICK -> the light (macro B) | MIDS -> the structure (macro A) |
    // slow BASS -> the speed (macro C) | HI-HAT -> bloom | SNARE -> trip | BASSLINE -> intensity |
    // CHAOS state -> the colour family (macro D). Amounts per scene, exceptions where the concept asks.
    struct Route { dali::ModSource src; const char* target; float amount, attackMs, releaseMs; };
    using MS = dali::ModSource;
    //                       kick  mid   bassS hat   snare bline chaos
    static const float amt[][7] = {
        /* 01 Kali       */ { 0.35f, 0.20f, 0.20f, 0.15f, 0.25f, 0.12f, 0.00f },
        /* 02 Tidal      */ { 0.35f, 0.15f, 0.20f, 0.15f, 0.20f, 0.10f, 0.30f },
        /* 03 Tunnel     */ { 0.15f, 0.25f, 0.30f, 0.15f, 0.30f, 0.12f, 0.35f },
        /* 04 Apollonian */ { 0.40f, 0.12f, 0.20f, 0.15f, 0.25f, 0.12f, 0.30f },
        /* 05 Gyroid     */ { 0.40f, 0.15f, 0.25f, 0.15f, 0.25f, 0.12f, 0.30f },
        /* 06 Mandelbulb */ { 0.40f, 0.20f, 0.20f, 0.15f, 0.25f, 0.12f, 0.30f },
        /* 07 Fourth Dim */ { 0.35f, 0.15f, 0.25f, 0.15f, 0.25f, 0.12f, 0.30f },
        /* 08 Mandelbox  */ { 0.40f, 0.10f, 0.20f, 0.15f, 0.25f, 0.12f, 0.30f },
        /* 09 Crystal    */ { 0.45f, 0.08f, 0.20f, 0.18f, 0.25f, 0.12f, 0.30f },
        /* 10 Menger     */ { 0.40f, 0.06f, 0.30f, 0.15f, 0.30f, 0.12f, 0.35f },
        /* 11 Julia      */ { 0.35f, 0.25f, 0.20f, 0.15f, 0.25f, 0.12f, 0.30f },
        /* 12 Ocean      */ { 0.30f, 0.10f, 0.25f, 0.10f, 0.15f, 0.10f, 0.30f },
        /* 13 Gate       */ { 0.45f, 0.08f, 0.30f, 0.15f, 0.30f, 0.12f, 0.35f },
        /* 14 Megastruct */ { 0.40f, 0.10f, 0.25f, 0.15f, 0.25f, 0.12f, 0.30f },
        /* 15 KIFS       */ { 0.40f, 0.12f, 0.20f, 0.15f, 0.25f, 0.12f, 0.30f },
        /* 16 Biomech    */ { 0.40f, 0.10f, 0.25f, 0.15f, 0.25f, 0.12f, 0.30f },
        /* 17 Nebula     */ { 0.35f, 0.15f, 0.20f, 0.12f, 0.20f, 0.12f, 0.30f },
        /* 18 Sierpinski */ { 0.45f, 0.06f, 0.20f, 0.15f, 0.30f, 0.12f, 0.30f },
        /* 19 Torus      */ { 0.40f, 0.20f, 0.20f, 0.15f, 0.25f, 0.12f, 0.30f },
        /* 20 Image      */ { 0.20f, 0.20f, 0.00f, 0.10f, 0.25f, 0.10f, 0.00f },
    };
    const int row = juce::jlimit(0, int(sizeof(amt) / sizeof(amt[0])) - 1, scene);
    const float* k = amt[row];
    const juce::String sceneId(dali::sceneLibrary()[size_t(scene)].id);
    // targets follow the grammar, with the exceptions the concepts ask for
    juce::String kickT = id::macroB, midT = id::macroA, bassT = id::macroC, chaosT = id::macroD;
    if (sceneId == "kali")  { kickT = id::macroD; }                         // Kali: macro D is its glow
    if (sceneId == "ocean") { bassT = id::macroA; midT = id::macroC; }      // Ocean: the bass is the swell
    if (sceneId == "nexus") { midT = id::macroC; bassT = id::macroA; }      // Torus: the mids spin the rings
    if (sceneId == "image") { kickT = id::macroC; }                         // Image: the kick punches the zoom
    const Route routes[] = {
        { MS::Kick,       kickT.toRawUTF8(),  k[0],   0.0f,  160.0f },
        { MS::Mid,        midT.toRawUTF8(),   k[1],  60.0f,  450.0f },
        { MS::BassSlow,   bassT.toRawUTF8(),  k[2], 200.0f,  900.0f },
        { MS::HiHat,      "bloom",            k[3],   0.0f,   90.0f },
        { MS::Snare,      sceneId == "image" ? "macroD" : "tripAmount", k[4], 0.0f, 240.0f },
        { MS::Bassline,   "intensity",        k[5],   0.0f,  110.0f },
        { MS::StateChaos, chaosT.toRawUTF8(), k[6], 600.0f, 2500.0f },
    };
    // only the previous scene's routes are replaced: routes the user built (or edited) stay untouched
    auto slots = matrix.getSlots();
    for (auto& sl : slots)
        if (sl.fromScene) { sl = dali::ModSlot {}; sl.source = 0; sl.target = -1; }
    auto isFree = [](const dali::ModSlot& sl) { return sl.source == 0 || sl.target < 0; };
    size_t next = 0;
    for (auto& r : routes)
    {
        if (r.amount <= 0.0f) continue;
        const int t = dali::ModulationTarget::fromParamId(r.target);
        if (t < 0) continue;
        while (next < slots.size() && !isFree(slots[next])) ++next;
        if (next >= slots.size()) break;                                // the user's routes fill the matrix
        auto& sl = slots[next++];
        sl = dali::ModSlot {};
        sl.enabled = true; sl.source = int(r.src); sl.target = t; sl.amount = r.amount;
        sl.attackMs = r.attackMs; sl.releaseMs = r.releaseMs; sl.smoothingMs = 20.0f;
        sl.fromScene = true;
    }
    matrix.setAll(slots);
}

void DaliVisualProcessor::resetEverything()
{
    pushUndo();
    {
        const juce::ScopedValueSetter<int> guard(restoring, restoring + 1);
        for (auto& d : dali::params::all())
            if (auto* prm = apvts.getParameter(d.id))
            {
                prm->beginChangeGesture();
                prm->setValueNotifyingHost(prm->getDefaultValue());
                prm->endChangeGesture();
            }
        matrix.clear();
        effectChain.reset();
        image.clear();
        lastSceneSeen = juce::roundToInt(apvts.getRawParameterValue(dali::params::id::scene)->load());
    }
    applySceneInit(false);
}

juce::String DaliVisualProcessor::getInputSourceStatus() const
{
    if (inputSource.load() == SystemAudio) return loopback.getStatus();
    if (isStandalone()) return "Audio input (Options > Audio/MIDI Settings)";
    return "Track audio from the host";
}

DaliVisualProcessor::~DaliVisualProcessor()
{
    apvts.removeParameterListener(dali::params::id::scene, this);
    cancelPendingUpdate();
    loopback.stop();
    analyzer.setTap(nullptr);
    output.close();
    engineState.recorder = nullptr;
    recorder.stop();                                   // the file is finished by the recorder's destructor
}

bool DaliVisualProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return in == out || in.isDisabled();
}

void DaliVisualProcessor::prepareToPlay(double sr, int)
{
    sampleRate = sr;
    analyzer.prepare(sr);
}

void DaliVisualProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const auto t0 = juce::Time::getHighResolutionTicks();
    const int numSamples = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch) buffer.clear(ch, 0, numSamples);

    // 1. analysis input (wait-free)
    if (numIn > 0 && numSamples > 0)
        analyzer.push(buffer.getReadPointer(0), numIn > 1 ? buffer.getReadPointer(1) : nullptr, numSamples);
    analyzer.setSensitivity(sensitivityParam->load());

    // 2. host transport
    dali::HostTiming ht;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            ht.valid = pos->getBpm().hasValue();
            ht.playing = pos->getIsPlaying();
            ht.bpm = pos->getBpm().orFallback(0.0);
            ht.ppq = pos->getPpqPosition().orFallback(0.0);
            ht.barStartPpq = pos->getPpqPositionOfLastBarStart().orFallback(0.0);
            if (auto sig = pos->getTimeSignature()) { ht.numerator = sig->numerator; ht.denominator = sig->denominator; }
            ht.stamp = dali::AudioAnalyzer::now();
        }
    }
    engineState.host.writeFromAudioThread(ht);

    // 3. MIDI
    for (const auto metadata : midiMessages)
        midi.pushFromAudioThread(metadata.getMessage());

    // Standalone: visual instrument only — never send the input back to the speakers.
    if (isStandalone()) buffer.clear();

    // audio-thread load (for the CPU meter)
    const double elapsed = juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks() - t0);
    const double budget = numSamples / juce::jmax(1.0, sampleRate);
    loadSmoothed += (float(elapsed / juce::jmax(1e-6, budget)) - loadSmoothed) * 0.05f;
    engineState.telemetry.cpuLoad.store(loadSmoothed);
}

// =============================================================================
//  state
// =============================================================================
juce::ValueTree DaliVisualProcessor::captureState(bool includeGlobal) const
{
    juce::ValueTree st(stateId);
    st.setProperty("version", 1, nullptr);

    juce::ValueTree p("Params");
    for (auto& d : dali::params::all())
        if (auto* prm = apvts.getParameter(d.id))
            p.setProperty(d.id, prm->convertFrom0to1(prm->getValue()), nullptr);
    st.appendChild(p, nullptr);

    st.appendChild(matrix.toValueTree(), nullptr);
    st.appendChild(effectChain.toValueTree(), nullptr);

    juce::ValueTree o("Output");
    o.setProperty("renderScaleV2", engineState.output.renderScaleIndex.load(), nullptr);
    o.setProperty("vsync", engineState.output.vsync.load(), nullptr);
    o.setProperty("previewWhileOutput", engineState.output.previewWhileOutput.load(), nullptr);
    o.setProperty("display", engineState.output.displayIndex.load(), nullptr);
    st.appendChild(o, nullptr);

    if (image.hasImage())
    {
        const auto png = image.getEncodedPNG();
        juce::ValueTree img("Image");
        img.setProperty("name", image.getName(), nullptr);
        img.setProperty("png", juce::Base64::toBase64(png.getData(), png.getSize()), nullptr);
        st.appendChild(img, nullptr);
    }

    if (includeGlobal)
    {
        st.appendChild(midi.toValueTree(), nullptr);
        st.setProperty("inputSource", inputSource.load(), nullptr);
    }
    return st;
}

void DaliVisualProcessor::applyState(const juce::ValueTree& st)
{
    if (!st.hasType(stateId)) return;
    const juce::ScopedValueSetter<int> guard(restoring, restoring + 1);

    const auto p = st.getChildWithName("Params");
    for (auto& d : dali::params::all())
        if (auto* prm = apvts.getParameter(d.id))
        {
            const float norm = p.hasProperty(d.id) ? prm->convertTo0to1(float(p.getProperty(d.id)))
                                                    : prm->getDefaultValue();
            prm->setValueNotifyingHost(norm);
        }

    matrix.fromValueTree(st.getChildWithName(dali::ModulationMatrix::treeId));
    lastSceneSeen = juce::roundToInt(apvts.getRawParameterValue(dali::params::id::scene)->load());   // restored, not chosen
    effectChain.fromValueTree(st.getChildWithName(dali::EffectChain::treeId));

    const auto o = st.getChildWithName("Output");
    if (o.isValid())
    {
        engineState.output.renderScaleIndex = int(o.getProperty("renderScaleV2", 0));   // (v1 indices meant something else)
        engineState.output.vsync = bool(o.getProperty("vsync", true));
        engineState.output.previewWhileOutput = bool(o.getProperty("previewWhileOutput", true));
        engineState.output.displayIndex = int(o.getProperty("display", -1));
    }

    const auto img = st.getChildWithName("Image");
    if (img.isValid())
    {
        juce::MemoryOutputStream out;
        if (juce::Base64::convertFromBase64(out, img.getProperty("png").toString()))
            image.loadEncoded(out.getMemoryBlock(), img.getProperty("name", "Image").toString());
    }
    // No image in the preset: keep the current source image (source and template are separate).

    const auto m = st.getChildWithName(dali::MidiMapper::treeId);
    if (m.isValid()) midi.fromValueTree(m);
    if (st.hasProperty("inputSource")) setInputSource(int(st.getProperty("inputSource")));
}

void DaliVisualProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = captureState(true).createXml())
        copyXmlToBinary(*xml, destData);
}

void DaliVisualProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        // STANDALONE: by default every launch starts fresh (only the setup comes back); the creative state
        // of the last session returns only when "Restore last session on launch" is on in SETTINGS
        bool fresh = false;
        if (isStandalone() && !dali::AppPrefs::restoreSession() && tree.hasType(stateId))
        {
            juce::ValueTree setup(stateId);
            setup.copyPropertiesFrom(tree, nullptr);
            for (const juce::Identifier keep : { juce::Identifier("Output"), dali::MidiMapper::treeId })
            {
                const auto c = tree.getChildWithName(keep);
                if (c.isValid()) setup.appendChild(c.createCopy(), nullptr);
            }
            tree = setup;
            fresh = true;
        }
        auto apply = [this, tree, fresh]
        {
            applyState(tree);
            if (fresh)
            {
                matrix.clear();
                effectChain.reset();
                applySceneInit(false);                // the first scene at its best
            }
        };
        if (juce::MessageManager::getInstance()->isThisTheMessageThread()) apply();
        else juce::MessageManager::callAsync(apply);
    }
}

juce::AudioProcessorEditor* DaliVisualProcessor::createEditor()
{
    return new DaliVisualEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DaliVisualProcessor();
}
