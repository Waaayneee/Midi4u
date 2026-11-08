#pragma once

#include "AudioEndpoint.h"

#include <optional>
#include <string>
#include <vector>

namespace midi4u
{

// True when using this capture device as the mic would feed the mix back into itself:
// the capture side of a virtual cable ("CABLE Output"), loopback devices ("Stereo Mix"),
// or the same device the mix is being sent to (BlackHole is both input and output).
bool wouldFeedBack(const std::string& micName, const std::string& outputName);

// The mics that may be offered in the dropdown for this output.
std::vector<DeviceInfo> usableMics(const std::vector<DeviceInfo>& mics, const std::string& outputName);

// Finds a saved device again: by system ID first, then by exact name.
// Never returns a different device, so a missing mic stays missing.
std::optional<DeviceInfo> findSavedDevice(const DeviceInfo& saved, const std::vector<DeviceInfo>& available);

// First device whose name starts with prefix, ignoring case.
std::optional<DeviceInfo> findByPrefix(const std::vector<DeviceInfo>& devices, const std::string& prefix);

} // namespace midi4u
