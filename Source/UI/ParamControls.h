#pragma once
// ============================================================================
//  Parameter-bound controls. Every control is attached to a real APVTS
//  parameter (no placeholder controls). Right-click → MIDI Learn / Clear,
//  "Modulate by…", remove modulation, reset. Knobs show a live modulation
//  ring (cyan) with the effective value computed by the render engine.
// ============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"

namespace dali
{
/** Shared right-click menu for any parameter control. */
void showParamMenu(DaliVisualProcessor& p, const juce::String& paramId, juce::Component& target);

class ParamKnob : public juce::Component, private juce::Timer
{
public:
    ParamKnob(DaliVisualProcessor& p, const juce::String& paramId, const juce::String& labelText = {});
    void setLabel(const juce::String& t) { label.setText(t, juce::dontSendNotification); }
    void setTooltip(const juce::String& t) { slider.setTooltip(t + "\n\nRight-click: MIDI Learn / Modulate / Reset"); }
    void resized() override;
    juce::String getParamId() const { return paramId; }

private:
    struct KnobSlider : juce::Slider
    {
        std::function<void()> onRightClick;
        void mouseDown(const juce::MouseEvent& e) override
        {
            if (e.mods.isPopupMenu()) { if (onRightClick) onRightClick(); return; }
            juce::Slider::mouseDown(e);
        }
    };
    void timerCallback() override;

    DaliVisualProcessor& proc;
    juce::String paramId;
    int paramIndex;
    KnobSlider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    float lastMod = -2.0f;
    bool lastLearn = false;
};

class ParamToggle : public juce::ToggleButton
{
public:
    ParamToggle(DaliVisualProcessor& p, const juce::String& paramId, const juce::String& text = {});
    void mouseDown(const juce::MouseEvent& e) override;
private:
    DaliVisualProcessor& proc;
    juce::String paramId;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

class ParamCombo : public juce::ComboBox
{
public:
    ParamCombo(DaliVisualProcessor& p, const juce::String& paramId);
    void mouseDown(const juce::MouseEvent& e) override;
private:
    DaliVisualProcessor& proc;
    juce::String paramId;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

/** Small section header used by the panels. */
class SectionLabel : public juce::Label
{
public:
    explicit SectionLabel(const juce::String& t);
};

/** Lays knobs out in a grid of 'cols' columns. Returns the used height. */
int layoutKnobGrid(juce::Array<juce::Component*> items, juce::Rectangle<int> area, int cols, int cellH);
} // namespace dali
