#pragma once
// ============================================================================
//  VisualScene — one of the 8 generative scenes: its description (Library),
//  its compiled shader and its own feedback history (uPrev).
// ============================================================================
#include "Library.h"
#include "ShaderProgram.h"
#include "RenderTarget.h"

namespace dali
{
class VisualScene
{
public:
    explicit VisualScene(const SceneInfo& i) : info(i) {}

    bool compile(const juce::String& vertexSrc)
    {
        return shader.build(vertexSrc, Shader::assembleFragment(Shader::resource(info.resource)), info.name);
    }

    const SceneInfo& info;
    Shader shader;
    PingPong history;       // own frame history for feedback-based scenes
};
} // namespace dali
