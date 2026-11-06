cmake_minimum_required(VERSION 3.22)
project(AmpForge VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(AmpForge
    COMPANY_NAME "AmpForge Audio"
    PLUGIN_MANUFACTURER_CODE Afrg
    PLUGIN_CODE Afg1
    FORMATS VST3 Standalone
    PRODUCT_NAME "AmpForge"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Distortion
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(AmpForge PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Core/Params.cpp
    Source/Amps/AmpModel.cpp
    Source/Cabinet/CabinetEngine.cpp
    Source/Presets/PresetManager.cpp
    Source/Presets/FactoryPresets.cpp)

target_compile_definitions(AmpForge PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AmpForge
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
