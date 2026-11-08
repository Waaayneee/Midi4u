juce_add_plugin(Midi4uTap
    COMPANY_NAME "Midi4u"
    BUNDLE_ID "com.midi4u.tap"
    PLUGIN_MANUFACTURER_CODE Mi4u
    PLUGIN_CODE Tap1
    FORMATS VST3
    PRODUCT_NAME "Midi4u Tap"
    VERSION ${PROJECT_VERSION}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Fx Tools
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Midi4uTap PRIVATE TapPlugin.cpp)

target_compile_definitions(Midi4uTap PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Midi4uTap
    PRIVATE
        midi4u_core
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
