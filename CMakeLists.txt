cmake_minimum_required(VERSION 3.22)

project(Repetitor VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: lokaler Pfad (-DJUCE_PATH=...) oder automatischer Download
if(DEFINED JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Repetitor
    COMPANY_NAME "Dani Koenig"
    BUNDLE_ID "ch.danikoenig.repetitor"
    PLUGIN_MANUFACTURER_CODE Dkng
    PLUGIN_CODE Rpt1
    FORMATS VST3 AU
    AU_MAIN_TYPE kAudioUnitType_MIDIProcessor
    PRODUCT_NAME "Repetitor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Generator)

target_sources(Repetitor PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Look.cpp)

target_compile_definitions(Repetitor PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Repetitor
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Optionaler Test (nur lokal): -DREPETITOR_TEST_SRC=tests/EngineTest.cpp
if(REPETITOR_TEST_SRC)
    add_executable(RepetitorTest ${REPETITOR_TEST_SRC})
    target_link_libraries(RepetitorTest PRIVATE Repetitor)
    target_include_directories(RepetitorTest PRIVATE $<TARGET_PROPERTY:Repetitor,INCLUDE_DIRECTORIES>)
    target_compile_definitions(RepetitorTest PRIVATE $<TARGET_PROPERTY:Repetitor,COMPILE_DEFINITIONS>)
endif()
