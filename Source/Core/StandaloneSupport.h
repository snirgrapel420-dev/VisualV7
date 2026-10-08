#pragma once
// ============================================================================
//  Standalone-only helpers (no-ops inside a plug-in).
// ============================================================================
namespace dali::standalone
{
/** JUCE's standalone wrapper mutes the audio INPUT by default ("to avoid a feedback loop"),
    so the visuals never heard anything (notably on macOS, where there is no system-audio
    capture to fall back on). Dali Visual never plays its input back (its output is silent),
    so there is no feedback risk: open the input. Returns false when not running standalone
    or the wrapper is not ready yet. Message thread. */
bool unmuteInput();
}
