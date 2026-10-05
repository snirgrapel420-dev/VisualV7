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
}
