cmake_minimum_required(VERSION 3.22)

project(Repetitor VERSION 1.5.1)

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

# Zwei Varianten aus demselben Code:
#  Repetitor       MIDI-Effekt (Bitwig, Logic MIDI FX, Reaper ...)
#  Repetitor Instrument  Instrument mit MIDI-Ausgang (Ableton Live: auf eigener Spur, andere Spuren holen "MIDI From")
function(repetitor_target target product code bundle au_type is_instr)
    if(is_instr)
        set(midi_fx FALSE)
        set(synth TRUE)
        set(cats Instrument Generator)
    else()
        set(midi_fx TRUE)
        set(synth FALSE)
        set(cats Fx Generator)
    endif()

    juce_add_plugin(${target}
        COMPANY_NAME "Dani Koenig"
        BUNDLE_ID "ch.danikoenig.${bundle}"
        PLUGIN_MANUFACTURER_CODE Dkng
        PLUGIN_CODE ${code}
        FORMATS VST3 AU
        AU_MAIN_TYPE ${au_type}
        PRODUCT_NAME "${product}"
        IS_SYNTH ${synth}
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT ${midi_fx}
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES ${cats})

    target_sources(${target} PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Look.cpp)

    target_compile_definitions(${target} PUBLIC
        REPETITOR_INSTRUMENT=$<IF:$<BOOL:${is_instr}>,1,0>
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(${target}
        PRIVATE
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endfunction()

repetitor_target(Repetitor     "Repetitor"      Rpt1 repetitor     kAudioUnitType_MIDIProcessor FALSE)
repetitor_target(RepetitorInst "Repetitor Instrument" Rpt2 repetitorinst kAudioUnitType_MusicDevice   TRUE)

# Optionaler Test (nur lokal): -DREPETITOR_TEST_SRC=tests/EngineTest.cpp
if(REPETITOR_TEST_SRC)
    add_executable(RepetitorTest ${REPETITOR_TEST_SRC})
    target_link_libraries(RepetitorTest PRIVATE Repetitor)
    target_include_directories(RepetitorTest PRIVATE $<TARGET_PROPERTY:Repetitor,INCLUDE_DIRECTORIES>)
    target_compile_definitions(RepetitorTest PRIVATE $<TARGET_PROPERTY:Repetitor,COMPILE_DEFINITIONS>)
endif()
