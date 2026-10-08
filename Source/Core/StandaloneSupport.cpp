#include "StandaloneSupport.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

// The shared code is compiled once for every format: the holder only exists in the standalone app.
// (Its 'currentInstance' is an inline variable - one object per binary - so inside the VST3 / AU
// it simply stays nullptr.)
#if JucePlugin_Build_Standalone && ! JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
 #define DALI_HAS_STANDALONE_HOLDER 1
#endif

namespace dali::standalone
{
bool unmuteInput()
{
   #if DALI_HAS_STANDALONE_HOLDER
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
    {
        holder->getMuteInputValue().setValue(false);      // also stored in the app's settings file
        return true;
    }
   #endif
    return false;
}
} // namespace dali::standalone
