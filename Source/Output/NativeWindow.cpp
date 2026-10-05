#if defined(_WIN32)
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif
#include "NativeWindow.h"

namespace dali::native
{
#if defined(_WIN32)
bool fillMonitorAt(void* handle, int x, int y)
{
    HWND hwnd = static_cast<HWND>(handle);
    if (hwnd == nullptr) return false;
    POINT pt { x, y };
    HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi {};
    mi.cbSize = sizeof(mi);
    if (mon == nullptr || !GetMonitorInfoW(mon, &mi)) return false;
    const RECT r = mi.rcMonitor;                        // full monitor, including the task bar area
    return SetWindowPos(hwnd, HWND_TOPMOST, r.left, r.top, r.right - r.left, r.bottom - r.top,
                        SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_NOOWNERZORDER) != 0;
}

bool isEscapeDown()
{
    return (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
}
#else
bool fillMonitorAt(void*, int, int) { return false; }
bool isEscapeDown() { return false; }
#endif
}
