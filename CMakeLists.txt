cmake_minimum_required(VERSION 3.22)
project(LumetrixPro VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Descarga JUCE automáticamente ────────────────────────────
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
)
FetchContent_MakeAvailable(JUCE)

# ── Plugin target ─────────────────────────────────────────────
juce_add_plugin(LumetrixPro
    PLUGIN_MANUFACTURER_CODE Lmtx
    PLUGIN_CODE             Lmpr
    FORMATS                 VST3
    PRODUCT_NAME            "LUMETRIX PRO"
    PLUGIN_MANUFACTURER     "YourName"
    COMPANY_NAME            "YourName"
    BUNDLE_ID               com.yourname.lumetrixpro
    VERSION                 1.0.0

    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    VST3_CATEGORIES         "Analyzer" "Tools"
)

# ── Sources ───────────────────────────────────────────────────
target_sources(LumetrixPro PRIVATE
    PluginProcessor.h
    PluginProcessor.cpp
    PluginEditor.h
    PluginEditor.cpp
)

# ── Compile definitions ───────────────────────────────────────
target_compile_definitions(LumetrixPro PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

# ── Link libraries ────────────────────────────────────────────
target_link_libraries(LumetrixPro PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
