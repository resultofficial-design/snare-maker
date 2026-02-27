cmake_minimum_required(VERSION 3.21)
project(SnareMaker VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── JUCE ──────────────────────────────────────────────────────────────────────
add_subdirectory("C:/HISE/HISE/JUCE" "${CMAKE_BINARY_DIR}/JUCE")

# ── Plugin target ─────────────────────────────────────────────────────────────
juce_add_plugin(SnareMaker
    COMPANY_NAME                "YourCompany"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    Ycom
    PLUGIN_CODE                 Snrm
    FORMATS                     VST3 Standalone   # Add AU here when targeting macOS
    PRODUCT_NAME                "SnareMaker"
)

# ── Generate JuceHeader.h and add its directory to the include path ───────────
# Required so that #include <JuceHeader.h> resolves in our source files.
juce_generate_juce_header(SnareMaker)

# ── Sources ───────────────────────────────────────────────────────────────────
target_sources(SnareMaker PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/EnvelopeEditor.cpp
)

# ── Compile definitions ───────────────────────────────────────────────────────
target_compile_definitions(SnareMaker PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(SnareMaker
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
