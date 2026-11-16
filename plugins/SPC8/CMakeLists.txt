# SPC8 - S-DSP (SNES) sample instrument
#
# Phase 2.1.1: engine skeleton + audible tone. The WebView UI arrives in 2.1.4,
# which is when NEEDS_WEBVIEW2 flips on and Source/ui/public/ is added.
#
# Generated from templates/CMakeLists.txt.template.

if(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
elseif(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
    set(AU_MAIN_TYPE kAudioUnitType_MusicDevice)
elseif(UNIX)
    set(PLUGIN_FORMATS VST3 LV2 Standalone)
    set(LV2_URI "https://github.com/noizefield/audio-plugin-coder/SPC8")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

message(STATUS "SPC8: Building for ${PLUGIN_FORMATS}")
message(STATUS "SPC8: UI - plain editor in 2.1.1; WebView page lands in 2.1.4")

juce_add_plugin(SPC8
    COMPANY_NAME "Hoursiscool"
    PLUGIN_MANUFACTURER_CODE Hrsl
    PLUGIN_CODE Spc8
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "SPC8"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    NEEDS_WEBVIEW2 FALSE
    VST3_CATEGORIES Instrument
    AU_MAIN_TYPE ${AU_MAIN_TYPE}
    LV2URI ${LV2_URI}
)

target_sources(SPC8
    PRIVATE
        Source/PluginProcessor.cpp
)

target_include_directories(SPC8
    PRIVATE
        Source
)

target_link_libraries(SPC8
    PRIVATE
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
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(SPC8
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        _USE_MATH_DEFINES=1
)

if(APPLE)
    target_compile_definitions(SPC8 PUBLIC JUCE_USE_CURL=0)
    set_target_properties(SPC8 PROPERTIES
        XCODE_ATTRIBUTE_ARCHS "x86_64;arm64"
        XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO
    )
endif()

if(UNIX AND NOT APPLE)
    target_compile_definitions(SPC8 PUBLIC JUCE_USE_CURL=0 JUCE_JACK=1)
endif()

# ============================================
# S-DSP console self-test (2.1.1 acceptance)
# ============================================
# The DSP core is JUCE-free, so a plain console binary can prove a MIDI note
# produces audio without loading a DAW.
option(SPC8_BUILD_SELFTEST "Build the SPC8 S-DSP console self-test" OFF)
if(SPC8_BUILD_SELFTEST)
    add_executable(SPC8_SelfTest Source/dsp/selftest.cpp)
    target_include_directories(SPC8_SelfTest PRIVATE Source)
    set_target_properties(SPC8_SelfTest PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
endif()
