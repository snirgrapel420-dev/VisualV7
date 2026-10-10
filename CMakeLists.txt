# =============================================================================
#  DALI VISUAL by DALI AUDIO — audio-reactive generative visual instrument
#  Builds: VST3 + Standalone (one shared engine)
#
#  JUCE is taken from (first match):
#    1. -DDALI_JUCE_PATH=/path/to/JUCE   (a local JUCE checkout)
#    2. ./JUCE                            (git submodule / copy next to this file)
#    3. FetchContent download of JUCE 8.0.4
# =============================================================================
cmake_minimum_required(VERSION 3.22)
project(DaliVisual VERSION 6.9.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
endif()

set(DALI_JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout (optional)")
if(DALI_JUCE_PATH AND EXISTS "${DALI_JUCE_PATH}/CMakeLists.txt")
    add_subdirectory("${DALI_JUCE_PATH}" JUCE EXCLUDE_FROM_ALL)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(DaliVisual
    COMPANY_NAME                "Dali Audio"
    COMPANY_WEBSITE             "https://daliaudio.com"
    BUNDLE_ID                   com.daliaudio.dalivisual
    PRODUCT_NAME                "Dali Visual"
    PLUGIN_MANUFACTURER_CODE    Dali
    PLUGIN_CODE                 DvIs
    FORMATS                     VST3 AU Standalone
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT  "Dali Visual listens to audio input to drive the visuals."
    VST3_CATEGORIES             Fx Analyzer
    COPY_PLUGIN_AFTER_BUILD     FALSE)

# ---- shaders + scene thumbnails embedded as BinaryData (names: file name with '.' -> '_') ----
file(GLOB DALI_SHADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/scenes/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/fx/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/thumbs/*.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/watermark.png")
juce_add_binary_data(DaliVisualBinaryData
    HEADER_NAME BinaryData.h
    NAMESPACE   BinaryData
    SOURCES     ${DALI_SHADERS})

target_sources(DaliVisual PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Core/Parameters.cpp
    Source/Core/AutoPilot.cpp
    Source/Audio/FFT.cpp
    Source/Audio/BeatTracker.cpp
    Source/Audio/FeatureExtractor.cpp
    Source/Audio/MusicalClock.cpp
    Source/Audio/AudioAnalyzer.cpp
    Source/Audio/LoopbackCapture.cpp
    Source/Modulation/ModulationCore.cpp
    Source/Modulation/ModulationMatrix.cpp
    Source/Image/ImageDNA.cpp
    Source/Image/ImageProcessor.cpp
    Source/Image/TemplateGenerator.cpp
    Source/Render/ShaderProgram.cpp
    Source/Render/RenderTarget.cpp
    Source/Render/RenderEngine.cpp
    Source/Output/OutputManager.cpp
    Source/Output/NativeWindow.cpp
    Source/Output/VideoRecorder.cpp
    Source/Output/VideoEncoderWin.cpp
    Source/Render/RecordCapture.cpp
    Source/Core/StandaloneSupport.cpp
    Source/Core/License.cpp
    Source/Midi/MidiMapper.cpp
    Source/UI/DaliLookAndFeel.cpp
    Source/UI/ParamControls.cpp
    Source/UI/Panels.cpp
    Source/UI/Chrome.cpp
    Source/UI/VisualView.cpp)

target_include_directories(DaliVisual PRIVATE Source)

target_compile_definitions(DaliVisual PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0)

if(MSVC)
    target_compile_options(DaliVisual PRIVATE /utf-8 /bigobj)
endif()
if(WIN32)
    # ole32: WASAPI loopback (system audio) · Media Foundation: the video recorder (H.264 + AAC)
    target_link_libraries(DaliVisual PRIVATE ole32 mfplat mfreadwrite mfuuid)
endif()
if(APPLE)
    # the video recorder: AVAssetWriter (H.264 via VideoToolbox + AAC)
    target_sources(DaliVisual PRIVATE Source/Output/VideoEncoderMac.mm)
    set_source_files_properties(Source/Output/VideoEncoderMac.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    target_link_libraries(DaliVisual PRIVATE "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo")
endif()

target_link_libraries(DaliVisual
    PRIVATE
        DaliVisualBinaryData
        juce::juce_audio_utils
        juce::juce_opengl
        juce::juce_cryptography
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# ---- optional: JUCE-free unit tests for the DSP / modulation / image core ----
option(DALI_BUILD_TESTS "Build the core unit tests" OFF)
if(DALI_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
