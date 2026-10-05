#pragma once
// ============================================================================
//  VisualView — a component that owns an OpenGL 3.2 core context and a
//  RenderEngine. Used for the editor preview and for the fullscreen output.
//  JUCE renders it on its own GL thread; component painting is disabled so the
//  message thread never touches the GPU pipeline.
// ============================================================================
#include <juce_opengl/juce_opengl.h>
#include "../Render/RenderEngine.h"
#include <functional>

namespace dali
{
class VisualView : public juce::Component
{
public:
    VisualView(EngineState& state, RenderEngine::Role role);
    ~VisualView() override;

    void resized() override { engine.setLogicalSize(getWidth(), getHeight()); }
    void paint(juce::Graphics&) override {}
    void mouseDoubleClick(const juce::MouseEvent&) override { if (onDoubleClick) onDoubleClick(); }

    std::function<void()> onDoubleClick;

private:
    juce::OpenGLContext context;
    RenderEngine engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualView)
};
} // namespace dali
