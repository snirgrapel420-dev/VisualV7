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

bool clientSize(void* handle, int& w, int& h)
{
    RECT r {};
    if (handle == nullptr || !GetClientRect(static_cast<HWND>(handle), &r)) return false;
    w = r.right - r.left; h = r.bottom - r.top;
    return w > 0 && h > 0;
}

struct ChildFind { HWND parent; HWND found; };
static BOOL CALLBACK firstChildProc(HWND child, LPARAM lp)
{
    auto* c = reinterpret_cast<ChildFind*>(lp);
    if (GetParent(child) == c->parent) { c->found = child; return FALSE; }
    return TRUE;
}

bool glSurfaceSize(void* handle, int& w, int& h)
{
    ChildFind c { static_cast<HWND>(handle), nullptr };
    if (c.parent == nullptr) return false;
    EnumChildWindows(c.parent, firstChildProc, reinterpret_cast<LPARAM>(&c));
    RECT r {};
    if (c.found == nullptr || !GetClientRect(c.found, &r)) return false;
    w = r.right - r.left; h = r.bottom - r.top;
    return w > 0 && h > 0;
}

bool currentDrawableSize(int& w, int& h)
{
    HDC dc = wglGetCurrentDC();
    if (dc == nullptr) return false;
    HWND hwnd = WindowFromDC(dc);
    RECT r {};
    if (hwnd == nullptr || !GetClientRect(hwnd, &r)) return false;
    w = r.right - r.left; h = r.bottom - r.top;
    return w > 0 && h > 0;
}

struct FillCtx { HWND parent; int w, h; };
static BOOL CALLBACK fillChildProc(HWND child, LPARAM lp)
{
    const auto* c = reinterpret_cast<const FillCtx*>(lp);
    if (GetParent(child) != c->parent) return TRUE;                   // direct children only (the GL surface)
    RECT r {};
    GetWindowRect(child, &r);
    POINT tl { r.left, r.top };
    ScreenToClient(c->parent, &tl);
    if (tl.x != 0 || tl.y != 0 || r.right - r.left != c->w || r.bottom - r.top != c->h)
        SetWindowPos(child, nullptr, 0, 0, c->w, c->h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    return TRUE;
}

void fillChildren(void* handle)
{
    HWND hwnd = static_cast<HWND>(handle);
    RECT r {};
    if (hwnd == nullptr || !GetClientRect(hwnd, &r)) return;
    FillCtx ctx { hwnd, r.right - r.left, r.bottom - r.top };
    EnumChildWindows(hwnd, fillChildProc, reinterpret_cast<LPARAM>(&ctx));
}

bool coversMonitorAt(void* handle, int x, int y, int& mw, int& mh)
{
    HWND hwnd = static_cast<HWND>(handle);
    if (hwnd == nullptr) return false;
    POINT pt { x, y };
    MONITORINFO mi {}; mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST), &mi)) return false;
    mw = mi.rcMonitor.right - mi.rcMonitor.left; mh = mi.rcMonitor.bottom - mi.rcMonitor.top;
    RECT w {};
    if (!GetWindowRect(hwnd, &w)) return false;
    return w.left == mi.rcMonitor.left && w.top == mi.rcMonitor.top && w.right == mi.rcMonitor.right && w.bottom == mi.rcMonitor.bottom;
}
#else
bool fillMonitorAt(void*, int, int) { return false; }
bool isEscapeDown() { return false; }
bool currentDrawableSize(int&, int&) { return false; }
bool clientSize(void*, int&, int&) { return false; }
bool glSurfaceSize(void*, int&, int&) { return false; }
void fillChildren(void*) {}
bool coversMonitorAt(void*, int, int, int& mw, int& mh) { mw = mh = 0; return true; }
#endif
}
