#pragma once
// ============================================================================
//  FrameSink — hook for sharing the final frame with other applications.
//  A Spout (Windows) / Syphon (macOS) / NDI adapter implements this interface
//  and registers itself with OutputManager::addFrameSink().
//  Called on the GL thread of the engine that renders the *output* (the
//  fullscreen engine when open, otherwise the preview), right after the final
//  graded frame is composed. Texture: GL_TEXTURE_2D, RGBA16F, bottom-up.
// ============================================================================
namespace dali
{
class FrameSink
{
public:
    virtual ~FrameSink() = default;
    virtual void publishFrame(unsigned int glTextureId, int width, int height) = 0;
    /** The GL context that published frames is about to be destroyed. */
    virtual void contextClosing() {}
};
} // namespace dali
