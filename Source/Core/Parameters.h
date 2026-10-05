#pragma once
// ============================================================================
//  Parameters — single, data-driven definition of every automatable
//  parameter. The order of all() is stable: it is the index space used by the
//  render engine, the modulation targets and the UI.
// ============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

namespace dali::params
{
enum class Kind { Float, Choice, Bool };

struct Def
{
    juce::String id;
    juce::String name;
    Kind  kind = Kind::Float;
    float min = 0.0f, max = 1.0f, def = 0.0f, step = 0.0f;
    bool  modulatable = false;
    juce::StringArray choices;
    juce::String group;
    juce::String label;
};

/** All parameter definitions (stable order). */
const std::vector<Def>& all();

/** Index of a parameter id in all(), or -1. */
int indexOf(const juce::String& id);

/** Modulation targets: indices into all() of every modulatable parameter. */
const std::vector<int>& modTargets();

/** Target slot of a parameter index, or -1 if it is not modulatable. */
int modTargetOf(int paramIndex);

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

// ---- frequently used ids ---------------------------------------------------
namespace id
{
    inline const juce::String scene = "scene", intensity = "intensity", speed = "speed",
        macroA = "macroA", macroB = "macroB", macroC = "macroC", macroD = "macroD",
        audioDrive = "audioDrive", idleMotion = "idleMotion", dynamics = "dynamics", bloom = "bloom", autoFX = "autoFX", motionSmooth = "motionSmooth", transitionTime = "transitionTime", tripAmount = "tripAmount", sceneInit = "sceneInit", chaosMode = "chaosMode", sharpen = "sharpen",
        autoPilot = "autoPilot", autoBars = "autoBars", autoOnDrop = "autoOnDrop",
        sensitivity = "sensitivity", smoothing = "smoothing",
        reactBass = "reactBass", reactMid = "reactMid", reactHigh = "reactHigh", reactTransient = "reactTransient",
        syncSource = "syncSource", syncDiv = "syncDiv", internalBpm = "internalBpm",
        palette = "palette", hue = "hue", saturation = "saturation", brightness = "brightness", contrast = "contrast",
        colorAmount = "colorAmount", colorShift = "colorShift", audioColor = "audioColor",
        customHueA = "customHueA", customHueB = "customHueB",
        tplEnable = "tplEnable", imgMode = "imgMode", tplMode = "tplMode", tplBlend = "tplBlend", tplMix = "tplMix";

    inline juce::String fxOn (const char* fx) { return juce::String("fx_") + fx + "_on"; }
    inline juce::String fxAmt(const char* fx) { return juce::String("fx_") + fx + "_amt"; }
    inline juce::String fxP2 (const char* fx) { return juce::String("fx_") + fx + "_p2"; }
}

/** Template parameter ids, in the order of the Image panel. */
const juce::StringArray& templateParamIds();
} // namespace dali::params
