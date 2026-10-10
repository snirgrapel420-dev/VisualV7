#include "RenderEngine.h"
#include "../Output/NativeWindow.h"
#include "../Output/FrameSink.h"
#include "../Output/VideoRecorder.h"
#include "../Core/License.h"
#include "BinaryData.h"
#include <cstring>

using namespace juce::gl;

namespace dali
{
namespace
{
inline float smoothTo(float cur, float target, float dt, float tau) noexcept
{
    return tau <= 1e-4f ? target : cur + (target - cur) * (1.0f - std::exp(-dt / tau));
}
}

// =============================================================================
RenderEngine::RenderEngine(EngineState& s, juce::OpenGLContext& c, Role r)
    : state(s), context(c), role(r)
{
    using namespace params;
    auto I = [](const juce::String& pid) { const int i = indexOf(pid); jassert(i >= 0); return i; };
    pScene = I(id::scene); pIntensity = I(id::intensity); pSpeed = I(id::speed);
    pMacro[0] = I(id::macroA); pMacro[1] = I(id::macroB); pMacro[2] = I(id::macroC); pMacro[3] = I(id::macroD);
    pSensitivity = I(id::sensitivity); pSmoothing = I(id::smoothing);
    pReact[0] = I(id::reactBass); pReact[1] = I(id::reactMid); pReact[2] = I(id::reactHigh); pReact[3] = I(id::reactTransient);
    pSyncSource = I(id::syncSource); pSyncDiv = I(id::syncDiv); pInternalBpm = I(id::internalBpm);
    pPalette = I(id::palette); pHue = I(id::hue); pSat = I(id::saturation); pBright = I(id::brightness);
    pContrast = I(id::contrast); pColorAmount = I(id::colorAmount); pColorShift = I(id::colorShift);
    pAudioColor = I(id::audioColor); pCustomA = I(id::customHueA); pCustomB = I(id::customHueB);
    pTplEnable = I(id::tplEnable); pTplMode = I(id::tplMode); pTplBlend = I(id::tplBlend); pTplMix = I(id::tplMix);
    pTplMirror = I("tplMirror"); pTplKaleido = I("tplKaleido"); pTplRotation = I("tplRotation"); pTplMotion = I("tplMotion");
    pAudioDrive = I(id::audioDrive); pIdleMotion = I(id::idleMotion); pDynamics = I(id::dynamics);
    pAutoPilot = I(id::autoPilot); pAutoBars = I(id::autoBars); pAutoOnDrop = I(id::autoOnDrop); pBloom = I(id::bloom); pImgMode = I(id::imgMode); pAutoFX = I(id::autoFX); pSharpen = I(id::sharpen);
    pMotionSmooth = I(id::motionSmooth); pTransitionTime = I(id::transitionTime); pTrip = I(id::tripAmount); pChaos = I(id::chaosMode);
    jassert(juce::String(sceneLibrary()[size_t(kImageSceneIndex)].id) == "image");
    autoTargetIndex[0] = modTargetOf(pMacro[0]); autoTargetIndex[1] = modTargetOf(pMacro[1]);
    autoTargetIndex[2] = modTargetOf(pMacro[2]); autoTargetIndex[3] = modTargetOf(pMacro[3]);
    autoTargetIndex[4] = modTargetOf(pColorShift);

    tplUniforms = {
        { "uTScale", I("tplScale") }, { "uTSym", I("tplSymmetry") }, { "uTSymCount", I("tplSymCount") },
        { "uTWarp", I("tplWarp") }, { "uTTwist", I("tplTwist") }, { "uTNoise", I("tplNoise") },
        { "uTDistortion", I("tplDistortion") }, { "uTFeedback", I("tplFeedback") }, { "uTRecursion", I("tplRecursion") },
        { "uTEdge", I("tplEdge") }, { "uTThreshold", I("tplThreshold") }, { "uTLuminance", I("tplLuminance") },
        { "uTColorExtract", I("tplColorExtract") }, { "uTColorAmount", I("tplColorAmount") }, { "uTDetail", I("tplDetail") },
        { "uTComplexity", I("tplComplexity") }, { "uTDepth", I("tplDepth") }, { "uTReact", I("tplReact") } };

    for (auto& e : effectLibrary())
    {
        pFxOn.push_back(I(id::fxOn(e.id)));
        {
            const juce::String fid(e.id);
            const int idx = int(pFxOn.size()) - 1;
            if (fid == "kaleido") fxKaleido = idx; else if (fid == "trails") fxTrails = idx; else if (fid == "warp") fxWarp = idx;
            else if (fid == "hueshift") fxHue = idx; else if (fid == "chromatic") fxChroma = idx;
        }
        pFxAmt.push_back(I(id::fxAmt(e.id)));
        pFxP2.push_back(I(id::fxP2(e.id)));
    }
    modOffsets.assign(modTargets().size(), 0.0f);
}

RenderEngine::~RenderEngine() = default;

// =============================================================================
//  parameter access: atomic reads of the APVTS parameters + modulation offsets
// =============================================================================
float RenderEngine::effective(int index) const noexcept
{
    auto* p = state.param(index);
    if (p == nullptr) return 0.0f;
    float n = p->getValue();
    const int t = params::modTargetOf(index);
    if (t >= 0) n = juce::jlimit(0.0f, 1.0f, n + modOffsets[size_t(t)]);
    return p->convertFrom0to1(n);
}

int RenderEngine::choice(int index) const noexcept
{
    auto* p = state.param(index);
    return p != nullptr ? juce::roundToInt(p->convertFrom0to1(p->getValue())) : 0;
}

bool RenderEngine::flag(int index) const noexcept
{
    auto* p = state.param(index);
    return p != nullptr && p->getValue() > 0.5f;
}

// =============================================================================
//  GL lifecycle
// =============================================================================
void RenderEngine::newOpenGLContextCreated()
{
    glGenVertexArrays(1, &vao);
    vertexSrc = Shader::resource("fullscreen_vert");

    glGenTextures(1, &spectrumTex);                         // 128 x 2: spectrum + waveform for every scene
    glBindTexture(GL_TEXTURE_2D, spectrumTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 128, 2, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    juce::StringArray errors;
    auto check = [&](Shader& s, bool ok) { if (!ok) errors.add(s.getName() + ": " + s.getError()); };

    scenes.clear();
    for (auto& info : sceneLibrary())
    {
        auto sc = std::make_unique<VisualScene>(info);
        check(sc->shader, sc->compile(vertexSrc));
        scenes.push_back(std::move(sc));
    }
    effects.clear();
    for (auto& info : effectLibrary())
    {
        auto fx = std::make_unique<VisualEffect>(info);
        check(fx->shader, fx->compile(vertexSrc));
        effects.push_back(std::move(fx));
    }
    check(templateLayer,     templateLayer.build(vertexSrc, Shader::assembleFragment(Shader::resource("template_layer_frag")), "Template Layer"));
    check(templateComposite, templateComposite.build(vertexSrc, Shader::assembleFragment(Shader::resource("template_composite_frag")), "Template Composite"));
    check(outputShader,      outputShader.build(vertexSrc, Shader::assembleFragment(Shader::resource("output_frag")), "Output"));
    createWatermark();
    check(crossfade,         crossfade.build(vertexSrc, Shader::assembleFragment(Shader::resource("crossfade_frag")), "Crossfade"));

    const juce::String info = juce::String((const char*) glGetString(GL_RENDERER)) + "  |  OpenGL "
                            + juce::String((const char*) glGetString(GL_VERSION));
    {
        const juce::SpinLock::ScopedLockType sl(state.telemetry.infoLock);
        state.telemetry.rendererInfo = errors.isEmpty() ? info : info + "\nShader errors:\n" + errors.joinIntoString("\n");
    }
    DBG("DaliVisual renderer: " << info << (errors.isEmpty() ? juce::String() : "\n  " + errors.joinIntoString("\n  ")));

    ready = outputShader.isValid();
    startTime = lastTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
    fpsWindowStart = startTime;
    dnaVersion = 0xffffffffu;
    lastVsync = -1;
    currentScene = fadeFromScene = -1;
}

void RenderEngine::openGLContextClosing()
{
    {
        const juce::SpinLock::ScopedLockType sl(state.sinkLock);
        for (auto* s : state.sinks) s->contextClosing();
    }
    for (auto& s : scenes)  { s->shader.release(); s->history.release(); }
    for (auto& e : effects) { e->shader.release(); e->history.release(); }
    scenes.clear(); effects.clear();
    templateLayer.release(); templateComposite.release(); outputShader.release(); crossfade.release();
    templateHistory.release(); composite.release(); fadeTarget.release(); fxA.release(); fxB.release(); finalTarget.release();
    recordCapture.release();
    watermarkShader.release();
    if (logoTex != 0) glDeleteTextures(1, &logoTex);
    logoTex = 0;
    if (dnaTex != 0)   glDeleteTextures(1, &dnaTex);
    if (colorTex != 0) glDeleteTextures(1, &colorTex);
    if (flowTex != 0) glDeleteTextures(1, &flowTex);
    if (spectrumTex != 0) glDeleteTextures(1, &spectrumTex);
    dnaTex = colorTex = flowTex = spectrumTex = 0;
    hasImage = false;
    if (vao != 0) glDeleteVertexArrays(1, &vao);
    vao = 0;
    ready = false;
}

// =============================================================================
//  analysis → smoothed uniforms + musical clock
// =============================================================================
// TRIP: how much of each curated effect the musical state asks for (0 = off). CALM stays almost
// clean, BUILD tightens the warp, PEAK opens symmetry and trails, CHAOS pushes everything.
float RenderEngine::tripAmount(int e, float& p2) const
{
    const float trip = juce::jmax(effective(pTrip), chaosAmt) * 2.0f * (1.0f + 0.5f * chaosAmt);   // default 0.5 -> 1.0; CHAOS -> 3.0
    if (trip <= 0.001f) return 0.0f;
    const float calm = au.state[0], build = au.state[1], peak = au.state[2], chaos = au.state[3], rel = au.state[4];
    if (e == fxKaleido) { p2 = 0.3f + 0.25f * chaos; return juce::jlimit(0.0f, 1.0f, trip * (0.22f * peak + 0.45f * chaos + 0.08f * build)); }
    if (e == fxTrails)  { p2 = 0.15f + 0.3f * chaos; return juce::jlimit(0.0f, 0.88f, trip * (0.18f + 0.22f * peak + 0.3f * chaos + 0.15f * rel - 0.1f * calm)); }
    if (e == fxWarp)    { p2 = 0.4f;                 return juce::jlimit(0.0f, 1.0f, trip * (0.05f + 0.15f * build + 0.2f * chaos + 0.12f * au.kick * peak)); }
    if (e == fxHue)     { p2 = 0.1f + 0.3f * chaos;  return juce::jlimit(0.0f, 1.0f, trip * 0.08f * (peak + 2.0f * chaos)); }
    if (e == fxChroma)  { p2 = 0.0f;                 return juce::jlimit(0.0f, 1.0f, trip * (0.12f * au.snare + 0.2f * chaos)); }
    return 0.0f;
}

// Keep the picture smooth: when the frame rate sags, trade resolution and raymarch detail quickly;
// when there is headroom for a while, climb back slowly (hysteresis: no pumping).
void RenderEngine::updateQuality(float dt)
{
    if (dt <= 0.0f || dt > 0.5f) return;
    fpsAvg = smoothTo(fpsAvg, 1.0f / dt, dt, 0.4f);
    if (fpsAvg < 55.0f)       { quality = juce::jmax(0.0f, quality - dt * 0.8f); upHold = 4.0f; }
    else if (fpsAvg > 58.5f)  { upHold -= dt; if (upHold <= 0.0f) quality = juce::jmin(1.0f, quality + dt * 0.08f); }
}

bool RenderEngine::isLeader() const noexcept
{
    return role == Role::Output || !state.telemetry.outputActive.load();
}

void RenderEngine::pullTimeline()
{
    auto& tl = state.timeline;
    sceneTime = tl.sceneTime.load(); templateMotion = tl.templateMotion.load();
    bassTime = tl.bassTime.load(); midTime = tl.midTime.load(); highTime = tl.highTime.load(); levelTime = tl.levelTime.load();
    randomStep = tl.randomStep.load();
}

void RenderEngine::pushTimeline()
{
    auto& tl = state.timeline;
    tl.sceneTime = sceneTime; tl.templateMotion = templateMotion;
    tl.bassTime = bassTime; tl.midTime = midTime; tl.highTime = highTime; tl.levelTime = levelTime;
    tl.colorDrift = color.getDrift(); tl.randomStep = randomStep;
}

void RenderEngine::updateAnalysis(double now, float dt)
{
    // timeline leadership: a new leader continues the shared timeline instead of starting from zero
    // CHAOS: everything to its most psychedelic level (0.3 s in, 0.8 s out)
    chaosAmt = smoothTo(chaosAmt, flag(pChaos) ? 1.0f : 0.0f, dt, flag(pChaos) ? 0.3f : 0.8f);
    leading = isLeader();
    if (leading && !wasLeading) { pullTimeline(); color.setDrift(state.timeline.colorDrift.load()); }
    wasLeading = leading;

    const auto snap = state.analyzer.snapshot();
    // No audio arriving at all (no device, stopped host, nothing playing through the
    // loopback) → treat as silence so the picture comes to rest instead of freezing mid-motion.
    const bool stale = snap.stamp <= 0.0 || now - snap.stamp > 0.35;
    static const AudioFeatures silentFeatures {};
    const AudioFeatures& f = stale ? silentFeatures : snap.features;

    activity = smoothTo(activity, stale ? 0.0f : f.activity, dt, activity < f.activity ? 0.08f : 0.35f);
    if (!stale && f.dropCount != dropCount) dropCount = f.dropCount;

    const float tau  = effective(pSmoothing) * 0.25f;          // 0 .. 250 ms
    const float fast = tau * 0.3f;
    const float rb = effective(pReact[0]), rm = effective(pReact[1]), rh = effective(pReact[2]),
                rt = effective(pReact[3]) * (1.0f + 0.4f * chaosAmt);   // CHAOS: harder impacts

    au.bass         = smoothTo(au.bass,         f.bassEnv * rb,     dt, tau);
    au.mid          = smoothTo(au.mid,          f.midEnv * rm,      dt, tau);
    au.high         = smoothTo(au.high,         f.highEnv * rh,     dt, tau);
    au.energy       = smoothTo(au.energy,       f.energy,           dt, tau);
    au.kick         = smoothTo(au.kick,         f.kick * rt,        dt, fast);   // impacts: kick/snare/transients share one control
    au.transient    = smoothTo(au.transient,    f.transient * rt,   dt, fast);
    au.onset        = smoothTo(au.onset,        f.onset,            dt, fast);
    au.centroid     = smoothTo(au.centroid,     f.centroid,         dt, tau);
    au.flux         = smoothTo(au.flux,         f.flux,             dt, fast);
    au.width        = smoothTo(au.width,        f.width,            dt, tau);
    au.pan          = smoothTo(au.pan,          f.pan,              dt, tau);
    au.stereoEnergy = smoothTo(au.stereoEnergy, f.stereoEnergy,     dt, tau);
    au.rms          = smoothTo(au.rms,          f.rms,              dt, tau);
    au.peak         = smoothTo(au.peak,         f.peak,             dt, fast);
    au.snare        = smoothTo(au.snare,        f.snare * rt,       dt, fast);
    au.hat          = smoothTo(au.hat,          f.hat * rh,         dt, fast);
    au.build        = smoothTo(au.build,        f.build,            dt, tau);
    au.drop         = smoothTo(au.drop,         f.drop,             dt, fast);
    au.sub          = smoothTo(au.sub,          f.sub * rb,         dt, tau);
    au.bassNote     = smoothTo(au.bassNote,     f.bassNote * rb,    dt, fast);
    au.lowMid       = smoothTo(au.lowMid,       f.lowMid * rm,      dt, tau);
    au.highMid      = smoothTo(au.highMid,      f.highMid * rh,     dt, fast);
    au.energyMed    = smoothTo(au.energyMed,    f.energyMed,        dt, tau);
    au.midMed       = smoothTo(au.midMed,       f.midMed * rm,      dt, tau);
    au.bassSlow     = smoothTo(au.bassSlow,     f.bassSlow * rb,    dt, tau);
    au.energySlow   = smoothTo(au.energySlow,   f.energySlow,       dt, tau);
    au.centroidSlow = smoothTo(au.centroidSlow, f.centroidSlow,     dt, tau);
    au.fluxSlow     = smoothTo(au.fluxSlow,     f.fluxSlow,         dt, tau);
    au.kickDensity  = smoothTo(au.kickDensity,  f.kickDensity,      dt, 0.3f);
    au.onsetDensity = smoothTo(au.onsetDensity, f.onsetDensity,     dt, 0.3f);
    au.dynRange     = smoothTo(au.dynRange,     f.dynamicRange,     dt, 0.5f);
    au.stateTime    = stale ? 0.0f : f.stateTime;
    for (size_t i = 0; i < au.state.size(); ++i)                  // the analyzer already cross-fades states;
        au.state[i] = smoothTo(au.state[i], f.stateWeight[i], dt, 0.05f);   // silence/stale -> CALM
    if (chaosAmt > 0.001f)
    {
        static const float wild[5] = { 0.0f, 0.0f, 0.25f, 0.75f, 0.0f };
        for (size_t i = 0; i < au.state.size(); ++i) au.state[i] += (wild[i] - au.state[i]) * 0.85f * chaosAmt;
    }

    uploadSpectrum(f, stale);

    // band times: each clock only runs while its band sounds (Synesthesia-style)
    double spd = effective(pSpeed);
    // MOTION reads the sound on a slower time scale than IMPACTS: the clocks that move cameras and
    // structures follow smoothed envelopes, so a rolling 16th bassline does not make the motion
    // stutter; flashes and ripples keep using the fast envelopes.
    spd *= 1.0 + 0.5 * chaosAmt;                                     // CHAOS: everything moves faster
    const float flowTau = 0.05f + 0.75f * effective(pMotionSmooth);
    flowBass  = smoothTo(flowBass,  au.bass,   dt, flowTau);
    flowMid   = smoothTo(flowMid,   au.mid,    dt, flowTau);
    flowHigh  = smoothTo(flowHigh,  au.high,   dt, flowTau * 0.6f);
    flowLevel = smoothTo(flowLevel, au.energy, dt, flowTau);
    if (leading)
    {
        bassTime  = std::fmod(bassTime  + dt * spd * activity * (0.08 + 1.6 * flowBass),  10000.0);
        midTime   = std::fmod(midTime   + dt * spd * activity * (0.08 + 1.4 * flowMid),   10000.0);
        highTime  = std::fmod(highTime  + dt * spd * activity * (0.08 + 1.4 * flowHigh),  10000.0);
        levelTime = std::fmod(levelTime + dt * spd * activity * (0.08 + 1.5 * flowLevel), 10000.0);
    }

    DetectedTiming det;
    det.bpm = f.bpm; det.confidence = f.bpmConfidence; det.beatPhase = f.beatPhase; det.stamp = snap.stamp;
    // The musical clock only runs while music plays: in silence everything rests.
    if (activity > 0.01f)
    {
        clock.update(now, dt * juce::jmin(1.0f, activity * 1.5f), SyncSource(choice(pSyncSource)), state.host.read(), det,
                     effective(pInternalBpm), choice(pSyncDiv));
        if (leading && clock.beatHappened()) randomStep = random.nextFloat();
    }

    const auto midiCount = state.midiTriggerCount.load();
    if (midiCount != lastMidiCount) { lastMidiCount = midiCount; midiEnv = juce::jmax(midiEnv, state.midiTriggerVelocity.load()); }
    else midiEnv *= std::exp(-dt * 6.0f);
}

void RenderEngine::updateModulation(float dt)
{
    const float sp = clock.syncPhase();
    sources[ModSource::None]         = 0.0f;
    sources[ModSource::Bass]         = au.bass;
    sources[ModSource::Mid]          = au.mid;
    sources[ModSource::High]         = au.high;
    sources[ModSource::Energy]       = au.energy;
    sources[ModSource::Kick]         = au.kick;
    sources[ModSource::Transient]    = au.transient;
    sources[ModSource::Onset]        = au.onset;
    sources[ModSource::Beat]         = clock.beatPulse() * activity;
    sources[ModSource::BeatPhase]    = clock.beatPhase();
    sources[ModSource::BarPhase]     = clock.barPhase();
    sources[ModSource::SyncLFO]      = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * sp);
    sources[ModSource::SyncSaw]      = sp;
    sources[ModSource::SyncSquare]   = sp < 0.5f ? 1.0f : 0.0f;
    sources[ModSource::Centroid]     = au.centroid;
    sources[ModSource::Flux]         = au.flux;
    sources[ModSource::StereoWidth]  = au.width;
    sources[ModSource::StereoEnergy] = au.stereoEnergy;
    sources[ModSource::StereoPan]    = 0.5f + 0.5f * au.pan;
    sources[ModSource::RMS]          = au.rms;
    sources[ModSource::Peak]         = au.peak;
    sources[ModSource::MidiTrigger]  = midiEnv;
    sources[ModSource::RandomStep]   = randomStep;
    sources[ModSource::Snare]        = au.snare;
    sources[ModSource::HiHat]        = au.hat;
    sources[ModSource::Build]        = au.build;
    sources[ModSource::Drop]         = au.drop;
    sources[ModSource::Sub]          = au.sub;
    sources[ModSource::LowMid]       = au.lowMid;
    sources[ModSource::HighMid]      = au.highMid;
    sources[ModSource::BassSlow]     = au.bassSlow;
    sources[ModSource::EnergySlow]   = au.energySlow;
    sources[ModSource::KickDensity]  = au.kickDensity;
    sources[ModSource::OnsetDensity] = au.onsetDensity;
    sources[ModSource::DynamicRange] = au.dynRange;
    sources[ModSource::StateCalm]    = au.state[0];
    sources[ModSource::StateBuild]   = au.state[1];
    sources[ModSource::StatePeak]    = au.state[2];
    sources[ModSource::StateChaos]   = au.state[3];
    sources[ModSource::StateRelease] = au.state[4];
    sources[ModSource::Bassline]     = au.bassNote;

    const auto version = state.matrix.getVersion();
    if (version != lastMatrixVersion) { lastMatrixVersion = version; modEngine.reset(); }

    const auto slots = state.matrix.getSlots();
    modEngine.process(slots, sources, dt, modOffsets.data(), int(modOffsets.size()));
    updateAutoPilot(dt);
}

void RenderEngine::updateAutoPilot(float dt)
{
    static const int barsTable[] = { 2, 4, 8, 16, 32 };
    const int mode = choice(pAutoPilot);
    const int bars = barsTable[juce::jlimit(0, 4, choice(pAutoBars))];
    const bool onDrop = flag(pAutoOnDrop);

    bool snap = false;
    if (onDrop && dropCount != autoLastDrop) { autoLastDrop = dropCount; snap = true; }
    juce::ignoreUnused(bars);
    // the phrase key comes from Auto Pilot (one count for every engine: preview and output match)
    const std::int64_t key = state.telemetry.variationKey.load();

    if (mode == 0) autoTarget.fill(0.0f);
    else if (key != autoKey && activity > 0.5f)
    {
        juce::Random r(key * 7919 + 17);                 // same phrase → same variation in every engine
        for (int i = 0; i < 4; ++i) autoTarget[size_t(i)] = (r.nextFloat() - 0.5f) * 0.5f;   // macros ±0.25
        autoTarget[4] = r.nextFloat() * 0.35f;           // colour shift
    }
    if (mode != 0) autoKey = key;

    // glide half a bar into the new variation (or snap on a drop)
    const float beatSec = 60.0f / float(juce::jmax(40.0, clock.bpm() > 0 ? clock.bpm() : 120.0));
    const float tauA = snap ? 0.02f : beatSec * 2.0f;
    for (int i = 0; i < kAutoTargets; ++i)
    {
        autoCurrent[size_t(i)] = smoothTo(autoCurrent[size_t(i)], autoTarget[size_t(i)], dt * juce::jmax(0.05f, activity), tauA);
        const int t = autoTargetIndex[i];
        if (t >= 0) modOffsets[size_t(t)] += autoCurrent[size_t(i)];
    }
}

void RenderEngine::uploadImageIfChanged()
{
    const auto v = state.image.getVersion();
    if (v == dnaVersion) return;
    dnaVersion = v;

    auto dna = state.image.getDNA();
    if (dna == nullptr || !dna->isValid()) { hasImage = false; return; }

    auto upload = [&](unsigned int& tex, const std::vector<float>& data)
    {
        if (tex == 0) glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, dna->width, dna->height, 0, GL_RGBA, GL_FLOAT, data.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    };
    upload(dnaTex, dna->dna);
    upload(colorTex, dna->color);
    if (dna->flow.size() == dna->dna.size()) upload(flowTex, dna->flow);
    imgPalette = dna->palette;
    imgAspect = dna->aspect;
    imgMask = dna->hasAlpha ? 1.0f : 0.0f;
    hasImage = true;
    templateHistory.clear();
}

void RenderEngine::uploadSpectrum(const AudioFeatures& f, bool silent)
{
    // spectrum rows are already attack/release smoothed by the analyzer; ease further per frame
    for (int i = 0; i < 128; ++i)
    {
        const float target = silent ? 0.0f : f.spectrum[size_t(i)];
        float& v = spectrumData[size_t(i)];
        v += (target - v) * (target > v ? 0.85f : 0.25f);
        spectrumData[size_t(128 + i)] = silent ? 0.0f : f.wave[size_t(i)];
    }
    glBindTexture(GL_TEXTURE_2D, spectrumTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, 2, GL_RED, GL_FLOAT, spectrumData.data());
}

// =============================================================================
//  drawing helpers
// =============================================================================
void RenderEngine::bindTexture(int unit, unsigned int tex)
{
    glActiveTexture(GLenum(GL_TEXTURE0 + unit));
    glBindTexture(GL_TEXTURE_2D, tex);
}

void RenderEngine::drawFullscreen() { glDrawArrays(GL_TRIANGLES, 0, 3); }

// =============================================================================
//  DEMO watermark
// =============================================================================
void RenderEngine::createWatermark()
{
    static const char* frag = R"(#version 150
in vec2 vUV;
out vec4 fragColor;
uniform sampler2D uLogo;
uniform float uAlpha;
void main() { fragColor = texture(uLogo, vec2(vUV.x, 1.0 - vUV.y)) * uAlpha; }   // premultiplied
)";
    watermarkShader.build(vertexSrc, frag, "Watermark");

    int size = 0;
    juce::Image img;
    if (const char* data = BinaryData::getNamedResource("watermark_png", size))
        img = juce::ImageFileFormat::loadFrom(data, size_t(size));
    if (!img.isValid())
    {
        // no logo file: the name as text
        img = juce::Image(juce::Image::ARGB, 1024, 256, true);
        juce::Graphics g(img);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(150.0f, juce::Font::bold)));
        g.drawText("DALI AUDIO", img.getBounds(), juce::Justification::centred);
    }
    img = img.convertedToFormat(juce::Image::ARGB);
    const int w = img.getWidth(), h = img.getHeight();
    std::vector<juce::uint8> pixels(size_t(w) * size_t(h) * 4);
    {
        const juce::Image::BitmapData bd(img, juce::Image::BitmapData::readOnly);
        for (int y = 0; y < h; ++y)
            std::memcpy(pixels.data() + size_t(y) * size_t(w) * 4, bd.getLinePointer(y), size_t(w) * 4);   // BGRA, premultiplied
    }
    logoAspect = float(w) / float(juce::jmax(1, h));
    glGenTextures(1, &logoTex);
    glBindTexture(GL_TEXTURE_2D, logoTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderEngine::drawWatermark(int x, int y, int w, int h, bool recording)
{
    if (!License::showsWatermark() || logoTex == 0 || !watermarkShader.isValid() || w < 16 || h < 16) return;

    auto drawLogo = [&](int lx, int ly, int lw, int lh, float alpha)
    {
        glViewport(x + lx, y + ly, lw, lh);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        watermarkShader.use();
        watermarkShader.set("uLogo", 0);
        watermarkShader.set("uAlpha", alpha);
        bindTexture(0, logoTex);
        drawFullscreen();
        glDisable(GL_BLEND);
        glViewport(x, y, w, h);
    };

    if (recording)
    {
        // videos made with the demo / trial always carry a small logo in the corner as well
        int bw = juce::roundToInt(w * 0.16f), bh = juce::roundToInt(float(bw) / logoAspect);
        if (bh > h / 8) { bh = h / 8; bw = juce::roundToInt(float(bh) * logoAspect); }
        drawLogo(w - bw - juce::roundToInt(w * 0.025f), juce::roundToInt(h * 0.03f), bw, bh, 0.7f);
    }

    // every 12 s, 3.2 s on screen (fading in and out), at a different spot each time
    const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
    constexpr double period = 12.0, shown = 3.2, fade = 0.45;
    const double phase = std::fmod(t, period);
    if (phase > shown) return;
    const float alpha = 0.9f * float(juce::jmin(1.0, phase / fade, (shown - phase) / fade));
    const int spot = int(std::fmod(t / period, 5.0));

    int lw = juce::roundToInt(w * 0.3f), lh = juce::roundToInt(float(lw) / logoAspect);
    if (lh > h / 4) { lh = h / 4; lw = juce::roundToInt(float(lh) * logoAspect); }
    const int mx = juce::roundToInt(w * 0.05f), my = juce::roundToInt(h * 0.07f);
    int lx = (w - lw) / 2, ly = (h - lh) / 2;                                  // 0: centre
    if (spot == 1) { lx = mx;          ly = h - lh - my; }                     // top left (GL: y up)
    if (spot == 2) { lx = w - lw - mx; ly = h - lh - my; }                     // top right
    if (spot == 3) { lx = w - lw - mx; ly = my; }                              // bottom right
    if (spot == 4) { lx = mx;          ly = my; }                              // bottom left

    drawLogo(lx, ly, lw, lh, alpha);
}

void RenderEngine::setCommon(Shader& s, int w, int h)
{
    s.use();
    s.set("uRes", float(w), float(h));
    s.set("uTime", float(std::fmod(sceneTime, 7200.0)));
    s.set("uAbsTime", float(std::fmod(lastTime - startTime, 3600.0)));
    s.set("uBass", au.bass);       s.set("uMid", au.mid);         s.set("uHigh", au.high);
    s.set("uEnergy", au.energy);   s.set("uKick", au.kick);       s.set("uTransient", au.transient);
    s.set("uBeat", clock.beatPulse() * activity);
    s.set("uSnare", au.snare); s.set("uHat", au.hat); s.set("uBuild", au.build); s.set("uDrop", au.drop);
    s.set("uActivity", activity);
    s.set("uBassTime", float(bassTime)); s.set("uMidTime", float(midTime));
    s.set("uHighTime", float(highTime)); s.set("uLevelTime", float(levelTime));
    s.set("uSpectrum", 5);
    s.set("uQuality", shaderQuality);
    s.set("uColourShift", effective(pColorShift)); s.set("uMusicColour", juce::jmax(effective(pAudioColor), chaosAmt));
    s.set("uSub", au.sub); s.set("uBassNote", au.bassNote); s.set("uLowMid", au.lowMid); s.set("uHighMid", au.highMid);
    s.set("uEnergyMed", au.energyMed); s.set("uMidMed", au.midMed);
    s.set("uBassSlow", au.bassSlow); s.set("uEnergySlow", au.energySlow);
    s.set("uCentroidSlow", au.centroidSlow); s.set("uFluxSlow", au.fluxSlow);
    s.set("uKickDensity", au.kickDensity); s.set("uOnsetDensity", au.onsetDensity); s.set("uDynRange", au.dynRange);
    s.set("uState", au.state[0], au.state[1], au.state[2], au.state[3]);
    s.set("uStateRelease", au.state[4]);
    s.set("uStateTime", au.stateTime);
    s.set("uCentroid", au.centroid); s.set("uFlux", au.flux); s.set("uWidth", au.width); s.set("uPan", au.pan);
    s.set("uBeatPhase", clock.beatPhase()); s.set("uBarPhase", clock.barPhase()); s.set("uSyncPhase", clock.syncPhase());
    s.set("uBeatClock", float(std::fmod(clock.beatClock(), 256.0)));
    s.set("uMacro", effective(pMacro[0]), effective(pMacro[1]), effective(pMacro[2]), effective(pMacro[3]));
    s.set("uIntensity", effective(pIntensity));
    color.apply(s);
    s.set("uTex", 0); s.set("uPrev", 1); s.set("uDNA", 2); s.set("uImgColor", 3); s.set("uLayer", 4);
}

void RenderEngine::setImageUniforms(Shader& s)
{
    // the Image Reactor scene renders the loaded image itself, with the image parameters
    for (auto& [uniform, index] : tplUniforms) s.set(uniform, effective(index));
    s.set("uTAngle", float(effective(pTplRotation) * juce::MathConstants<double>::twoPi));
    s.set("uImgMotion", effective(pTplMotion));
    s.set("uImgMode", choice(pImgMode));
    s.set("uImgAspect", imgAspect);
    s.set("uImgMask", imgMask);
    s.set("uHasImage", hasImage ? 1.0f : 0.0f);
    bindTexture(2, hasImage ? dnaTex : 0u);
    bindTexture(3, hasImage ? colorTex : 0u);
    bindTexture(6, hasImage ? flowTex : 0u);
    s.set("uImgFlow", 6);
    s.set3("uImgPal0", imgPalette[0].data()); s.set3("uImgPal1", imgPalette[1].data());
    s.set3("uImgPal2", imgPalette[2].data()); s.set3("uImgPal3", imgPalette[3].data());
}

void RenderEngine::renderScene(VisualScene& sc, int w, int h)
{
    if (sc.history.ensure(w, h)) sc.history.clear();
    auto& target = sc.history.current();
    auto& prev = sc.history.previous();
    target.bind();
    setCommon(sc.shader, w, h);
    if (&sc == scenes[size_t(kImageSceneIndex)].get()) setImageUniforms(sc.shader);
    bindTexture(0, prev.texture());
    bindTexture(1, prev.texture());
    drawFullscreen();
    sc.history.swap();                          // previous() now holds this frame
}

// =============================================================================
//  frame
// =============================================================================
void RenderEngine::renderOpenGL()
{
    if (!ready) return;

    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const float dt = (float) juce::jlimit(0.0, 0.1, now - lastTime);
    lastTime = now;

    const int vs = state.output.vsync.load() ? 1 : 0;
    if (vs != lastVsync) { context.setSwapInterval(vs); lastVsync = vs; }

    GLint screenFbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &screenFbo);

    const double scale = context.getRenderingScale();
    const int physW = juce::jmax(1, juce::roundToInt(logicalW.load() * scale));
    const int physH = juce::jmax(1, juce::roundToInt(logicalH.load() * scale));

    const bool outputActive = state.telemetry.outputActive.load();
    const bool isPreview = role == Role::Preview;

    if (isPreview && outputActive && !state.output.previewWhileOutput.load())
    {
        glBindFramebuffer(GL_FRAMEBUFFER, GLuint(screenFbo));
        glViewport(0, 0, physW, physH);
        glClearColor(0.03f, 0.015f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    const int scaleIdx = state.output.renderScaleIndex.load();
    const bool adaptive = scaleIdx == OutputSettings::kAdaptive;
    if (adaptive) updateQuality(dt); else quality = 1.0f;
    float resScale = adaptive ? 0.55f + 0.45f * quality : OutputSettings::scaleFor(scaleIdx);
    shaderQuality = adaptive ? quality : 1.0f;
    if (isPreview && outputActive)
    {
        // while live the preview is a monitor: small and light, the GPU belongs to the projector
        resScale = juce::jmin(resScale, 640.0f / float(juce::jmax(1, physW)));
        shaderQuality = juce::jmin(shaderQuality, 0.6f);
    }
    else if (!isPreview || !outputActive) state.telemetry.quality = shaderQuality;
    // (the preview window is small: rendering it at full quality costs little, and it must match the output)
    const bool imageAllowed = License::allows(Feature::Image);               // DEMO lock

    // RECORDING: the engine that publishes the output also renders the video frames. The preview then
    // renders the recording's own frame (its size and aspect, shown letterboxed) - what you see is what
    // is recorded, at full recording resolution; the live output keeps its screen and is cropped to fill.
    auto* recorder = state.recorder;
    const bool publishes = (role == Role::Output) || !outputActive;
    int recW = 0, recH = 0;
    const bool recordingHere = recorder != nullptr && publishes && recorder->frameSize(physW, physH, recW, recH);
    const bool recordFrameView = recordingHere && isPreview;
    const int baseW = recordFrameView ? recW : physW;
    const int baseH = recordFrameView ? recH : physH;
    const int w = juce::jmax(16, juce::roundToInt(baseW * resScale));
    const int h = juce::jmax(16, juce::roundToInt(baseH * resScale));

    updateAnalysis(now, dt);
    updateModulation(dt);

    // Motion follows the music: silence rests (or idles), louder passages move faster.
    const float speed = effective(pSpeed);
    const float drive = effective(pAudioDrive);
    const float musical = (1.0f - drive) + drive * (0.25f + 1.1f * au.energy + 0.35f * au.bass);
    const float rate = juce::jmap(activity, effective(pIdleMotion), musical);
    if (leading)
    {
        sceneTime += dt * speed * rate;
        templateMotion += dt * effective(pTplMotion) * (0.5 + 0.5 * speed) * rate;
    }

    ColorSystem::Inputs ci;
    ci.palette = choice(pPalette); ci.customHueA = effective(pCustomA); ci.customHueB = effective(pCustomB);
    ci.colorShift = effective(pColorShift); ci.audioColor = effective(pAudioColor); ci.colorAmount = effective(pColorAmount);
    ci.centroid = au.centroid; ci.flux = au.flux; ci.kick = au.kick;
    if (!leading) color.setDrift(state.timeline.colorDrift.load());
    color.update(ci, leading ? dt : 0.0f);
    if (leading) pushTimeline(); else pullTimeline();

    uploadImageIfChanged();

    glBindVertexArray(vao);
    bindTexture(5, spectrumTex);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);

    // ---- 1. scene (+ crossfade on change) ----------------------------------------------------
    const int sceneIndex = License::nearestAllowedScene(juce::jlimit(0, int(scenes.size()) - 1, choice(pScene)));   // DEMO lock
    if (sceneIndex != currentScene)
    {
        if (currentScene >= 0 && currentScene != sceneIndex) { fadeFromScene = currentScene; fadeAmount = 0.0f; }
        currentScene = sceneIndex;
        scenes[size_t(sceneIndex)]->history.clear();
    }

    VisualScene& scene = *scenes[size_t(currentScene)];
    if (scene.shader.isValid()) renderScene(scene, w, h);
    else { scene.history.ensure(w, h); }
    unsigned int src = scene.history.previous().texture();

    if (fadeFromScene >= 0)
    {
        fadeAmount += dt / juce::jmax(0.3f, effective(pTransitionTime));   // user-set transition time
        VisualScene& old = *scenes[size_t(fadeFromScene)];
        if (fadeAmount >= 1.0f || !old.shader.isValid() || !crossfade.isValid())
        {
            old.history.release();
            fadeFromScene = -1;
        }
        else
        {
            renderScene(old, w, h);
            fadeTarget.ensure(w, h);
            fadeTarget.bind();
            setCommon(crossfade, w, h);
            crossfade.set("uMix", fadeAmount);
            bindTexture(0, old.history.previous().texture());
            bindTexture(4, src);
            drawFullscreen();
            src = fadeTarget.texture();
        }
    }

    // ---- 2. image template layer ------------------------------------------------------------------
    if (hasImage && imageAllowed && flag(pTplEnable) && currentScene != kImageSceneIndex && templateLayer.isValid() && templateComposite.isValid())
    {
        if (templateHistory.ensure(w, h)) templateHistory.clear();
        templateHistory.current().bind();
        setCommon(templateLayer, w, h);
        for (auto& [uniform, index] : tplUniforms) templateLayer.set(uniform, effective(index));
        templateLayer.set("uTMode", choice(pTplMode));
        templateLayer.set("uTMirror", flag(pTplMirror) ? 1.0f : 0.0f);
        templateLayer.set("uTKaleido", flag(pTplKaleido) ? 1.0f : 0.0f);
        templateLayer.set("uTAngle", float(effective(pTplRotation) * juce::MathConstants<double>::twoPi + templateMotion * 0.5));
        templateLayer.set("uTMotion", float(std::fmod(templateMotion, 1000.0)));
        templateLayer.set("uImgAspect", imgAspect);
        bindTexture(0, src);
        bindTexture(1, templateHistory.previous().texture());
        bindTexture(2, dnaTex);
        bindTexture(3, colorTex);
        drawFullscreen();
        templateHistory.swap();

        composite.ensure(w, h);
        composite.bind();
        setCommon(templateComposite, w, h);
        templateComposite.set("uTMix", effective(pTplMix));
        templateComposite.set("uTBlend", choice(pTplBlend));
        bindTexture(0, src);
        bindTexture(4, templateHistory.previous().texture());
        drawFullscreen();
        src = composite.texture();
    }

    // ---- 3. effects rack ---------------------------------------------------------------------------
    const auto order = state.effects.getOrder();
    bool useA = true;
    for (int pos = 0; pos < EffectChain::kNumEffects; ++pos)
    {
        const int e = order[size_t(pos)];
        VisualEffect& fx = *effects[size_t(e)];
        float autoP2 = 0.0f;
        const float autoAmt = tripAmount(e, autoP2);
        const bool userOn = flag(pFxOn[size_t(e)]);
        const bool on = (userOn || autoAmt > 0.02f) && fx.shader.isValid();
        if (!on)
        {
            if (fx.wasActive && fx.info.stateful) fx.history.release();
            fx.wasActive = false;
            continue;
        }

        RenderTarget* out = nullptr;
        unsigned int prevTex = src;
        if (fx.info.stateful)
        {
            if (fx.history.ensure(w, h) || !fx.wasActive) fx.history.clear();
            out = &fx.history.current();
            prevTex = fx.history.previous().texture();
        }
        else
        {
            out = useA ? &fxA : &fxB;
            if (out->texture() == src) { useA = !useA; out = useA ? &fxA : &fxB; }
            out->ensure(w, h);
            useA = !useA;
        }
        fx.wasActive = true;

        out->bind();
        setCommon(fx.shader, w, h);
        fx.shader.set("uAmt", userOn ? juce::jmax(effective(pFxAmt[size_t(e)]), autoAmt) : autoAmt);
        fx.shader.set("uP2",  userOn ? effective(pFxP2[size_t(e)]) : autoP2);
        bindTexture(0, src);
        bindTexture(1, prevTex);
        drawFullscreen();
        src = out->texture();
        if (fx.info.stateful) fx.history.swap();
    }

    // ---- 4. output ------------------------------------------------------------------------------------
    auto runOutput = [&](int vw, int vh)
    {
        setCommon(outputShader, vw, vh);
        outputShader.set("uHue", effective(pHue));
        outputShader.set("uSaturation", effective(pSat));
        outputShader.set("uBrightness", effective(pBright));
        outputShader.set("uContrast", effective(pContrast));
        outputShader.set("uDynamics", effective(pDynamics));
        outputShader.set("uBloom", effective(pBloom));
        outputShader.set("uAutoFX", juce::jmax(effective(pAutoFX), chaosAmt) * (1.0f + 0.5f * chaosAmt));
        outputShader.set("uSharpen", effective(pSharpen));
        bindTexture(0, src);
        drawFullscreen();
    };

    // bloom reads the mip chain of the final image
    bindTexture(0, src);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    const bool publisher = (role == Role::Output) || !outputActive;
    bool haveSinks = false;
    { const juce::SpinLock::ScopedLockType sl(state.sinkLock); haveSinks = !state.sinks.isEmpty(); }
    if (publisher && haveSinks)
    {
        finalTarget.ensure(w, h);
        finalTarget.bind();
        runOutput(w, h);
        const juce::SpinLock::ScopedLockType sl(state.sinkLock);
        for (auto* s : state.sinks) s->publishFrame(finalTarget.texture(), w, h);
    }

    // the screen pass fills the REAL surface: on mixed-DPI setups the scale factor can disagree with the
    // window, which left dead borders around the output; the drawable's true size is the authority
    int screenW = physW, screenH = physH;
    if (role == Role::Output)
    {
        int dw = 0, dh = 0;
        if (native::currentDrawableSize(dw, dh)) { screenW = dw; screenH = dh; }
    }
    if (role == Role::Output) { state.telemetry.outSurfaceW = screenW; state.telemetry.outSurfaceH = screenH;
                                state.telemetry.outRenderW = w; state.telemetry.outRenderH = h; }
    if (recordingHere)
        recordCapture.process(*recorder, recW, recH, w, h, [&](int vw, int vh) { runOutput(vw, vh); },
                              [&](int fw, int fh) { drawWatermark(0, 0, fw, fh, true); });
    else if (recordCapture.isAllocated())
        recordCapture.release();

    glBindFramebuffer(GL_FRAMEBUFFER, GLuint(screenFbo));
    glViewport(0, 0, screenW, screenH);
    if (recordFrameView && std::abs(float(w) / float(h) - float(screenW) / float(juce::jmax(1, screenH))) > 0.01f)
    {
        // letterbox: the recording frame inside the preview
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        const float a = float(w) / float(h);
        int vw = screenW, vh = juce::roundToInt(screenW / a);
        if (vh > screenH) { vh = screenH; vw = juce::roundToInt(screenH * a); }
        glViewport((screenW - vw) / 2, (screenH - vh) / 2, vw, vh);
        runOutput(vw, vh);
        drawWatermark((screenW - vw) / 2, (screenH - vh) / 2, vw, vh);
    }
    else
    {
        runOutput(screenW, screenH);
        drawWatermark(0, 0, screenW, screenH);
    }

    bindTexture(0, src);                                  // other passes sample it at level 0 only
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    bindTexture(6, 0); bindTexture(5, 0); bindTexture(4, 0); bindTexture(3, 0); bindTexture(2, 0); bindTexture(1, 0); bindTexture(0, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    publishTelemetry(now);
}

void RenderEngine::publishTelemetry(double now)
{
    ++frameCounter;
    const bool isPreview = role == Role::Preview;
    if (now - fpsWindowStart >= 0.5)
    {
        const float fps = float(frameCounter / (now - fpsWindowStart));
        (isPreview ? state.telemetry.previewFps : state.telemetry.outputFps).store(fps);
        if (!isPreview || !state.telemetry.outputActive.load())
            state.telemetry.frameMs.store(fps > 0 ? 1000.0f / fps : 0.0f);
        frameCounter = 0;
        fpsWindowStart = now;
    }

    // Clock + modulation telemetry comes from the engine showing the main picture.
    const bool mainPicture = isPreview ? !state.telemetry.outputActive.load() || state.output.previewWhileOutput.load()
                                       : true;
    if (!mainPicture) return;

    state.telemetry.bpm.store(float(clock.bpm()));
    state.telemetry.clockSource.store(int(clock.source()));
    state.telemetry.beatPulse.store(clock.beatPulse() * activity);
    state.telemetry.activity.store(activity);
    state.telemetry.barCount.store(std::int64_t(std::floor(clock.beatClock() / 4.0)));

    const auto& targets = params::modTargets();
    for (size_t t = 0; t < targets.size(); ++t)
    {
        const int pi = targets[t];
        if (pi >= EngineState::kMaxParams) continue;
        if (std::abs(modOffsets[t]) > 1e-4f)
        {
            auto* p = state.param(pi);
            state.modulated[size_t(pi)].store(p != nullptr ? juce::jlimit(0.0f, 1.0f, p->getValue() + modOffsets[t]) : -1.0f);
        }
        else state.modulated[size_t(pi)].store(-1.0f);
    }
}
} // namespace dali
