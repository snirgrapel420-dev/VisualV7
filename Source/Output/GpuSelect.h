#pragma once
// ============================================================================
//  GpuSelect — which graphics card renders Dali Visual.
//
//  Laptops with two GPUs (Intel UHD + NVIDIA RTX / AMD) start programs on the
//  weak integrated GPU unless told otherwise. Three layers fix that:
//   1. the Standalone .exe exports NvOptimusEnablement / AmdPowerXpressRequest-
//      HighPerformance (the drivers then pick the dedicated GPU automatically)
//   2. a plug-in runs inside the DAW, so the choice belongs to the DAW's .exe:
//      setPreference() writes exactly what Windows Settings > Display > Graphics
//      writes (per user) for the program Dali Visual is running in
//   3. the UI warns when the picture runs on an integrated GPU although a
//      dedicated one is present, and offers the switch
//  macOS: Apple Silicon has one GPU. Intel Macs with two GPUs switch to the
//  dedicated one for OpenGL (the standalone's Info.plist disables automatic
//  switching); a plug-in follows the host. JUCE-free.
// ============================================================================
#include <string>
#include <vector>

namespace dali::gpu
{
struct Adapter { std::string name; bool integrated = false; };

/** The hardware GPUs (Windows: DXGI; elsewhere: empty). */
std::vector<Adapter> adapters();
/** The first dedicated (non-integrated) GPU's name, or empty. */
std::string dedicatedName();
/** True when a renderer string (GL_RENDERER) names an integrated GPU. */
bool isIntegratedName(const std::string& name);

/** Windows only: the program the GPU preference applies to (the DAW or the standalone). */
std::string hostProgramName();
/** 0 = let Windows decide, 1 = power saving (integrated), 2 = high performance (dedicated); -1 = not available. */
int getPreference();
bool setPreference(int pref);
bool isSupported();
}
