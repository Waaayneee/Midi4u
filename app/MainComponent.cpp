#include "MainComponent.h"

#include "DeviceRules.h"
#include "Platform.h"

using midi4u::DeviceInfo;
using midi4u::DeviceState;
using midi4u::TapReaderStatus;

namespace
{
const juce::Colour green(0xff3ccf6e), amber(0xfff0b429), red(0xffe5484d), grey(0xff8a8f98);
const juce::Colour panel(0xff26282e), text(0xffe6e8ec), dimText(0xffa9aeb8);

constexpr int kNoneId = 1;
constexpr int kMissingId = 1000; // the saved device while it is not connected
constexpr int kFirstDeviceId = 2;

void fillDeviceCombo(juce::ComboBox& combo, const std::vector<DeviceInfo>& devices, const std::optional<DeviceInfo>& selected)
{
    combo.clear(juce::dontSendNotification);
    combo.addItem("None", kNoneId);

    int selectedId = kNoneId;

    for (size_t i = 0; i < devices.size(); ++i)
    {
        combo.addItem(devices[i].name, kFirstDeviceId + int(i));
        if (selected && devices[i] == *selected)
            selectedId = kFirstDeviceId + int(i);
    }

    if (selected && selectedId == kNoneId)
    {
        // Matched by ID or name (the name may have changed), otherwise shown as missing.
        if (const auto match = midi4u::findSavedDevice(*selected, devices))
        {
            for (size_t i = 0; i < devices.size(); ++i)
                if (devices[i] == *match)
                    selectedId = kFirstDeviceId + int(i);
        }
        else
        {
            combo.addItem(juce::String(selected->name) + " (not connected)", kMissingId);
            selectedId = kMissingId;
        }
    }

    combo.setSelectedId(selectedId, juce::dontSendNotification);
}

std::optional<DeviceInfo> pickedDevice(const juce::ComboBox& combo, const std::vector<DeviceInfo>& devices)
{
    const int index = combo.getSelectedId() - kFirstDeviceId;

    if (index >= 0 && index < int(devices.size()))
        return devices[size_t(index)];

    return std::nullopt;
}

struct LightState
{
    juce::Colour colour;
    juce::String detail;
};

LightState deviceLight(const midi4u::DeviceSupervisor& supervisor, const juce::String& noneText, juce::Colour noneColour)
{
    const juce::String name = supervisor.selected() ? juce::String(supervisor.selected()->name) : juce::String();

    switch (supervisor.state())
    {
        case DeviceState::running: return {green, name};
        case DeviceState::starting: return {amber, "Starting " + name};
        case DeviceState::missing: return {red, name + " is not connected"};
        case DeviceState::failed: return {red, name + ": " + juce::String(supervisor.lastError())};
        case DeviceState::notSelected: break;
    }
    return {noneColour, noneText};
}

LightState pluginLight(TapReaderStatus status)
{
    switch (status)
    {
        case TapReaderStatus::connected: return {green, "Receiving music from Ableton"};
        case TapReaderStatus::waitingForPlugin: return {red, "No signal from Midi4u Tap. Is Ableton open?"};
        case TapReaderStatus::versionMismatch: return {red, "Please update Midi4u (plugin and app versions differ)"};
        case TapReaderStatus::noBridge: break;
    }
    return {red, "Bridge not open"};
}

void setupFader(juce::Slider& fader, float db)
{
    fader.setSliderStyle(juce::Slider::LinearHorizontal);
    fader.setRange(-60.0, 12.0, 0.1);
    fader.setTextValueSuffix(" dB");
    fader.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 22);
    fader.setDoubleClickReturnValue(true, 0.0);
    fader.setValue(db, juce::dontSendNotification);
}

void styleLabel(juce::Label& label, bool bold = false)
{
    label.setColour(juce::Label::textColourId, bold ? text : dimText);
    label.setFont(juce::FontOptions(14.0f, bold ? juce::Font::bold : juce::Font::plain));
}
} // namespace

//==============================================================================
void LevelMeter::setPeak(float peak)
{
    const float db = peak > 0.0f ? 20.0f * std::log10(peak) : -100.0f;
    const float target = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
    level_ = target > level_ ? target : level_ * 0.85f + target * 0.15f; // fast up, slow down
    repaint();
}

void LevelMeter::paint(juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff15161a));
    g.fillRoundedRectangle(area, 3.0f);
    g.setColour(level_ > 0.95f ? red : (level_ > 0.8f ? amber : green));
    g.fillRoundedRectangle(area.withWidth(area.getWidth() * level_), 3.0f);
}

void StatusLight::set(juce::Colour colour, const juce::String& detail)
{
    if (colour == colour_ && detail == detail_)
        return;

    colour_ = colour;
    detail_ = detail;
    repaint();
}

void StatusLight::paint(juce::Graphics& g)
{
    auto area = getLocalBounds();
    g.setColour(colour_);
    g.fillEllipse(area.removeFromLeft(14).withSizeKeepingCentre(12, 12).translated(0, -8).toFloat());
    area.removeFromLeft(8);
    g.setColour(text);
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText(title_, area.removeFromTop(area.getHeight() / 2), juce::Justification::bottomLeft);
    g.setColour(dimText);
    g.setFont(juce::FontOptions(12.0f));
    g.drawFittedText(detail_, area, juce::Justification::topLeft, 2);
}

//==============================================================================
SetupPanel::SetupPanel(AppController& controller, std::function<void()> onDone)
    : controller_(controller), onDone_(std::move(onDone))
{
    const juce::String titles[6] = {"1. Virtual cable", "2. Midi4u Tap plugin", "3. Mic",
                                    "4. Output", "5. Discord settings", "6. Test"};

    for (int i = 0; i < 6; ++i)
    {
        auto& step = steps_[i];
        step.title.setText(titles[i], juce::dontSendNotification);
        styleLabel(step.title, true);
        styleLabel(step.detail);
        step.detail.setMinimumHorizontalScale(1.0f);
        addAndMakeVisible(step.title);
        addAndMakeVisible(step.detail);
        addChildComponent(step.fix);
    }

    steps_[0].fix.onClick = [] { platform::cableDownloadPage().launchInDefaultBrowser(); };
    steps_[1].fix.onClick = [] {
        const auto folder = platform::vst3Folder();
        (folder.isDirectory() ? folder : folder.getParentDirectory()).startAsProcess();
    };
    steps_[2].fix.onClick = [this] { controller_.useDefaultMic(); };
    steps_[3].fix.onClick = [this] {
        if (cableFound_)
            controller_.useCableOutput();
        else
            platform::cableDownloadPage().launchInDefaultBrowser();
    };
    steps_[5].fix.onClick = [this] {
        controller_.playTestTone();
        tonePlayed_ = true;
    };

    const juce::String checklistText[4] = {
        "Input device set to \"" + platform::cableDiscordInputName() + "\"",
        "Output device set to your headphones (Scarlett)",
        "Noise Suppression, Echo Cancellation and Automatic Gain Control off",
        "Input Sensitivity set manually to the lowest level"};

    for (int i = 0; i < 4; ++i)
    {
        checklist_[i].setButtonText(checklistText[i]);
        checklist_[i].setToggleState(controller_.settings().discordChecklist[i], juce::dontSendNotification);
        checklist_[i].onClick = [this, i] {
            controller_.settings().discordChecklist[i] = checklist_[i].getToggleState();
            controller_.saveSettings();
        };
        addAndMakeVisible(checklist_[i]);
    }

    addAndMakeVisible(micMeter_);
    addAndMakeVisible(doneButton_);
    doneButton_.onClick = [this] { onDone_(); };
}

void SetupPanel::start()
{
    nextCableScanMs_ = 0;
    tonePlayed_ = false;

    if (! controller_.settings().mic)
        controller_.useDefaultMic();

    if (! controller_.outputIsCable())
        controller_.useCableOutput();

    refresh(0.0f);
}

void SetupPanel::setStep(int index, bool done, const juce::String& detail, const juce::String& fixText)
{
    auto& step = steps_[index];

    if (step.done != done)
    {
        step.done = done;
        repaint(rows_[index]);
    }

    step.detail.setText(detail, juce::dontSendNotification);
    step.fix.setButtonText(fixText);
    step.fix.setVisible(fixText.isNotEmpty());
}

void SetupPanel::refresh(float micPeak)
{
    // Scanning devices is not free, so the cable check runs every 2 seconds.
    const auto now = juce::Time::getMillisecondCounter();

    if (now >= nextCableScanMs_)
    {
        nextCableScanMs_ = now + 2000;
        const bool found = controller_.cableDevice().has_value();

        if (found && ! cableFound_ && ! controller_.outputIsCable())
            controller_.useCableOutput(); // the cable just appeared
        cableFound_ = found;
    }

    const auto prefix = platform::cableOutputPrefix();

    // 1. Virtual cable
    if (cableFound_)
        setStep(0, true, "\"" + prefix + "\" is installed.");
    else
        setStep(0, false, "Install the free virtual cable, then restart if asked. This turns green when \"" + prefix + "\" appears.",
                "Download");

    // 2. Plugin
    const auto tap = controller_.pluginStatus();
    const bool installed = platform::installedPlugin().exists();

    if (tap == TapReaderStatus::connected)
        setStep(1, true, "Midi4u Tap is sending music from Ableton.");
    else if (tap == TapReaderStatus::versionMismatch)
        setStep(1, false, "Please update Midi4u: the plugin and app versions differ.");
    else if (! installed)
        setStep(1, false, "Copy \"Midi4u Tap.vst3\" into " + platform::vst3Folder().getFullPathName(), "Open folder");
    else
        setStep(1, false, "In Ableton, drag Midi4u Tap onto the Master track. Not listed? Turn on \"Use VST3 Plug-In System Folder\" in Ableton's Plug-Ins settings and rescan.");

    // 3. Mic
    const auto& mic = controller_.micSupervisor();
    const auto micName = mic.selected() ? juce::String(mic.selected()->name) : juce::String();
    micMeter_.setPeak(micPeak);

    if (mic.state() == DeviceState::running)
        setStep(2, true, micName + ". Speak and watch the meter.");
    else if (mic.state() == DeviceState::starting)
        setStep(2, false, "Starting " + micName);
    else if (! mic.selected())
        setStep(2, false, "No mic selected.", "Use default mic");
    else
        setStep(2, false, micName + " is not working: " + juce::String(mic.lastError()), "Use default mic");

    // 4. Output
    const auto& output = controller_.outputSupervisor();

    if (controller_.outputIsCable() && output.state() == DeviceState::running)
        setStep(3, true, "Sending the mix to \"" + prefix + "\".");
    else if (controller_.outputIsCable() && output.state() == DeviceState::starting)
        setStep(3, false, "Starting \"" + prefix + "\"");
    else if (cableFound_)
        setStep(3, false, "The mix should go to \"" + prefix + "\".", "Select " + prefix);
    else
        setStep(3, false, "Waiting for step 1.");

    // 5. Discord
    bool allTicked = true;
    for (const auto& item : checklist_)
        allTicked = allTicked && item.getToggleState();
    setStep(4, allTicked, "In Discord, open User Settings > Voice & Video and tick each item:");

    // 6. Test
    setStep(5, tonePlayed_,
            tonePlayed_ ? "Did you hear the beep in Discord's Mic Test? If not, check the red items above."
                        : "Start Discord's Mic Test, then play a short beep into the mix.",
            "Play test tone");
}

void SetupPanel::resized()
{
    auto area = getLocalBounds().reduced(16);
    doneButton_.setBounds(area.removeFromBottom(30).removeFromRight(100));
    area.removeFromBottom(8);

    const int heights[6] = {62, 62, 74, 62, 150, 62};

    for (int i = 0; i < 6; ++i)
    {
        rows_[i] = area.removeFromTop(heights[i]);
        auto row = rows_[i].withTrimmedLeft(28).reduced(0, 4);
        auto& step = steps_[i];

        auto top = row.removeFromTop(22);
        step.fix.setBounds(top.removeFromRight(150));
        step.title.setBounds(top);

        if (i == 4)
        {
            step.detail.setBounds(row.removeFromTop(22));
            for (auto& item : checklist_)
                item.setBounds(row.removeFromTop(24));
        }
        else if (i == 2)
        {
            step.detail.setBounds(row.removeFromTop(30));
            micMeter_.setBounds(row.removeFromTop(10).withWidth(240).translated(4, 2));
        }
        else
        {
            step.detail.setBounds(row);
        }
    }
}

void SetupPanel::paint(juce::Graphics& g)
{
    g.fillAll(panel);

    for (int i = 0; i < 6; ++i)
    {
        g.setColour(steps_[i].done ? green : grey);
        g.fillEllipse(juce::Rectangle<float>(float(rows_[i].getX() + 2), float(rows_[i].getY() + 9), 14.0f, 14.0f));
    }
}

//==============================================================================
MainComponent::MainComponent(AppController& controller)
    : controller_(controller), setupPanel_(controller, [this] { showSetup(false); })
{
    auto& settings = controller_.settings();

    for (auto* light : {&pluginLight_, &micLight_, &outputLight_})
        addAndMakeVisible(light);

    addChildComponent(outputFix_);
    outputFix_.onClick = [this] { fixOutput(); };

    for (auto* label : {&musicLabel_, &micLabel_})
    {
        styleLabel(*label, true);
        addAndMakeVisible(label);
    }

    setupFader(musicFader_, settings.musicDb);
    setupFader(micFader_, settings.micDb);
    musicFader_.onValueChange = [this] { controller_.setMusicDb(float(musicFader_.getValue())); };
    micFader_.onValueChange = [this] { controller_.setMicDb(float(micFader_.getValue())); };
    musicFader_.onDragEnd = micFader_.onDragEnd = [this] { controller_.saveSettings(); };

    musicMute_.setToggleState(settings.musicMuted, juce::dontSendNotification);
    micMute_.setToggleState(settings.micMuted, juce::dontSendNotification);
    musicMute_.onClick = [this] { controller_.setMusicMuted(musicMute_.getToggleState()); };
    micMute_.onClick = [this] { controller_.setMicMuted(micMute_.getToggleState()); };

    for (juce::Component* c : {static_cast<juce::Component*>(&musicFader_), static_cast<juce::Component*>(&micFader_),
                               static_cast<juce::Component*>(&musicMute_), static_cast<juce::Component*>(&micMute_),
                               static_cast<juce::Component*>(&musicMeter_), static_cast<juce::Component*>(&micMeter_)})
        addAndMakeVisible(c);

    for (auto* label : {&micInputLabel_, &sendToLabel_, &latencyLabel_})
    {
        styleLabel(*label);
        addAndMakeVisible(label);
    }

    micInput_.beforePopup = [this] { refreshMicList(); };
    sendTo_.beforePopup = [this] { refreshOutputList(); };
    micInput_.onChange = [this] {
        if (micInput_.getSelectedId() != kMissingId)
            controller_.selectMic(pickedDevice(micInput_, micList_));
    };
    sendTo_.onChange = [this] {
        if (sendTo_.getSelectedId() != kMissingId)
        {
            controller_.selectOutput(pickedDevice(sendTo_, outputList_));
            refreshMicList(); // the feedback rule depends on the output
        }
    };
    addAndMakeVisible(micInput_);
    addAndMakeVisible(sendTo_);

    latency_.addItem("Low", 1);
    latency_.addItem("Normal", 2);
    latency_.addItem("Safe", 3);
    latency_.setSelectedId(int(settings.latency) + 1, juce::dontSendNotification);
    latency_.onChange = [this] { controller_.setLatency(midi4u::Latency(latency_.getSelectedId() - 1)); };
    addAndMakeVisible(latency_);

    startWithSystem_.setToggleState(settings.startWithSystem, juce::dontSendNotification);
    startWithSystem_.onClick = [this] { controller_.setStartWithSystem(startWithSystem_.getToggleState()); };
    addChildComponent(startWithSystem_);
    startWithSystem_.setVisible(platform::supportsStartWithSystem());

    setupButton_.onClick = [this] { showSetup(true); };
    testToneButton_.onClick = [this] { controller_.playTestTone(); };
    addAndMakeVisible(setupButton_);
    addAndMakeVisible(testToneButton_);

    addChildComponent(setupPanel_);

    refreshMicList();
    refreshOutputList();
    refreshStatus();
    setSize(600, 640);
    startTimerHz(15);

    if (! settings.output) // first launch
        showSetup(true);
}

MainComponent::~MainComponent()
{
    stopTimer();
}

void MainComponent::refreshMicList()
{
    micList_ = controller_.micChoices();
    fillDeviceCombo(micInput_, micList_, controller_.settings().mic);
}

void MainComponent::refreshOutputList()
{
    outputList_ = controller_.outputChoices();
    fillDeviceCombo(sendTo_, outputList_, controller_.settings().output);
}

void MainComponent::showSetup(bool show)
{
    setupPanel_.setVisible(show);

    if (show)
    {
        setupPanel_.start();
        setupPanel_.toFront(false);
    }
    else
    {
        refreshMicList();
        refreshOutputList();
    }
}

void MainComponent::fixOutput()
{
    if (controller_.cableDevice())
        controller_.useCableOutput();
    else
        platform::cableDownloadPage().launchInDefaultBrowser();

    refreshOutputList();
}

void MainComponent::refreshStatus()
{
    const auto plugin = pluginLight(controller_.pluginStatus());
    pluginLight_.set(plugin.colour, plugin.detail);

    const auto mic = deviceLight(controller_.micSupervisor(), "No mic selected", grey);
    micLight_.set(mic.colour, mic.detail);

    const auto output = deviceLight(controller_.outputSupervisor(), "Choose where to send the mix", red);
    outputLight_.set(output.colour, output.detail);

    const auto outputState = controller_.outputSupervisor().state();
    outputFix_.setVisible(outputState != DeviceState::running && outputState != DeviceState::starting);
}

void MainComponent::timerCallback()
{
    auto& engine = controller_.engine();
    const float micPeak = engine.micMeter().take();
    musicMeter_.setPeak(engine.mixer().musicMeter().take());
    micMeter_.setPeak(micPeak);

    if (++ticks_ % 4 == 0)
    {
        refreshStatus();

        if (setupPanel_.isVisible())
            setupPanel_.refresh(micPeak);
    }
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(panel);
    g.setColour(text);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("Midi4u", getLocalBounds().reduced(16).removeFromTop(30), juce::Justification::centredLeft);
}

void MainComponent::resized()
{
    setupPanel_.setBounds(getLocalBounds());

    auto area = getLocalBounds().reduced(16);
    area.removeFromTop(40);

    // Status lights
    auto lights = area.removeFromTop(48);
    const int third = lights.getWidth() / 3;
    pluginLight_.setBounds(lights.removeFromLeft(third).reduced(4, 0));
    micLight_.setBounds(lights.removeFromLeft(third).reduced(4, 0));
    outputLight_.setBounds(lights.reduced(4, 0));
    outputFix_.setBounds(outputLight_.getBounds().removeFromRight(44).removeFromTop(22));
    area.removeFromTop(20);

    // Faders
    auto strip = [&](juce::Label& label, juce::Slider& fader, juce::ToggleButton& mute, LevelMeter& meter) {
        auto row = area.removeFromTop(30);
        label.setBounds(row.removeFromLeft(60));
        mute.setBounds(row.removeFromRight(70));
        fader.setBounds(row);
        meter.setBounds(area.removeFromTop(8).withTrimmedLeft(64).withTrimmedRight(150));
        area.removeFromTop(16);
    };
    strip(musicLabel_, musicFader_, musicMute_, musicMeter_);
    strip(micLabel_, micFader_, micMute_, micMeter_);
    area.removeFromTop(8);

    // Manual mode
    auto field = [&](juce::Label& label, juce::Component& control) {
        auto row = area.removeFromTop(30);
        label.setBounds(row.removeFromLeft(100));
        control.setBounds(row);
        area.removeFromTop(10);
    };
    field(micInputLabel_, micInput_);
    field(sendToLabel_, sendTo_);
    field(latencyLabel_, latency_);
    startWithSystem_.setBounds(area.removeFromTop(26).withTrimmedLeft(100));

    auto buttons = getLocalBounds().reduced(16).removeFromBottom(34);
    setupButton_.setBounds(buttons.removeFromLeft(160));
    buttons.removeFromLeft(10);
    testToneButton_.setBounds(buttons.removeFromLeft(140));
}
