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
    addAndMakeVisible(doneButton_);
    doneButton_.onClick = [this] { onDone_(); };
}

void SetupPanel::start()
{
    if (! controller_.settings().mic)
        controller_.useDefaultMic();

    if (! controller_.outputIsCable())
        controller_.useCableOutput();
}

void SetupPanel::resized()
{
    doneButton_.setBounds(getLocalBounds().reduced(16).removeFromBottom(30).removeFromRight(100));
}

void SetupPanel::paint(juce::Graphics& g)
{
    g.fillAll(panel);

    const auto cable = platform::cableOutputPrefix();
    const std::pair<int, juce::String> points[] = {
        {0, "Install the free virtual cable from " + platform::cableDownloadPage().toString(false)
                + ", then restart if asked. Afterwards \"" + cable + "\" appears in your sound devices."},
        {0, "In Ableton, drag Midi4u Tap onto the Master track. Not listed? Turn on \"Use VST3 Plug-In System "
            "Folders\" in Ableton's Plug-Ins settings and click Rescan."},
        {0, "In this window, check that Mic input is your mic and Send mix to is \"" + cable
                + "\". Midi4u picks both by itself the first time."},
        {0, "In Discord, open User Settings > Voice & Video and set:"},
        {1, "Input Device: \"" + platform::cableDiscordInputName() + "\""},
        {1, "Output Device: your headphones (Scarlett)"},
        {1, "Noise Suppression, Echo Cancellation and Automatic Gain Control: off"},
        {1, "Input Sensitivity: set manually to the lowest level"},
        {0, "Test: start Discord's Mic Test and click Play test tone. You should hear a short beep."},
        {0, "When everything works, the three lights at the top of this window are green."}};

    auto area = getLocalBounds().reduced(24, 20);
    g.setColour(text);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("Set up Midi4u", area.removeFromTop(28), juce::Justification::centredLeft);
    area.removeFromTop(8);

    for (size_t i = 0; i < std::size(points); ++i)
    {
        const auto& [level, line] = points[i];
        const bool nextIsSubPoint = i + 1 < std::size(points) && points[i + 1].first == 1;
        auto row = area.withTrimmedLeft(level * 20);
        g.setColour(dimText);
        g.setFont(juce::FontOptions(14.0f));
        g.drawText(level == 0 ? juce::String::fromUTF8("\xe2\x80\xa2") : "-", row.removeFromLeft(16).withHeight(20),
                   juce::Justification::centredLeft);

        juce::AttributedString string(line);
        string.setFont(juce::FontOptions(14.0f));
        string.setColour(text);
        juce::TextLayout layout;
        layout.createLayout(string, float(row.getWidth()));
        layout.draw(g, row.withHeight(int(std::ceil(layout.getHeight()))).toFloat().translated(0, 1));
        area.removeFromTop(int(std::ceil(layout.getHeight())) + (nextIsSubPoint ? 4 : 10));
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
    musicMeter_.setPeak(engine.mixer().musicMeter().take());
    micMeter_.setPeak(engine.micMeter().take());

    if (++ticks_ % 4 == 0)
        refreshStatus();
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
