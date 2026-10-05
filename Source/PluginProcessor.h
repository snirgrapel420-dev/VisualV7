#pragma once
// ============================================================================
//  DALI VISUAL — audio processor (shared by VST3 and Standalone).
//
//  The audio thread does only three cheap, wait-free things:
//    1. pushes the input into the analyzer FIFO,
//    2. publishes host transport (try-lock, never blocks),
//    3. forwards MIDI into the MIDI-mapper FIFO.
//  Audio passes through untouched in the plug-in; the Standalone silences its
//  output (it is a visual instrument listening to an input — no feedback).
// ============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include "Core/EngineState.h"
#include "Core/Parameters.h"
#include "Audio/AudioAnalyzer.h"
#include "Audio/LoopbackCapture.h"
#include "Core/AutoPilot.h"
#include "Modulation/ModulationMatrix.h"
#include "Render/EffectChain.h"
#include "Image/ImageProcessor.h"
#include "Image/TemplateGenerator.h"
#include "Midi/MidiMapper.h"
#include "Output/OutputManager.h"

class DaliVisualProcessor : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener,
                            private juce::AsyncUpdater
{
public:
    DaliVisualProcessor();
    ~DaliVisualProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    /** Full instrument state. includeGlobal adds MIDI mappings + audio source (plug-in state only). */
    juce::ValueTree captureState(bool includeGlobal) const;
    void applyState(const juce::ValueTree& state);

    bool isStandalone() const noexcept { return wrapperType == wrapperType_Standalone; }

    /** Message thread. Makes the loaded image the visual itself, starting clean: scene 17,
        nothing inherited from the previous preset (effects off, image-only sound routes,
        the image's own colours). resetImageControls also resets the image controls (new image). */
    void applyImageReactorLook(bool resetImageControls);

    // ---- UNDO / REDO (message thread): a snapshot is taken before every destructive change ----------
    void pushUndo();
    bool undo();
    bool redo();
    bool canUndo() const noexcept { return !undoStack.empty(); }
    bool canRedo() const noexcept { return !redoStack.empty(); }
    juce::ChangeBroadcaster historyChanged;

    /** Message thread: loads the current scene's own init values (undoable). */
    void applySceneInit();

    // ---- audio source (standalone) ---------------------------------------------------------
    enum InputSource { AudioInput = 0, SystemAudio = 1 };
    /** Message thread. SystemAudio = what the computer plays (Windows loopback). */
    void setInputSource(int source);
    int getInputSource() const noexcept { return inputSource.load(); }
    bool canCaptureSystemAudio() const noexcept { return isStandalone() && dali::LoopbackCapture::isSupported(); }
    juce::String getInputSourceStatus() const;

    // ---- accessors for the UI ---------------------------------------------------------------
    juce::AudioProcessorValueTreeState apvts;
    dali::AudioAnalyzer     analyzer;
    dali::LoopbackCapture   loopback { analyzer };
    dali::ModulationMatrix  matrix;
    dali::EffectChain       effectChain;
    dali::ImageProcessor    image;
    dali::EngineState       engineState;
    dali::AutoPilot         autoPilot { apvts, engineState };
    dali::MidiMapper        midi;
    dali::OutputManager     output;
    dali::TemplateGenerator templates;

    static inline const juce::Identifier stateId { "DaliVisualState" };

private:
    void parameterChanged(const juce::String& id, float newValue) override;
    void handleAsyncUpdate() override;
    std::vector<juce::ValueTree> undoStack, redoStack;
    int lastSceneSeen = -1;
    int restoring = 0;
    std::atomic<float>* sensitivityParam = nullptr;
    std::atomic<int> inputSource { AudioInput };
    double sampleRate = 48000.0;
    float loadSmoothed = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DaliVisualProcessor)
};
