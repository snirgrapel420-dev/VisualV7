#include "GpuSelect.h"
#include <algorithm>
#include <cctype>

#if defined(_WIN32)
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <dxgi.h>
 #pragma comment(lib, "dxgi.lib")
 #pragma comment(lib, "advapi32.lib")

// Layer 1: ask the NVIDIA / AMD drivers for the dedicated GPU. Only honoured when exported from the
// .exe (the Standalone); inside a plug-in DLL they are ignored, which is harmless.
extern "C"
{
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace dali::gpu
{
static std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return s;
}

bool isIntegratedName(const std::string& n)
{
    const auto s = lower(n);
    if (s.find(" arc") != std::string::npos) return false;                     // Intel Arc: a dedicated GPU
    return s.find("intel") != std::string::npos || s.find("uhd") != std::string::npos || s.find("iris") != std::string::npos
        || s.find("radeon(tm) graphics") != std::string::npos || s.find("radeon graphics") != std::string::npos
        || s.find("vega 8") != std::string::npos || s.find("microsoft basic") != std::string::npos
        || s.find("gdi generic") != std::string::npos || s.find("llvmpipe") != std::string::npos;
}

#if defined(_WIN32)
static std::string narrow(const wchar_t* w)
{
    const int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    std::string s(size_t(n > 1 ? n - 1 : 0), '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, nullptr, nullptr);
    return s;
}

std::vector<Adapter> adapters()
{
    std::vector<Adapter> out;
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory))) || factory == nullptr) return out;
    IDXGIAdapter1* a = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &a) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 d {};
        if (SUCCEEDED(a->GetDesc1(&d)) && (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
        {
            Adapter ad;
            ad.name = narrow(d.Description);
            // integrated GPUs share system memory and have little or no dedicated VRAM
            ad.integrated = isIntegratedName(ad.name) || d.DedicatedVideoMemory < (512ull << 20);
            const bool dup = std::any_of(out.begin(), out.end(), [&](const Adapter& x) { return x.name == ad.name; });
            if (!dup) out.push_back(ad);
        }
        a->Release();
    }
    factory->Release();
    return out;
}

static std::wstring exePath()
{
    wchar_t buf[32768];
    const DWORD n = GetModuleFileNameW(nullptr, buf, DWORD(sizeof(buf) / sizeof(buf[0])));
    return std::wstring(buf, n);
}

std::string hostProgramName()
{
    const auto p = exePath();
    const auto slash = p.find_last_of(L"\\/");
    return narrow((slash == std::wstring::npos ? p : p.substr(slash + 1)).c_str());
}

static const wchar_t* kKey = L"Software\\Microsoft\\DirectX\\UserGpuPreferences";

int getPreference()
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) return 0;
    wchar_t data[256] = {};
    DWORD size = sizeof(data), type = 0;
    const auto path = exePath();
    int pref = 0;
    if (RegQueryValueExW(key, path.c_str(), nullptr, &type, reinterpret_cast<BYTE*>(data), &size) == ERROR_SUCCESS && type == REG_SZ)
    {
        const std::wstring v(data);
        const auto pos = v.find(L"GpuPreference=");
        if (pos != std::wstring::npos) pref = _wtoi(v.c_str() + pos + 14);
    }
    RegCloseKey(key);
    return pref >= 0 && pref <= 2 ? pref : 0;
}

bool setPreference(int pref)
{
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kKey, 0, nullptr, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        return false;
    const auto path = exePath();
    bool ok;
    if (pref <= 0)
        ok = RegDeleteValueW(key, path.c_str()) == ERROR_SUCCESS || getPreference() == 0;
    else
    {
        const std::wstring v = L"GpuPreference=" + std::to_wstring(pref > 2 ? 2 : pref) + L";";
        ok = RegSetValueExW(key, path.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(v.c_str()),
                            DWORD((v.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
    }
    RegCloseKey(key);
    return ok;
}

bool isSupported() { return true; }
#else
std::vector<Adapter> adapters() { return {}; }
std::string hostProgramName() { return {}; }
int getPreference() { return -1; }
bool setPreference(int) { return false; }
bool isSupported() { return false; }
#endif

std::string dedicatedName()
{
    for (auto& a : adapters())
        if (!a.integrated) return a.name;
    return {};
}
} // namespace dali::gpu
