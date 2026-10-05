# Core unit tests — no JUCE, no GPU. Run: ctest  (or ./DaliVisualCoreTests)
# Works as a sub-directory of the main project or standalone: cmake -S Tests -B build-tests
if(NOT DEFINED PROJECT_NAME)
    cmake_minimum_required(VERSION 3.22)
    project(DaliVisualCoreTests LANGUAGES CXX)
endif()
add_executable(DaliVisualCoreTests
    CoreTests.cpp
    ../Source/Audio/FFT.cpp
    ../Source/Audio/BeatTracker.cpp
    ../Source/Audio/FeatureExtractor.cpp
    ../Source/Audio/MusicalClock.cpp
    ../Source/Modulation/ModulationCore.cpp
    ../Source/Image/ImageDNA.cpp)
target_compile_features(DaliVisualCoreTests PRIVATE cxx_std_17)
find_package(Threads REQUIRED)
target_link_libraries(DaliVisualCoreTests PRIVATE Threads::Threads)
enable_testing()
add_test(NAME DaliVisualCoreTests COMMAND DaliVisualCoreTests)
