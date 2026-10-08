#pragma once
// ============================================================================
//  Native helpers for the fullscreen output window.
//   fillMonitorAt   size a window to the exact physical bounds of the monitor
//                   that contains a physical screen point (immune to mixed-DPI
//                   setups, e.g. a 150 % laptop panel next to a 100 % monitor)
//   isEscapeDown    reads the ESC key from the OS even when another window (the
//                   DAW) has the keyboard focus
//  Windows: Win32. Other platforms: no-op / false (JUCE handles them).
// ============================================================================
namespace dali::native
{
bool fillMonitorAt(void* nativeWindowHandle, int physicalX, int physicalY);
bool isEscapeDown();
bool clientSize(void* nativeWindowHandle, int& w, int& h);
bool glSurfaceSize(void* nativeWindowHandle, int& w, int& h);   // first direct child (JUCE's GL surface)
/** GL thread, context current: the true pixel size of the surface being drawn into (Windows). */
bool currentDrawableSize(int& width, int& height);
/** Stretch every child window (the OpenGL surface) over the parent's full client area. */
void fillChildren(void* nativeWindowHandle);
/** True when the window exactly covers the monitor that contains the physical point. */
bool coversMonitorAt(void* nativeWindowHandle, int physicalX, int physicalY, int& monitorW, int& monitorH);

/** While alive, the calling thread is Per-Monitor-DPI-aware (v2) - real physical pixels on every monitor.
    Needed inside a plug-in: the host decides the process DPI mode and JUCE does not change it, so the
    monitor geometry JUCE reports there is not usable for placing a window on a second display. */
class ScopedPerMonitorDpi
{
public:
    ScopedPerMonitorDpi();
    ~ScopedPerMonitorDpi();
private:
    [[maybe_unused]] void* previous = nullptr;
    ScopedPerMonitorDpi(const ScopedPerMonitorDpi&) = delete;
    ScopedPerMonitorDpi& operator=(const ScopedPerMonitorDpi&) = delete;
};

/** Monitors in the same order as juce::Desktop::getDisplays() (OS order, the primary swapped to index 0). */
int monitorCount();
/** Covers monitor #index (physical bounds, top-most). Works the same in a plug-in and in the standalone. */
bool fillMonitorIndex(void* nativeWindowHandle, int index);
bool coversMonitorIndex(void* nativeWindowHandle, int index, int& monitorW, int& monitorH);
}
