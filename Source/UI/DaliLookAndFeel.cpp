#include "DaliLookAndFeel.h"

namespace dali
{
DaliLookAndFeel::DaliLookAndFeel()
{
    using namespace colours;
    setColour(juce::ResizableWindow::backgroundColourId, bg);
    setColour(juce::Label::textColourId, text);
    setColour(juce::Slider::textBoxTextColourId, text);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::TextButton::buttonColourId, panel2);
    setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(0.85f));
    setColour(juce::TextButton::textColourOffId, text);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::ComboBox::backgroundColourId, panel2);
    setColour(juce::ComboBox::textColourId, text);
    setColour(juce::ComboBox::outlineColourId, outline);
    setColour(juce::ComboBox::arrowColourId, accent);
    setColour(juce::PopupMenu::backgroundColourId, panel);
    setColour(juce::PopupMenu::textColourId, text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha(0.35f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::PopupMenu::headerTextColourId, accent);
    setColour(juce::ToggleButton::textColourId, text);
    setColour(juce::TextEditor::backgroundColourId, panel2);
    setColour(juce::TextEditor::textColourId, text);
    setColour(juce::TextEditor::outlineColourId, outline);
    setColour(juce::TextEditor::focusedOutlineColourId, accent);
    setColour(juce::ScrollBar::thumbColourId, accent.withAlpha(0.45f));
    setColour(juce::AlertWindow::backgroundColourId, panel);
    setColour(juce::AlertWindow::textColourId, text);
    setColour(juce::TabbedComponent::backgroundColourId, panel);
    setColour(juce::TabbedComponent::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TabbedButtonBar::tabOutlineColourId, juce::Colours::transparentBlack);
}

void DaliLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float start, float end, juce::Slider& s)
{
    using namespace colours;
    const auto bounds = juce::Rectangle<int>(x, y, w, h).toFloat().reduced(4.0f);
    const float r = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float lw = juce::jmax(2.0f, r * 0.16f);
    const float arcR = r - lw * 0.5f;
    const bool learning = (bool) s.getProperties()["learn"];

    juce::Path track;
    track.addCentredArc(c.x, c.y, arcR, arcR, 0.0f, start, end, true);
    g.setColour(outline);
    g.strokePath(track, juce::PathStrokeType(lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float angle = start + pos * (end - start);
    juce::Path val;
    val.addCentredArc(c.x, c.y, arcR, arcR, 0.0f, start, angle, true);
    g.setColour(learning ? learn : (s.isEnabled() ? accent : textDim));
    g.strokePath(val, juce::PathStrokeType(lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // modulation ring: shows the live modulated value
    const float mod = s.getProperties().getWithDefault("mod", -1.0f);
    if (mod >= 0.0f)
    {
        const float mr = arcR - lw * 1.4f;
        const float ma = start + juce::jlimit(0.0f, 1.0f, mod) * (end - start);
        juce::Path m;
        m.addCentredArc(c.x, c.y, mr, mr, 0.0f, juce::jmin(angle, ma), juce::jmax(angle, ma), true);
        g.setColour(modRing);
        g.strokePath(m, juce::PathStrokeType(lw * 0.55f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.fillEllipse(juce::Rectangle<float>(4.0f, 4.0f).withCentre(c.getPointOnCircumference(mr, ma)));
    }

    g.setColour(panel2);
    g.fillEllipse(juce::Rectangle<float>(r * 1.0f, r * 1.0f).withCentre(c));
    g.setColour(text);
    const auto tip = c.getPointOnCircumference(r * 0.42f, angle);
    g.drawLine(juce::Line<float>(c.getPointOnCircumference(r * 0.12f, angle), tip), 2.0f);
}

void DaliLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                       juce::Slider::SliderStyle style, juce::Slider& s)
{
    using namespace colours;
    if (style != juce::Slider::LinearHorizontal && style != juce::Slider::LinearBar)
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, pos, 0, 0, style, s);
        return;
    }
    auto r = juce::Rectangle<int>(x, y, w, h).toFloat().reduced(0.0f, h * 0.3f);
    g.setColour(outline);
    g.fillRoundedRectangle(r, r.getHeight() * 0.5f);

    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float zeroX = bipolar ? float(s.getPositionOfValue(0.0)) : r.getX();
    auto fill = juce::Rectangle<float>(juce::jmin(zeroX, pos), r.getY(), std::abs(pos - zeroX), r.getHeight());
    g.setColour(s.isEnabled() ? accent : textDim);
    g.fillRoundedRectangle(fill, r.getHeight() * 0.5f);
    g.setColour(text);
    g.fillEllipse(juce::Rectangle<float>(r.getHeight() + 4.0f, r.getHeight() + 4.0f).withCentre({ pos, r.getCentreY() }));
}

void DaliLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    using namespace colours;
    auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    const bool on = b.getToggleState();
    const juce::Colour onColour = b.findColour(juce::TextButton::buttonOnColourId);
    g.setColour(on ? onColour : panel2.brighter(over ? 0.12f : 0.0f).darker(down ? 0.2f : 0.0f));
    g.fillRoundedRectangle(r, 5.0f);
    g.setColour(on ? onColour.brighter(0.3f) : outline);
    g.drawRoundedRectangle(r, 5.0f, 1.0f);
}

void DaliLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    using namespace colours;
    auto r = b.getLocalBounds().toFloat();
    const float h = juce::jmin(16.0f, r.getHeight() - 4.0f), w = h * 1.8f;
    auto sw = juce::Rectangle<float>(r.getX() + 2.0f, r.getCentreY() - h * 0.5f, w, h);
    const bool on = b.getToggleState();
    g.setColour(on ? accent : outline.brighter(over ? 0.2f : 0.0f));
    g.fillRoundedRectangle(sw, h * 0.5f);
    g.setColour(on ? juce::Colours::white : textDim);
    g.fillEllipse(juce::Rectangle<float>(h - 4.0f, h - 4.0f).withCentre({ on ? sw.getRight() - h * 0.5f : sw.getX() + h * 0.5f, sw.getCentreY() }));
    g.setColour(b.isEnabled() ? text : textDim);
    g.setFont(juce::Font(juce::FontOptions(12.5f)));
    g.drawFittedText(b.getButtonText(), juce::Rectangle<int>(int(sw.getRight()) + 6, 0, b.getWidth() - int(sw.getRight()) - 6, b.getHeight()),
                     juce::Justification::centredLeft, 1);
}

void DaliLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    using namespace colours;
    auto r = juce::Rectangle<int>(0, 0, w, h).toFloat().reduced(0.5f);
    g.setColour(panel2);
    g.fillRoundedRectangle(r, 5.0f);
    g.setColour(box.hasKeyboardFocus(true) ? accent : outline);
    g.drawRoundedRectangle(r, 5.0f, 1.0f);
    juce::Path arrow;
    const float ax = float(w) - 14.0f, ay = float(h) * 0.5f;
    arrow.addTriangle(ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour(accent);
    g.fillPath(arrow);
}

void DaliLookAndFeel::drawTabButton(juce::TabBarButton& b, juce::Graphics& g, bool over, bool)
{
    using namespace colours;
    auto r = b.getLocalBounds().toFloat();
    const bool front = b.isFrontTab();
    g.setColour(front ? panel2 : (over ? panel2.darker(0.2f) : panel));
    g.fillRect(r);
    if (front) { g.setColour(accent); g.fillRect(r.removeFromBottom(2.0f)); }
    g.setColour(front ? juce::Colours::white : textDim);
    g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
    g.drawFittedText(b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, 1);
}

int DaliLookAndFeel::getTabButtonBestWidth(juce::TabBarButton& b, int)
{
    const int n = juce::jmax(1, b.getTabbedButtonBar().getNumTabs());
    return b.getTabbedButtonBar().getWidth() / n;
}
} // namespace dali
