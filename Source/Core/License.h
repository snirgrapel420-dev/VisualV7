#pragma once
// ============================================================================
//  License — three modes:
//    FULL   a serial number was activated (offline, fixed codes: LicenseKeys.h)
//    TRIAL  7 days of the full feature set, started once by the user from the
//           launch dialog; the DALI AUDIO watermark still appears
//    DEMO   no serial and no running trial: watermark, scenes 01-05, GO LIVE and
//           your own image locked, REC limited to 30 s (watermark burned in)
//
//  The serial and the trial record live in the per-user settings (shared by the
//  plug-in and the standalone). The trial record is signed and kept in two
//  places; a record that was edited, or a clock set back, ends the trial.
//  Change what the demo allows in License.cpp only.
// ============================================================================
#include <juce_core/juce_core.h>

namespace dali
{
enum class Feature { Scene, LiveOutput, Recording, Image };

struct License
{
    enum class Mode { Demo, Trial, Full };

    /** Message thread, once per process (done on first use otherwise). */
    static void refresh();

    static Mode mode() noexcept;                                   // any thread
    static bool isFull() noexcept       { return mode() == Mode::Full; }
    static bool showsWatermark() noexcept { return mode() != Mode::Full; }
    static bool isSceneAllowed(int sceneIndex) noexcept;
    static int  nearestAllowedScene(int sceneIndex) noexcept;
    static bool allows(Feature f) noexcept;
    /** Longest recording in seconds (0 = no limit). */
    static double recordingLimitSeconds() noexcept;

    // ---- trial (message thread) -----------------------------------------------------------
    static constexpr int trialDays = 7;
    static bool canStartTrial();                 // never started on this computer
    static bool startTrial();
    static bool trialExpired();                  // a trial ran and is over
    static int  trialDaysLeft() noexcept;        // 0 when not in a trial

    // ---- serial (message thread) ----------------------------------------------------------
    static bool activate(const juce::String& serial);
    static void deactivate();
    static juce::String maskedSerial();          // "DALI-****-****-R6XV" or empty

    static juce::String featureName(Feature f);
    static juce::String demoSummary();           // what the demo includes, one line
    static juce::String statusText();            // "Full version (...)", "Trial - 5 days left", "Demo"

    static constexpr const char* buyUrl = "https://daliaudio.com";
};
} // namespace dali
