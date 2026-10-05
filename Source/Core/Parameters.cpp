#include "Parameters.h"
#include "../Audio/MusicalClock.h"
#include "../Render/Library.h"
#include "../Render/Palettes.h"
#include <map>

namespace dali::params
{
namespace
{
juce::StringArray toArray(const char* const* names, int n)
{
    juce::StringArray a; for (int i = 0; i < n; ++i) a.add(names[i]); return a;
}

std::vector<Def> build()
{
    std::vector<Def> d;
    auto F = [&](const juce::String& id, const juce::String& name, float mn, float mx, float df, bool mod,
                 const juce::String& group, const juce::String& label = {}, float step = 0.0f)
    { Def x; x.id = id; x.name = name; x.kind = Kind::Float; x.min = mn; x.max = mx; x.def = df; x.step = step;
      x.modulatable = mod; x.group = group; x.label = label; d.push_back(x); };
    auto C = [&](const juce::String& id, const juce::String& name, juce::StringArray ch, int df, const juce::String& group)
    { Def x; x.id = id; x.name = name; x.kind = Kind::Choice; x.min = 0; x.max = float(ch.size() - 1); x.def = float(df);
      x.step = 1; x.choices = ch; x.group = group; d.push_back(x); };
    auto B = [&](const juce::String& id, const juce::String& name, bool df, const juce::String& group)
    { Def x; x.id = id; x.name = name; x.kind = Kind::Bool; x.min = 0; x.max = 1; x.def = df ? 1.0f : 0.0f; x.step = 1;
      x.group = group; d.push_back(x); };

    // --- scene -------------------------------------------------------------
    juce::StringArray sceneNames;
    for (auto& s : sceneLibrary()) sceneNames.add(s.name);
    C(id::scene, "Scene", sceneNames, 0, "Scene");
    F(id::intensity, "Intensity", 0.0f, 1.0f, 0.75f, true, "Scene");
    F(id::speed, "Motion", 0.0f, 3.0f, 1.0f, true, "Scene", "x");
    F(id::macroA, "Macro A", 0.0f, 1.0f, 0.5f, true, "Scene");
    F(id::macroB, "Macro B", 0.0f, 1.0f, 0.5f, true, "Scene");
    F(id::macroC, "Macro C", 0.0f, 1.0f, 0.5f, true, "Scene");
    F(id::macroD, "Macro D", 0.0f, 1.0f, 0.5f, true, "Scene");
    F(id::audioDrive, "Audio Drive", 0.0f, 1.0f, 0.7f, true, "Scene");
    F(id::idleMotion, "Idle Motion", 0.0f, 1.0f, 0.0f, false, "Scene");
    F(id::dynamics, "Musical Dynamics", 0.0f, 1.0f, 0.6f, true, "Scene");
    F(id::bloom, "Bloom", 0.0f, 1.0f, 0.45f, true, "Color");
    F(id::autoFX, "Auto FX", 0.0f, 1.0f, 1.0f, true, "Scene");
    F(id::motionSmooth, "Motion Smoothness", 0.0f, 1.0f, 0.6f, false, "Scene");
    F(id::transitionTime, "Transition Time", 0.3f, 8.0f, 3.0f, false, "Scene");
    F(id::tripAmount, "Trip", 0.0f, 1.0f, 0.5f, true, "Scene");
    F(id::sharpen, "Sharpness", 0.0f, 1.0f, 0.35f, false, "Color");
    C(id::autoPilot, "Auto Pilot", { "Off", "Variations", "Variations + Scenes" }, 0, "Scene");
    C(id::autoBars, "Auto Pilot Every", { "2 bars", "4 bars", "8 bars", "16 bars", "32 bars" }, 2, "Scene");
    B(id::autoOnDrop, "Auto Pilot On Drop", true, "Scene");
    B(id::sceneInit, "Load Scene Init", true, "Scene");
    B(id::chaosMode, "CHAOS", false, "Scene");

    // --- audio reaction ------------------------------------------------------
    F(id::sensitivity, "Sensitivity", 0.0f, 2.0f, 1.0f, false, "Audio", "x");
    F(id::smoothing, "Smoothing", 0.0f, 1.0f, 0.3f, false, "Audio");
    F(id::reactBass, "Bass Reaction", 0.0f, 2.0f, 1.0f, true, "Audio", "x");
    F(id::reactMid, "Mid Reaction", 0.0f, 2.0f, 1.0f, true, "Audio", "x");
    F(id::reactHigh, "High Reaction", 0.0f, 2.0f, 1.0f, true, "Audio", "x");
    F(id::reactTransient, "Transient Reaction", 0.0f, 2.0f, 1.0f, true, "Audio", "x");
    C(id::syncSource, "Sync Source", { "Auto", "Host", "Detect", "Internal" }, 0, "Audio");
    C(id::syncDiv, "Sync Division", toArray(syncDivisionNames(), 11), 2, "Audio");
    F(id::internalBpm, "Internal BPM", 60.0f, 200.0f, 140.0f, false, "Audio", "BPM", 0.1f);

    // --- colour --------------------------------------------------------------
    C(id::palette, "Palette", toArray(paletteNames(), int(PaletteId::count)), int(PaletteId::DeepPurple), "Color");
    F(id::hue, "Hue", 0.0f, 1.0f, 0.0f, true, "Color");
    F(id::saturation, "Saturation", 0.0f, 2.0f, 1.0f, true, "Color", "x");
    F(id::brightness, "Brightness", 0.0f, 2.0f, 1.0f, true, "Color", "x");
    F(id::contrast, "Contrast", 0.0f, 2.0f, 1.0f, true, "Color", "x");
    F(id::colorAmount, "Color Amount", 0.0f, 1.0f, 1.0f, true, "Color");
    F(id::colorShift, "Colour Family", 0.0f, 1.0f, 0.0f, true, "Color");
    F(id::audioColor, "Music to Colour", 0.0f, 1.0f, 0.5f, true, "Color");
    F(id::customHueA, "Custom Base Hue", 0.0f, 1.0f, 0.75f, false, "Color");
    F(id::customHueB, "Custom Highlight Hue", 0.0f, 1.0f, 0.52f, false, "Color");

    // --- image template --------------------------------------------------------
    C(id::imgMode, "Image Visual Mode", { "Kaleidoscope", "Liquid", "Tunnel", "Spectral Slices", "Droste",
                                          "Glitch", "Depth 3D", "Neon Outline", "Pulse", "Flow Paint", "Flow Lines" }, 10, "Template");
    B(id::tplEnable, "Image Overlay On Scene", false, "Template");
    C(id::tplMode, "Template Mode", toArray(templateModeNames(), kNumTemplateModes), 0, "Template");
    C(id::tplBlend, "Template Blend", toArray(templateBlendNames(), kNumTemplateBlends), 0, "Template");
    F(id::tplMix, "Template Mix", 0.0f, 1.0f, 0.85f, true, "Template");
    F("tplScale", "Template Scale", 0.1f, 3.0f, 1.0f, true, "Template", "x");
    F("tplRotation", "Rotation", 0.0f, 1.0f, 0.0f, true, "Template");
    F("tplMotion", "Motion", 0.0f, 1.0f, 0.25f, true, "Template");
    F("tplSymmetry", "Symmetry", 0.0f, 1.0f, 1.0f, true, "Template");
    F("tplSymCount", "Symmetry Count", 1.0f, 16.0f, 6.0f, true, "Template", {}, 1.0f);
    B("tplMirror", "Mirror", false, "Template");
    B("tplKaleido", "Kaleidoscope", true, "Template");
    F("tplWarp", "Warp", 0.0f, 1.0f, 0.15f, true, "Template");
    F("tplTwist", "Twist", -1.0f, 1.0f, 0.0f, true, "Template");
    F("tplNoise", "Noise", 0.0f, 1.0f, 0.0f, true, "Template");
    F("tplDistortion", "Distortion", 0.0f, 1.0f, 0.1f, true, "Template");
    F("tplFeedback", "Feedback", 0.0f, 0.98f, 0.45f, true, "Template");
    F("tplRecursion", "Recursion", 0.0f, 1.0f, 0.3f, true, "Template");
    F("tplEdge", "Edge Detection", 0.0f, 1.0f, 0.3f, true, "Template");
    F("tplThreshold", "Threshold", 0.0f, 1.0f, 0.5f, true, "Template");
    F("tplLuminance", "Luminance", 0.0f, 1.0f, 0.0f, true, "Template");
    F("tplColorExtract", "Color Extraction", 0.0f, 1.0f, 0.35f, true, "Template");
    F("tplColorAmount", "Color Amount", 0.0f, 1.0f, 1.0f, true, "Template");
    F("tplDetail", "Detail", 0.0f, 1.0f, 0.6f, true, "Template");
    F("tplComplexity", "Complexity", 0.0f, 1.0f, 0.4f, true, "Template");
    F("tplDepth", "Depth", 0.0f, 1.0f, 0.5f, true, "Template");
    F("tplReact", "Audio Reactivity", 0.0f, 2.0f, 1.0f, true, "Template", "x");

    // --- effects ---------------------------------------------------------------
    for (auto& e : effectLibrary())
    {
        B(id::fxOn(e.id), juce::String(e.name) + " On", false, "FX");
        F(id::fxAmt(e.id), juce::String(e.name) + " " + e.p1Name, 0.0f, 1.0f, e.defAmt, true, "FX");
        F(id::fxP2(e.id), juce::String(e.name) + " " + e.p2Name, 0.0f, 1.0f, e.defP2, true, "FX");
    }
    return d;
}
} // namespace

const std::vector<Def>& all()
{
    static const std::vector<Def> defs = build();
    return defs;
}

int indexOf(const juce::String& pid)
{
    static const std::map<juce::String, int> map = [] {
        std::map<juce::String, int> m;
        for (int i = 0; i < int(all().size()); ++i) m[all()[size_t(i)].id] = i;
        return m;
    }();
    auto it = map.find(pid);
    return it == map.end() ? -1 : it->second;
}

const std::vector<int>& modTargets()
{
    static const std::vector<int> t = [] {
        std::vector<int> v;
        for (int i = 0; i < int(all().size()); ++i) if (all()[size_t(i)].modulatable) v.push_back(i);
        return v;
    }();
    return t;
}

int modTargetOf(int paramIndex)
{
    static const std::vector<int> inverse = [] {
        std::vector<int> v(all().size(), -1);
        for (int t = 0; t < int(modTargets().size()); ++t) v[size_t(modTargets()[size_t(t)])] = t;
        return v;
    }();
    return (paramIndex >= 0 && paramIndex < int(inverse.size())) ? inverse[size_t(paramIndex)] : -1;
}

const juce::StringArray& templateParamIds()
{
    static const juce::StringArray ids { "tplMix", "tplScale", "tplRotation", "tplMotion", "tplSymmetry", "tplSymCount",
        "tplWarp", "tplTwist", "tplNoise", "tplDistortion", "tplFeedback", "tplRecursion", "tplEdge", "tplThreshold",
        "tplLuminance", "tplColorExtract", "tplColorAmount", "tplDetail", "tplComplexity", "tplDepth", "tplReact" };
    return ids;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& d : all())
    {
        const juce::ParameterID pid { d.id, 1 };
        switch (d.kind)
        {
            case Kind::Float:
                layout.add(std::make_unique<juce::AudioParameterFloat>(pid, d.name,
                    juce::NormalisableRange<float>(d.min, d.max, d.step), d.def,
                    juce::AudioParameterFloatAttributes().withLabel(d.label)));
                break;
            case Kind::Choice:
                layout.add(std::make_unique<juce::AudioParameterChoice>(pid, d.name, d.choices, int(d.def)));
                break;
            case Kind::Bool:
                layout.add(std::make_unique<juce::AudioParameterBool>(pid, d.name, d.def > 0.5f));
                break;
        }
    }
    return layout;
}
} // namespace dali::params
