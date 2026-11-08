#include "JuceDevice.h"

#include "Platform.h"

namespace
{
std::unique_ptr<juce::AudioIODeviceType> createDeviceType()
{
#if JUCE_WINDOWS
    return std::unique_ptr<juce::AudioIODeviceType>(
        juce::AudioIODeviceType::createAudioIODeviceType_WASAPI(juce::WASAPIDeviceMode::shared));
#elif JUCE_MAC
    return std::unique_ptr<juce::AudioIODeviceType>(juce::AudioIODeviceType::createAudioIODeviceType_CoreAudio());
#else
    return nullptr;
#endif
}
} // namespace

JuceDevice::JuceDevice(midi4u::OutputCallback& output) : output_(&output), type_(createDeviceType()) {}
JuceDevice::JuceDevice(midi4u::MicCallback& mic) : mic_(&mic), type_(createDeviceType()) {}

JuceDevice::~JuceDevice()
{
    close();
}

std::vector<midi4u::DeviceInfo> JuceDevice::listDevices()
{
    std::vector<midi4u::DeviceInfo> devices;

    if (type_ == nullptr)
        return devices;

    type_->scanForDevices();
    const auto names = type_->getDeviceNames(isInput());
    const auto ids = platform::systemDeviceIds(isInput(), names);

    for (int i = 0; i < names.size(); ++i)
        devices.push_back({ids[size_t(i)], names[i].toStdString()});

    return devices;
}

std::optional<midi4u::DeviceInfo> JuceDevice::defaultDevice()
{
    const auto devices = listDevices();
    const int index = type_ != nullptr ? type_->getDefaultDeviceIndex(isInput()) : -1;

    if (index >= 0 && index < int(devices.size()))
        return devices[size_t(index)];

    return std::nullopt;
}

std::string JuceDevice::open(const midi4u::DeviceInfo& info, double bufferMs)
{
    close();

    if (type_ == nullptr)
        return "Audio devices are not supported on this system";

    type_->scanForDevices();
    const juce::String name(info.name);
    std::unique_ptr<juce::AudioIODevice> device(isInput() ? type_->createDevice({}, name) : type_->createDevice(name, {}));

    if (device == nullptr)
        return "Could not open " + info.name;

    // Keep the device's own rate (0 means "its default" for WASAPI), so opening it never
    // changes the system's audio settings.
    const double rate = device->getCurrentSampleRate();
    const int wantedFrames = juce::roundToInt((rate > 0 ? rate : 48000.0) * bufferMs / 1000.0);
    int bufferFrames = 0; // 0 means the device default

    for (const int size : device->getAvailableBufferSizes())
    {
        bufferFrames = size;
        if (size >= wantedFrames)
            break;
    }

    const int numChannels = isInput() ? device->getInputChannelNames().size() : device->getOutputChannelNames().size();

    if (numChannels == 0)
        return info.name + " has no usable channels";

    juce::BigInteger channels;
    channels.setRange(0, juce::jmin(2, numChannels), true);

    const auto error = isInput() ? device->open(channels, {}, rate, bufferFrames)
                                 : device->open({}, channels, rate, bufferFrames);

    if (error.isNotEmpty())
        return error.toStdString();

    device_ = std::move(device);
    device_->start(this);
    return {};
}

void JuceDevice::close()
{
    if (device_ != nullptr)
    {
        device_->stop(); // calls audioDeviceStopped
        device_->close();
        device_.reset();
    }
}

void JuceDevice::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const double rate = device->getCurrentSampleRate();
    const int maxFrames = juce::jmax(1, device->getCurrentBufferSizeSamples());
    scratch_.assign(size_t(juce::jmax(maxFrames, 4096)), 0.0f);

    if (output_ != nullptr)
        output_->outputStarting(rate, maxFrames);
    else
        mic_->micStarting(rate, maxFrames);
}

void JuceDevice::audioDeviceIOCallbackWithContext(const float* const* inputs, int numInputs, float* const* outputs,
                                                  int numOutputs, int numFrames, const juce::AudioIODeviceCallbackContext&)
{
    callbacks_.fetch_add(1, std::memory_order_relaxed);

    if (mic_ != nullptr)
    {
        if (numInputs >= 2)
            mic_->micCaptured(inputs[0], inputs[1], numFrames);
        else if (numInputs == 1)
            mic_->micCaptured(inputs[0], inputs[0], numFrames); // mono mic to both channels
    }

    if (output_ != nullptr && numOutputs >= 2)
    {
        output_->renderOutput(outputs[0], outputs[1], numFrames);
    }
    else if (output_ != nullptr && numOutputs == 1)
    {
        for (int done = 0; done < numFrames;)
        {
            const int chunk = juce::jmin(numFrames - done, int(scratch_.size()));
            output_->renderOutput(outputs[0] + done, scratch_.data(), chunk);
            done += chunk;
        }
    }

    for (int ch = (output_ != nullptr ? 2 : 0); ch < numOutputs; ++ch)
        juce::FloatVectorOperations::clear(outputs[ch], numFrames);
}

void JuceDevice::audioDeviceStopped()
{
    if (output_ != nullptr)
        output_->outputStopped();
    else
        mic_->micStopped();
}
