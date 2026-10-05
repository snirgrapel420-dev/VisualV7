#include "VisualView.h"

namespace dali
{
VisualView::VisualView(EngineState& state, RenderEngine::Role role)
    : engine(state, context, role)
{
    setOpaque(true);
    context.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
    context.setRenderer(&engine);
    context.setComponentPaintingEnabled(false);
    context.setContinuousRepainting(true);
    context.attachTo(*this);
}

VisualView::~VisualView()
{
    context.detach();          // stops the GL thread before the engine is destroyed
}
} // namespace dali
