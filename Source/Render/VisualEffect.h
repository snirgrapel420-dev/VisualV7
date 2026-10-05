#pragma once
// ============================================================================
//  VisualEffect — one GPU effect of the rack. Stateless effects read the
//  chain input; stateful ones (Feedback, Trails) also own a history buffer
//  which *is* their output, so their persistence survives reordering.
// ============================================================================
#include "Library.h"
#include "ShaderProgram.h"
#include "RenderTarget.h"

namespace dali
{
class VisualEffect
{
public:
    explicit VisualEffect(const EffectInfo& i) : info(i) {}

    bool compile(const juce::String& vertexSrc)
    {
        return shader.build(vertexSrc, Shader::assembleFragment(Shader::resource(info.resource)), info.name);
    }

    const EffectInfo& info;
    Shader shader;
    PingPong history;        // used only when info.stateful
    bool wasActive = false;  // clears history when re-enabled
};
} // namespace dali
