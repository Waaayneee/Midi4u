// Midi4u Tap: sits on Ableton's Master track, passes audio through untouched and
// copies it to the Midi4u App through shared memory. All bridge logic lives in
// core/TapPluginSide so it is covered by the tests.
//
// Optional "sound for Discord stream": Ableton plays through ASIO, which Discord can't
// hear when you share only the Ableton window. When this is on, the plugin also plays the
// music to the virtual cable from inside Ableton's process, and Discord's window sound
// capture picks that up. It reuses the app's Engine, with Ableton's audio as the "mic".

#include "DeviceRules.h"
#include "DeviceSupervisor.h"
#include "Engine.h"
#include "JuceDevice.h"
#include "Platform.h"
#include "TapPluginSide.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace
{
uint64_t nowMs()
{
    return static_cast<uint64_t>(juce::Time::getMillisecondCounterHiRes());
}

class TapProcessor final : public juce::AudioProcessor, private juce::Timer
{
public:
    TapProcessor()
        : AudioProcessor(BusesProperties()
                             .withInput("Input", juce::AudioChannelSet::stereo(), true)
                             .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
        bridge_.tick(nowMs());
        stream_.setLatency(kStreamLatency);
        startTimerHz(20);
    }

    ~TapProcessor() override { stopTimer(); }

    midi4u::TapPluginSide& bridge() { return bridge_; }

    // Any thread. The timer opens or closes the device.
    void setStreamSound(bool on) { streamWanted_.store(on, std::memory_order_relaxed); }
    bool streamSound() const { return streamWanted_.load(std::memory_order_relaxed); }

    // Message thread.
    juce::String streamStatus() const
    {
        using State = midi4u::DeviceState;
        const auto cable = platform::cableOutputPrefix();

        if (! streamSound())
            return "Off";

        if (streamSupervisor_ == nullptr)
            return "Starting...";

        if (! streamSupervisor_->selected())
            return cable + " not found";

        switch (streamSupervisor_->state())
        {
            case State::running:  return "Playing into " + cable;
            case State::starting: return "Starting...";
            case State::missing:  return cable + " not found";
            case State::failed:   return "Can't open " + cable;
            case State::notSelected: break;
        }
        return "Starting...";
    }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    // Saved with the Ableton project.
    void getStateInformation(juce::MemoryBlock& dest) override { juce::MemoryOutputStream(dest, false).writeBool(streamSound()); }

    void setStateInformation(const void* data, int size) override
    {
        if (size >= 1)
            setStreamSound(static_cast<const char*>(data)[0] != 0);
    }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
            && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        bridge_.setSampleRate(sampleRate);
        stream_.micStarting(sampleRate, samplesPerBlock);
    }

    void releaseResources() override { stream_.micStopped(); }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        // Input and output share this buffer and we never write to it, so the audio
        // reaches the headphones bit for bit. We only read a copy.
        if (buffer.getNumChannels() < 2)
            return;

        bridge_.process(buffer.getReadPointer(0), buffer.getReadPointer(1), static_cast<uint32_t>(buffer.getNumSamples()));

        if (streamSound())
            stream_.micCaptured(buffer.getReadPointer(0), buffer.getReadPointer(1), buffer.getNumSamples());
    }

    using AudioProcessor::processBlock;

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

private:
    // Extra headroom: Ableton hands over audio in large blocks, and stream delay doesn't matter.
    static constexpr auto kStreamLatency = midi4u::Latency::safe;

    void timerCallback() override
    {
        const auto now = nowMs();
        bridge_.tick(now);
        updateStream(now);
    }

    void updateStream(uint64_t now)
    {
        if (! streamSound())
        {
            streamSupervisor_.reset(); // closes the device
            streamDevice_.reset();
            return;
        }

        if (streamDevice_ == nullptr)
        {
            streamDevice_ = std::make_unique<JuceDevice>(static_cast<midi4u::OutputCallback&>(stream_));
            streamSupervisor_ = std::make_unique<midi4u::DeviceSupervisor>(*streamDevice_);
            streamSupervisor_->setBufferMs(midi4u::latencySettings(kStreamLatency).outputBufferMs, now);
            nextCableLookMs_ = 0;
        }

        // Until the cable shows up, look for it every few seconds.
        if (! streamSupervisor_->selected() && now >= nextCableLookMs_)
        {
            nextCableLookMs_ = now + midi4u::DeviceSupervisor::kRetryMs;

            if (const auto cable = midi4u::findByPrefix(streamDevice_->listDevices(), platform::cableOutputPrefix().toStdString()))
                streamSupervisor_->select(cable, now);
        }

        streamSupervisor_->tick(now);
    }

    midi4u::TapPluginSide bridge_;

    // Declared in this order so the device stops before the engine it calls goes away.
    midi4u::Engine stream_;
    std::unique_ptr<JuceDevice> streamDevice_;
    std::unique_ptr<midi4u::DeviceSupervisor> streamSupervisor_;
    std::atomic<bool> streamWanted_{false};
    uint64_t nextCableLookMs_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapProcessor)
};

class TapEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TapEditor(TapProcessor& owner) : AudioProcessorEditor(owner), processor_(owner)
    {
        streamButton_.setToggleState(owner.streamSound(), juce::dontSendNotification);
        streamButton_.onClick = [this] { processor_.setStreamSound(streamButton_.getToggleState()); };
        streamButton_.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        addAndMakeVisible(streamButton_);

        setSize(300, 130);
        startTimerHz(10);
    }

    void resized() override { streamButton_.setBounds(12, 80, 276, 24); }

    void paint(juce::Graphics& g) override
    {
        using Status = midi4u::TapPluginStatus;
        juce::Colour light;
        juce::String text;

        switch (status_)
        {
            case Status::connected:        light = juce::Colour(0xff3ccf6e); text = "Connected to Midi4u"; break;
            case Status::waitingForApp:    light = juce::Colour(0xfff0b429); text = "Waiting for the Midi4u app"; break;
            case Status::anotherTapActive: light = juce::Colour(0xff8a8f98); text = "Another Tap is already active"; break;
            case Status::versionMismatch:  light = juce::Colour(0xffe5484d); text = "Please update Midi4u"; break;
            case Status::noBridge:         light = juce::Colour(0xffe5484d); text = "Cannot reach Midi4u"; break;
        }

        g.fillAll(juce::Colour(0xff1f2126));

        const auto area = getLocalBounds().removeFromTop(80).reduced(16);
        g.setColour(light);
        g.fillEllipse(float(area.getX()), float(area.getCentreY() - 8), 16.0f, 16.0f);

        auto textArea = area.withTrimmedLeft(30);
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        g.drawText("Midi4u Tap", textArea.removeFromTop(area.getHeight() / 2), juce::Justification::bottomLeft);
        g.setColour(juce::Colour(0xffc8ccd4));
        g.setFont(juce::FontOptions(14.0f));
        g.drawText(text, textArea, juce::Justification::topLeft);

        g.setFont(juce::FontOptions(13.0f));
        g.drawText(streamText_, 46, 104, 242, 18, juce::Justification::centredLeft);
    }

private:
    void timerCallback() override
    {
        const auto status = processor_.bridge().status();
        const auto streamText = processor_.streamStatus();

        if (status != status_ || streamText != streamText_)
        {
            status_ = status;
            streamText_ = streamText;
            repaint();
        }

        // The host may restore a saved project while the window is open.
        streamButton_.setToggleState(processor_.streamSound(), juce::dontSendNotification);
    }

    TapProcessor& processor_;
    midi4u::TapPluginStatus status_ = processor_.bridge().status();
    juce::String streamText_ = processor_.streamStatus();
    juce::ToggleButton streamButton_{"Sound for Discord stream"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapEditor)
};

juce::AudioProcessorEditor* TapProcessor::createEditor()
{
    return new TapEditor(*this);
}
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TapProcessor();
}
