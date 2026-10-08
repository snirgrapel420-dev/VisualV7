#include "RecordCapture.h"
#include "../Output/VideoRecorder.h"

using namespace juce::gl;

namespace dali
{
bool RecordCapture::ensure(int w, int h)
{
    if (fboA != 0 && w == width && h == height) return true;
    release();
    width = w; height = h;

    auto makeTarget = [w, h](unsigned int& tex, unsigned int& fbo)
    {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    };
    const bool ok = makeTarget(texA, fboA) && makeTarget(texB, fboB);
    glBindTexture(GL_TEXTURE_2D, 0);

    for (auto& s : slots)
    {
        glGenBuffers(1, &s.pbo);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, s.pbo);
        glBufferData(GL_PIXEL_PACK_BUFFER, GLsizeiptr(w) * GLsizeiptr(h) * 4, nullptr, GL_STREAM_READ);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    if (!ok) release();
    return ok;
}

void RecordCapture::release()
{
    for (auto& s : slots)
    {
        if (s.fence != nullptr) glDeleteSync(s.fence);
        if (s.pbo != 0) glDeleteBuffers(1, &s.pbo);
        s = Slot {};
    }
    if (fboA != 0) glDeleteFramebuffers(1, &fboA);
    if (fboB != 0) glDeleteFramebuffers(1, &fboB);
    if (texA != 0) glDeleteTextures(1, &texA);
    if (texB != 0) glDeleteTextures(1, &texB);
    fboA = fboB = texA = texB = 0;
    width = height = next = 0;
}

void RecordCapture::readBack(VideoRecorder& rec, Slot& s)
{
    glBindBuffer(GL_PIXEL_PACK_BUFFER, s.pbo);
    const auto bytes = GLsizeiptr(width) * GLsizeiptr(height) * 4;
    if (const void* data = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, bytes, GL_MAP_READ_BIT))
    {
        if (s.generation == rec.generation())
            rec.deliverFrame(static_cast<const std::uint8_t*>(data), width, height, s.frame);
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    if (s.fence != nullptr) glDeleteSync(s.fence);
    s.fence = nullptr;
    s.pending = false;
}

void RecordCapture::collect(VideoRecorder& rec, bool waitForOldest)
{
    for (;;)
    {
        Slot* oldest = nullptr;
        for (auto& s : slots)
            if (s.pending && (oldest == nullptr || s.order < oldest->order)) oldest = &s;
        if (oldest == nullptr) return;
        // 2 ms at most when the ring is full; otherwise only frames the GPU has already finished
        const GLenum r = glClientWaitSync(oldest->fence, GL_SYNC_FLUSH_COMMANDS_BIT, waitForOldest ? GLuint64(2000000) : GLuint64(0));
        if (r == GL_TIMEOUT_EXPIRED) return;
        readBack(rec, *oldest);                // (GL_WAIT_FAILED: the data is read anyway, the slot is freed)
        waitForOldest = false;
    }
}

void RecordCapture::process(VideoRecorder& rec, int frameW, int frameH, int srcW, int srcH,
                            const std::function<void(int, int)>& drawOutput)
{
    if (!ensure(frameW, frameH)) return;
    collect(rec, false);

    std::int64_t frame = 0;
    if (!rec.frameDue(frame)) return;

    Slot& slot = slots[size_t(next)];
    if (slot.pending) collect(rec, true);
    if (slot.pending) return;                  // the GPU is far behind: skip (the writer repeats the last picture)

    // 1. the output pass at the recording size, cropped to fill the frame (aspect kept)
    glBindFramebuffer(GL_FRAMEBUFFER, fboA);
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    const double srcAspect = double(juce::jmax(1, srcW)) / double(juce::jmax(1, srcH));
    const double dstAspect = double(width) / double(height);
    int vw = width, vh = height;
    if (srcAspect > dstAspect) vw = juce::roundToInt(height * srcAspect);
    else                       vh = juce::roundToInt(width / srcAspect);
    glViewport((width - vw) / 2, (height - vh) / 2, vw, vh);
    drawOutput(vw, vh);

    // 2. flip on the GPU (video frames are top-down), then 3. start the asynchronous read-back
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fboA);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboB);
    glBlitFramebuffer(0, 0, width, height, 0, height, width, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fboB);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, slot.pbo);
    glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    slot.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    slot.frame = frame;
    slot.generation = rec.generation();
    slot.order = ++counter;
    slot.pending = slot.fence != nullptr;
    next = (next + 1) % int(slots.size());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
} // namespace dali
