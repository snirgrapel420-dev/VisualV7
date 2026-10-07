#pragma once
#include <juce_data_structures/juce_data_structures.h>

// ============================================================================
//  Application preferences that are not part of a project (stored per user).
//   restoreSession  standalone only: bring back the last session's creative state
//                   (parameters, modulation, effects, image) on launch. Off by
//                   default: every launch starts fresh, with the first scene's init.
//                   The setup (display, render, MIDI mappings, audio source) is
//                   always restored.
// ============================================================================
namespace dali
{
struct AppPrefs
{
    static juce::PropertiesFile::Options options()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "DaliVisual";
        o.folderName = "Dali Audio";
        o.filenameSuffix = ".settings";
        o.osxLibrarySubFolder = "Application Support";
        return o;
    }
    static bool restoreSession()
    {
        juce::PropertiesFile f(options());
        return f.getBoolValue("restoreSession", false);
    }
    static void setRestoreSession(bool b)
    {
        juce::PropertiesFile f(options());
        f.setValue("restoreSession", b);
        f.saveIfNeeded();
    }
};
} // namespace dali
