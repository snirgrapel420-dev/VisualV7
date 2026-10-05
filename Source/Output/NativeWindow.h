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
}
