#pragma once
// ============================================================================
//  RenderEngine — the GPU pipeline. One instance per OpenGL context (editor
//  preview and fullscreen output each own one); both are driven by the same
//  EngineState, so they show the same musical picture.
//
//  Per frame (GL thread — never touches the audio thread):
//    1  analysis snapshot + host timing → smoothing → MusicalClock
//    2  modulation sources → ModulationEngine → effective parameter values
//    3  scene pass            RGBA16F, own feedback history; 0.6 s luminance-
//                             keyed crossfade when the scene changes
//    4  image template layer  own feedback, composited over the scene
//    5  effects rack          user order; Feedback/Trails keep history
//    6  output pass           colour grade → screen (+ FrameSinks)
//
//  All shaders compile once in newOpenGLContextCreated(): switching scenes or
//  effects never stalls a frame.
// ============================================================================
#include <juce_opengl/juce_opengl.h>
#include "../Core/EngineState.h"
#include "../Modulation/ModulationCore.h"
#include "ColorSystem.h"
#include "VisualEffect.h"
#include "VisualScene.h"
#include "RecordCapture.h"
#include <array>
#include <memory>
#include <vector>

namespace dali
{
class RenderEngine : public juce::OpenGLRenderer
{
public:
    enum class Role { Preview, Output };

    RenderEngine(EngineState& state, juce::OpenGLContext& context, Role role);
    ~RenderEngine() override;

    /** Message thread: logical (un-scaled) size of the attached component. */
    void setLogicalSize(int w, int h) noexcept { logicalW.store(w); logicalH.store(h); }

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

private:
    struct AudioUniforms
    {
        float bass = 0, mid = 0, high = 0, energy = 0, kick = 0, transient = 0, onset = 0;
        float centroid = 0, flux = 0, width = 0, pan = 0, stereoEnergy = 0, rms = 0, peak = 0;
        float snare = 0, hat = 0, build = 0, drop = 0;
        // v4: six bands, time scales, densities, musical state
        float sub = 0, lowMid = 0, highMid = 0, energyMed = 0, midMed = 0, bassNote = 0;
        float bassSlow = 0, energySlow = 0, centroidSlow = 0, fluxSlow = 0;
        float kickDensity = 0, onsetDensity = 0, dynRange = 0, stateTime = 0;
        std::array<float, 5> state {{ 1, 0, 0, 0, 0 }};
    };

    float effective(int index) const noexcept;       // real value including modulation
    int   choice(int index) const noexcept;
    bool  flag(int index) const noexcept;

    void updateAnalysis(double now, float dt);
    void updateModulation(float dt);
    void uploadImageIfChanged();
    void uploadSpectrum(const AudioFeatures& f, bool silent);
    void renderScene(VisualScene& sc, int w, int h);
    void setImageUniforms(Shader& s);
    void setCommon(Shader& s, int w, int h);
    void drawFullscreen();
    static void bindTexture(int unit, unsigned int tex);
    void publishTelemetry(double now);
    bool isLeader() const noexcept;
    void pullTimeline();
    void pushTimeline();
    void updateAutoPilot(float dt);

    EngineState& state;
    juce::OpenGLContext& context;
    const Role role;
    std::atomic<int> logicalW { 640 }, logicalH { 360 };

    // --- GL resources ------------------------------------------------------------------
    bool ready = false;
    unsigned int vao = 0;
    juce::String vertexSrc;
    std::vector<std::unique_ptr<VisualScene>>  scenes;
    std::vector<std::unique_ptr<VisualEffect>> effects;
    Shader templateLayer, templateComposite, outputShader, crossfade;
    PingPong templateHistory;
    RenderTarget composite, fadeTarget, fxA, fxB, finalTarget;
    RecordCapture recordCapture;                     // video recording (this engine publishes the output)
    unsigned int dnaTex = 0, colorTex = 0, flowTex = 0, spectrumTex = 0;
    std::array<std::array<float, 3>, 4> imgPalette {};
    std::array<float, 256> spectrumData {};              // 128 x 2 (spectrum row, waveform row)
    double bassTime = 0.0, midTime = 0.0, highTime = 0.0, levelTime = 0.0;
    float imgAspect = 1.0f, imgMask = 0.0f;
    std::uint32_t dnaVersion = 0xffffffffu;
    bool hasImage = false;
    int currentScene = -1, fadeFromScene = -1;
    float fadeAmount = 1.0f;
    int lastVsync = -1;

    // --- time / analysis ------------------------------------------------------------------
    double lastTime = 0.0, startTime = 0.0;
    double sceneTime = 0.0, templateMotion = 0.0;
    AudioUniforms au;
    float activity = 0.0f;
    bool leading = true, wasLeading = false;
    // adaptive quality controller
    float quality = 1.0f, fpsAvg = 60.0f, upHold = 0.0f, shaderQuality = 1.0f;
    float chaosAmt = 0.0f;                          // CHAOS button, smoothed 0..1
    void updateQuality(float dt);
    float flowBass = 0.0f, flowMid = 0.0f, flowHigh = 0.0f, flowLevel = 0.0f;   // smoothed motion drivers
    // TRIP: a curated psychedelic layer on the effects chain, driven by the musical state
    int fxKaleido = -1, fxTrails = -1, fxWarp = -1, fxHue = -1, fxChroma = -1;
    float tripAmount(int effectIndex, float& p2) const;                 // smoothed 'music is playing' gate (0 = frozen)
    std::uint32_t dropCount = 0;

    // Auto Pilot: phrase-based variations (deterministic per phrase, so preview and
    // fullscreen output engines vary identically)
    static constexpr int kAutoTargets = 5;  // macro A-D, colour shift
    std::array<float, kAutoTargets> autoCurrent {}, autoTarget {};
    std::int64_t autoKey = -1;
    std::uint32_t autoDropsSeen = 0, autoLastDrop = 0;
    int autoTargetIndex[kAutoTargets] {};
    MusicalClock clock;
    ColorSystem color;

    // --- modulation ---------------------------------------------------------------------------
    ModulationEngine modEngine;
    std::vector<float> modOffsets;
    ModSourceValues sources;
    std::uint32_t lastMidiCount = 0, lastMatrixVersion = 0;
    float midiEnv = 0.0f, randomStep = 0.0f;
    juce::Random random;

    // --- telemetry ---------------------------------------------------------------------------
    int frameCounter = 0;
    double fpsWindowStart = 0.0;

    // cached parameter indices
    int pScene, pIntensity, pSpeed, pMacro[4], pSensitivity, pSmoothing, pReact[4], pSyncSource, pSyncDiv,
        pInternalBpm, pPalette, pHue, pSat, pBright, pContrast, pColorAmount, pColorShift, pAudioColor,
        pCustomA, pCustomB, pTplEnable, pTplMode, pTplBlend, pTplMix, pTplMirror, pTplKaleido,
        pTplRotation, pTplMotion, pAudioDrive, pIdleMotion, pDynamics, pAutoPilot, pAutoBars, pAutoOnDrop, pBloom, pImgMode, pAutoFX, pSharpen, pMotionSmooth, pTransitionTime, pTrip, pChaos;
    std::vector<std::pair<const char*, int>> tplUniforms;   // uniform name → param index
    std::vector<int> pFxOn, pFxAmt, pFxP2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RenderEngine)
};
} // namespace dali
