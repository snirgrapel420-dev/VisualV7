#pragma once
// ============================================================================
//  RecordCapture — GL side of the video recorder (one per RenderEngine).
//  Renders the output pass a second time straight into an RGBA8 frame of the
//  recording size (full quality, independent of the window), flips it on the
//  GPU, and reads it back through three pixel-buffer objects with fences, so
//  the render thread never waits for the transfer.
// ============================================================================
#include <juce_opengl/juce_opengl.h>
#include <array>
#include <cstdint>
#include <functional>

namespace dali
{
class VideoRecorder;

class RecordCapture
{
public:
    /** GL thread, while this engine publishes and the recorder wants frames.
        drawOutput(vw, vh) draws the output pass into the bound framebuffer / viewport.
        srcW x srcH: the aspect of the image being drawn (cropped to fill the frame). */
    void process(VideoRecorder& rec, int frameW, int frameH, int srcW, int srcH,
                 const std::function<void(int, int)>& drawOutput,
                 const std::function<void(int, int)>& drawOverlay = {});   // over the whole frame (watermark)
    /** GL thread: frees everything (context closing, or recording over). */
    void release();
    bool isAllocated() const noexcept { return fboA != 0; }

private:
    struct Slot
    {
        unsigned int pbo = 0;
        GLsync fence = nullptr;
        std::int64_t frame = -1;
        std::uint64_t order = 0;
        int generation = -1;
        bool pending = false;
    };

    bool ensure(int w, int h);
    void collect(VideoRecorder& rec, bool waitForOldest);
    void readBack(VideoRecorder& rec, Slot& s);

    unsigned int texA = 0, fboA = 0, texB = 0, fboB = 0;
    int width = 0, height = 0, next = 0;
    std::uint64_t counter = 0;
    std::array<Slot, 3> slots;
};
} // namespace dali