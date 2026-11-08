juce_add_gui_app(Midi4u
    PRODUCT_NAME "Midi4u"
    COMPANY_NAME "Midi4u"
    BUNDLE_ID "com.midi4u.app"
    VERSION ${PROJECT_VERSION}
    ICON_BIG ${CMAKE_CURRENT_SOURCE_DIR}/icon.png
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Midi4u mixes your mic with Ableton's music for Discord.")

target_sources(Midi4u PRIVATE
    AppController.cpp
    JuceDevice.cpp
    Main.cpp
    MainComponent.cpp
    Platform.cpp)

target_compile_definitions(Midi4u PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Midi4u,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Midi4u,JUCE_VERSION>"
    JUCE_ASIO=0)

target_link_libraries(Midi4u
    PRIVATE
        midi4u_core
        juce::juce_audio_devices
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(WIN32)
    target_link_libraries(Midi4u PRIVATE ole32)
endif()
