#include "DeviceRules.h"

#include <algorithm>
#include <cctype>

namespace midi4u
{

namespace
{
std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return s;
}

bool contains(const std::string& haystack, const char* needle)
{
    return haystack.find(needle) != std::string::npos;
}
} // namespace

bool wouldFeedBack(const std::string& micName, const std::string& outputName)
{
    const std::string mic = lower(micName);

    // VB-Cable capture sides: "CABLE Output", "CABLE-A Output", "CABLE-B Output"
    if (mic.rfind("cable", 0) == 0 && contains(mic, "output"))
        return true;

    if (contains(mic, "stereo mix") || contains(mic, "what u hear") || contains(mic, "wave out mix"))
        return true;

    return ! outputName.empty() && mic == lower(outputName);
}

std::vector<DeviceInfo> usableMics(const std::vector<DeviceInfo>& mics, const std::string& outputName)
{
    std::vector<DeviceInfo> result;

    for (const auto& mic : mics)
        if (! wouldFeedBack(mic.name, outputName))
            result.push_back(mic);

    return result;
}

std::optional<DeviceInfo> findSavedDevice(const DeviceInfo& saved, const std::vector<DeviceInfo>& available)
{
    if (! saved.id.empty())
        for (const auto& device : available)
            if (device.id == saved.id)
                return device;

    if (! saved.name.empty())
        for (const auto& device : available)
            if (device.name == saved.name)
                return device;

    return std::nullopt;
}

std::optional<DeviceInfo> findByPrefix(const std::vector<DeviceInfo>& devices, const std::string& prefix)
{
    const std::string wanted = lower(prefix);

    for (const auto& device : devices)
        if (lower(device.name).rfind(wanted, 0) == 0)
            return device;

    return std::nullopt;
}

} // namespace midi4u
