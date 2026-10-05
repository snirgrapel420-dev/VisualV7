#include "PluginProcessor.h"
#include "PluginEditor.h"

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

void DaliVisualProcessor::applySceneInit()
{
    pushUndo();
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
    output.close();
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
        if (juce::MessageManager::getInstance()->isThisTheMessageThread()) applyState(tree);
        else juce::MessageManager::callAsync([this, tree] { applyState(tree); });
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
