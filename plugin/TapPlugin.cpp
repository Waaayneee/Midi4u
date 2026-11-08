// Midi4u Tap: sits on Ableton's Master track, passes audio through untouched and
// copies it to the Midi4u App through shared memory. All bridge logic lives in
// core/TapPluginSide so it is covered by the tests.

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
        startTimerHz(20);
    }

    ~TapProcessor() override { stopTimer(); }

    midi4u::TapPluginSide& bridge() { return bridge_; }

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

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
            && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }

    void prepareToPlay(double sampleRate, int) override { bridge_.setSampleRate(sampleRate); }
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        // Input and output share this buffer and we never write to it, so the audio
        // reaches the headphones bit for bit. We only read a copy.
        if (buffer.getNumChannels() >= 2)
            bridge_.process(buffer.getReadPointer(0), buffer.getReadPointer(1),
                            static_cast<uint32_t>(buffer.getNumSamples()));
    }

    using AudioProcessor::processBlock;

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

private:
    void timerCallback() override { bridge_.tick(nowMs()); }

    midi4u::TapPluginSide bridge_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapProcessor)
};

class TapEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TapEditor(TapProcessor& owner) : AudioProcessorEditor(owner), processor_(owner)
    {
        setSize(300, 80);
        startTimerHz(10);
    }

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

        const auto area = getLocalBounds().reduced(16);
        g.setColour(light);
        g.fillEllipse(float(area.getX()), float(area.getCentreY() - 8), 16.0f, 16.0f);

        auto textArea = area.withTrimmedLeft(30);
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        g.drawText("Midi4u Tap", textArea.removeFromTop(area.getHeight() / 2), juce::Justification::bottomLeft);
        g.setColour(juce::Colour(0xffc8ccd4));
        g.setFont(juce::FontOptions(14.0f));
        g.drawText(text, textArea, juce::Justification::topLeft);
    }

private:
    void timerCallback() override
    {
        const auto status = processor_.bridge().status();

        if (status != status_)
        {
            status_ = status;
            repaint();
        }
    }

    TapProcessor& processor_;
    midi4u::TapPluginStatus status_ = processor_.bridge().status();

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
