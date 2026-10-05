#pragma once
// ============================================================================
//  RenderTarget — an RGBA16F texture + framebuffer (HDR headroom for glow,
//  feedback and trails). PingPong wraps two for stateful passes. GL thread.
// ============================================================================
#include <juce_opengl/juce_opengl.h>

namespace dali
{
class RenderTarget
{
public:
    ~RenderTarget() { release(); }
    bool ensure(int w, int h);            // (re)allocates on size change; clears to black
    void release();
    void bind() const;                    // binds FBO + viewport
    void clear() const;
    unsigned int texture() const noexcept { return tex; }
    int width() const noexcept  { return w; }
    int height() const noexcept { return h; }
    bool isValid() const noexcept { return fbo != 0; }
private:
    unsigned int tex = 0, fbo = 0;
    int w = 0, h = 0;
};

struct PingPong
{
    RenderTarget a, b;
    bool flip = false;
    RenderTarget& current()  { return flip ? b : a; }    // write
    RenderTarget& previous() { return flip ? a : b; }    // read (last frame)
    void swap() { flip = !flip; }
    bool ensure(int w, int h) { const bool r1 = a.ensure(w, h); const bool r2 = b.ensure(w, h); return r1 || r2; }
    void clear() { a.clear(); b.clear(); }
    void release() { a.release(); b.release(); }
};
} // namespace dali
