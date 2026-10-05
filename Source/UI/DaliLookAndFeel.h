#pragma once
// ============================================================================
//  DaliLookAndFeel — dark, minimal, neon-purple.
// ============================================================================
#include <juce_gui_basics/juce_gui_basics.h>

namespace dali
{
namespace colours
{
    const juce::Colour bg        { 0xff09080d };
    const juce::Colour panel     { 0xff121019 };
    const juce::Colour panel2    { 0xff1a1724 };
    const juce::Colour outline   { 0xff2a2638 };
    const juce::Colour accent    { 0xffb24dff };   // neon purple
    const juce::Colour accent2   { 0xff6f5cff };   // indigo
    const juce::Colour modRing   { 0xff3ee6ff };   // modulation (cyan)
    const juce::Colour learn     { 0xffffb03a };   // MIDI learn (amber)
    const juce::Colour text      { 0xffe9e5f2 };
    const juce::Colour textDim   { 0xff8b85a0 };
}

class DaliLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DaliLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawComboBox(juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void drawTabButton(juce::TabBarButton&, juce::Graphics&, bool over, bool down) override;
    void drawTabAreaBehindFrontButton(juce::TabbedButtonBar&, juce::Graphics&, int, int) override {}
    int  getTabButtonBestWidth(juce::TabBarButton&, int tabDepth) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override { return juce::Font(juce::FontOptions(13.0f)); }
    juce::Font getPopupMenuFont() override               { return juce::Font(juce::FontOptions(13.5f)); }
    juce::Font getTextButtonFont(juce::TextButton&, int) override { return juce::Font(juce::FontOptions(12.5f, juce::Font::bold)); }
};
} // namespace dali
