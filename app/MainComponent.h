#pragma once

#include "AppController.h"

#include <juce_gui_basics/juce_gui_basics.h>

// Horizontal peak meter, -60 to 0 dBFS.
class LevelMeter final : public juce::Component
{
public:
    void setPeak(float peak);
    void paint(juce::Graphics& g) override;

private:
    float level_ = 0.0f; // 0..1 on the dB scale
};

// Device dropdown that refreshes its list just before it opens.
class DeviceCombo final : public juce::ComboBox
{
public:
    std::function<void()> beforePopup;
    void showPopup() override
    {
        if (beforePopup)
            beforePopup();
        juce::ComboBox::showPopup();
    }
};

// Coloured dot with a title and a line of detail.
class StatusLight final : public juce::Component
{
public:
    explicit StatusLight(juce::String title) : title_(std::move(title)) {}
    void set(juce::Colour colour, const juce::String& detail);
    void paint(juce::Graphics& g) override;

private:
    juce::String title_, detail_;
    juce::Colour colour_ = juce::Colours::grey;
};

// The "Set up Midi4u" instructions, shown on first launch and from the Set up button.
class SetupPanel final : public juce::Component
{
public:
    SetupPanel(AppController& controller, std::function<void()> onDone);
    void start(); // picks the default mic and the cable output if they are not set yet
    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    AppController& controller_;
    std::function<void()> onDone_;
    juce::TextButton doneButton_{"Done"};
};

class MainComponent final : public juce::Component, private juce::Timer
{
public:
    explicit MainComponent(AppController& controller);
    ~MainComponent() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;
    void refreshMicList();
    void refreshOutputList();
    void refreshStatus();
    void fixOutput();
    void showSetup(bool show);

    AppController& controller_;

    StatusLight pluginLight_{"Plugin"}, micLight_{"Mic"}, outputLight_{"Output"};
    juce::TextButton outputFix_{"Fix"};

    juce::Label musicLabel_{{}, "Music"}, micLabel_{{}, "Mic"};
    juce::Slider musicFader_, micFader_;
    juce::ToggleButton musicMute_{"Mute"}, micMute_{"Mute"};
    LevelMeter musicMeter_, micMeter_;

    juce::Label micInputLabel_{{}, "Mic input"}, sendToLabel_{{}, "Send mix to"}, latencyLabel_{{}, "Latency"};
    DeviceCombo micInput_, sendTo_;
    juce::ComboBox latency_;
    juce::ToggleButton startWithSystem_{"Start with Windows"};

    juce::TextButton setupButton_{"Set up Midi4u"}, testToneButton_{"Play test tone"};
    SetupPanel setupPanel_;

    std::vector<midi4u::DeviceInfo> micList_, outputList_;
    int ticks_ = 0;
};
