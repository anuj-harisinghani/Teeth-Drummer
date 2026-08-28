cmake_minimum_required(VERSION 3.22)

project(TeethDrummer VERSION 0.101 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Download JUCE via FetchContent
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        9.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# Source files
set(SOURCE_FILES
    Source/Common/Constants.h
    Source/Common/LockFreeQueue.h
    Source/DSP/BiquadFilter.h
    Source/DSP/EnvelopeFollower.h
    Source/DSP/CircularAudioBuffer.h
    Source/DSP/TransientDetector.h
    Source/DSP/TransientDetector.cpp
    Source/DSP/FeatureExtractor.h
    Source/DSP/FeatureExtractor.cpp
    Source/Classifier/ModelData.h
    Source/Classifier/RuleBasedFallback.h
    Source/Classifier/DistanceClassifier.h
    Source/Classifier/DistanceClassifier.cpp
    Source/Classifier/ClassifierEngine.h
    Source/Classifier/ClassifierEngine.cpp
    Source/MIDI/VelocityCurve.h
    Source/MIDI/MIDITriggerEngine.h
    Source/MIDI/MIDITriggerEngine.cpp
    Source/Synth/DrumSampler.h
    Source/Synth/DrumSampler.cpp
    Source/Calibration/CalibrationManager.h
    Source/Calibration/CalibrationManager.cpp
    Source/Calibration/ProfileStorage.h
    Source/Calibration/ProfileStorage.cpp
    Source/UI/OscilloscopeComponent.h
    Source/UI/DrumKitPadVisualizer.h
    Source/UI/SensitivityControls.h
    Source/UI/CalibrationWizardView.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)

# JUCE Plugin Target (VST3 + Standalone Application)
juce_add_plugin(TeethDrummer
    COMPANY_NAME "TeethAudio"
    PLUGIN_NAME "Teeth Drummer"
    PRODUCT_NAME "Teeth Drummer"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE Teth
    PLUGIN_CODE Tthm
    FORMATS VST3 Standalone
)

target_sources(TeethDrummer PRIVATE ${SOURCE_FILES})

target_compile_definitions(TeethDrummer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    APP_VERSION_STRING=\"v${PROJECT_VERSION}\"
)

target_link_libraries(TeethDrummer PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce_recommended_config_flags
    juce_recommended_warning_flags
)

# Standalone DSP & Classifier Unit Tests
enable_testing()
add_executable(TeethDrummer_Tests
    Tests/TestDSPAndClassifier.cpp
    Source/DSP/TransientDetector.cpp
    Source/DSP/FeatureExtractor.cpp
    Source/Classifier/DistanceClassifier.cpp
    Source/Classifier/ClassifierEngine.cpp
    Source/Calibration/CalibrationManager.cpp
    Source/MIDI/MIDITriggerEngine.cpp
    Source/Synth/DrumSampler.cpp
)

target_include_directories(TeethDrummer_Tests PRIVATE Source)

target_link_libraries(TeethDrummer_Tests PRIVATE
    juce::juce_audio_basics
    juce::juce_core
)

add_test(NAME DSPAndClassifierTest COMMAND TeethDrummer_Tests)

