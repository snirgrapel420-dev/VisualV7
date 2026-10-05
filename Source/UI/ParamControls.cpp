#include "ParamControls.h"
#include "DaliLookAndFeel.h"

namespace dali
{
void showParamMenu(DaliVisualProcessor& p, const juce::String& paramId, juce::Component& target)
{
    auto* prm = p.apvts.getParameter(paramId);
    if (prm == nullptr) return;

    const int index = params::indexOf(paramId);
    const int modTarget = params::modTargetOf(index);
    const juce::String mapping = p.midi.describeMapping(paramId);
    const bool learning = p.midi.getLearnTarget() == paramId;

    juce::PopupMenu m;
    m.addSectionHeader(prm->getName(64));
    if (learning) m.addItem(1, "Cancel MIDI Learn");
    else          m.addItem(1, "MIDI Learn" + (mapping.isNotEmpty() ? "  (now " + mapping + ")" : juce::String()));
    m.addItem(2, "Clear MIDI mapping", mapping.isNotEmpty());

    if (modTarget >= 0)
    {
        juce::PopupMenu mod;
        for (int s = 1; s < int(ModSource::count); ++s) mod.addItem(100 + s, modSourceName(ModSource(s)));
        m.addSeparator();
        m.addSubMenu("Modulate by", mod);
        m.addItem(3, "Remove modulation", p.matrix.isParamModulated(index));
    }
    m.addSeparator();
    m.addItem(4, "Reset to default");

    juce::Component::SafePointer<juce::Component> safe(&target);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&target),
        [&p, paramId, modTarget, learning, prm, safe](int r)
        {
            if (r == 1) { if (learning) p.midi.cancelLearn(); else p.midi.startLearn(paramId); }
            else if (r == 2) p.midi.clearMapping(paramId);
            else if (r == 3)
            {
                p.pushUndo();
                auto slots = p.matrix.getSlots();
                for (auto& s : slots) if (s.target == modTarget) { s = ModSlot {}; s.source = 0; s.target = -1; }
                p.matrix.setAll(slots);
            }
            else if (r == 4) { p.pushUndo(); prm->beginChangeGesture(); prm->setValueNotifyingHost(prm->getDefaultValue()); prm->endChangeGesture(); }
            else if (r > 100)
            {
                if (p.matrix.addRoute(ModSource(r - 100), modTarget, 0.3f) < 0)
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "Modulation Matrix",
                                                           "All 16 modulation slots are in use. Remove one in the MOD tab.");
            }
            juce::ignoreUnused(safe);
        });
}

// =============================================================================
ParamKnob::ParamKnob(DaliVisualProcessor& p, const juce::String& id, const juce::String& labelText)
    : proc(p), paramId(id), paramIndex(params::indexOf(id))
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled(true, true, nullptr);
    slider.setDoubleClickReturnValue(true, 0.0);
    slider.onRightClick = [this] { showParamMenu(proc, paramId, slider); };
    addAndMakeVisible(slider);

    auto* prm = p.apvts.getParameter(id);
    label.setText(labelText.isNotEmpty() ? labelText : (prm != nullptr ? prm->getName(24) : id), juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions(11.0f)));
    label.setColour(juce::Label::textColourId, colours::textDim);
    label.setMinimumHorizontalScale(0.7f);
    addAndMakeVisible(label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, id, slider);
    setTooltip(prm != nullptr ? prm->getName(64) : id);
    if (prm != nullptr) slider.setDoubleClickReturnValue(true, prm->convertFrom0to1(prm->getDefaultValue()));
    startTimerHz(30);
}

void ParamKnob::resized()
{
    auto r = getLocalBounds();
    label.setBounds(r.removeFromBottom(16));
    slider.setBounds(r);
}

void ParamKnob::timerCallback()
{
    const float mod = (paramIndex >= 0 && paramIndex < EngineState::kMaxParams)
                    ? proc.engineState.modulated[size_t(paramIndex)].load() : -1.0f;
    const bool learn = proc.midi.getLearnTarget() == paramId;
    if (std::abs(mod - lastMod) > 0.002f || learn != lastLearn)
    {
        lastMod = mod; lastLearn = learn;
        slider.getProperties().set("mod", mod);
        slider.getProperties().set("learn", learn);
        slider.repaint();
    }
}

// =============================================================================
ParamToggle::ParamToggle(DaliVisualProcessor& p, const juce::String& id, const juce::String& text)
    : proc(p), paramId(id)
{
    auto* prm = p.apvts.getParameter(id);
    setButtonText(text.isNotEmpty() ? text : (prm != nullptr ? prm->getName(32) : id));
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, id, *this);
}

void ParamToggle::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { showParamMenu(proc, paramId, *this); return; }
    juce::ToggleButton::mouseDown(e);
}

// =============================================================================
ParamCombo::ParamCombo(DaliVisualProcessor& p, const juce::String& id) : proc(p), paramId(id)
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(p.apvts.getParameter(id)))
        addItemList(c->choices, 1);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, id, *this);
}

void ParamCombo::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { showParamMenu(proc, paramId, *this); return; }
    juce::ComboBox::mouseDown(e);
}

// =============================================================================
SectionLabel::SectionLabel(const juce::String& t)
{
    setText(t.toUpperCase(), juce::dontSendNotification);
    setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    setColour(juce::Label::textColourId, colours::accent);
}

int layoutKnobGrid(juce::Array<juce::Component*> items, juce::Rectangle<int> area, int cols, int cellH)
{
    const int cellW = area.getWidth() / juce::jmax(1, cols);
    for (int i = 0; i < items.size(); ++i)
        items[i]->setBounds(area.getX() + (i % cols) * cellW, area.getY() + (i / cols) * cellH, cellW, cellH);
    const int rows = (items.size() + cols - 1) / juce::jmax(1, cols);
    return rows * cellH;
}
} // namespace dali
