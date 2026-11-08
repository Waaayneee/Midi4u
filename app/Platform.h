#pragma once

#include <juce_core/juce_core.h>

#include <string>
#include <vector>

// Everything that differs between Windows (v1) and macOS (used for development, v3 later).
namespace platform
{

// Name prefix of the virtual cable's playback side that Discord listens to.
juce::String cableOutputPrefix();     // "CABLE Input" / "BlackHole 2ch"
juce::String cableDiscordInputName(); // what to pick in Discord: "CABLE Output" / "BlackHole 2ch"
juce::URL cableDownloadPage();

// Where Midi4u Tap.vst3 should be installed.
juce::File vst3Folder();
inline juce::File installedPlugin() { return vst3Folder().getChildFile("Midi4u Tap.vst3"); }

// %APPDATA%\Midi4u on Windows, ~/Library/Application Support/Midi4u on macOS.
juce::File appDataFolder();

bool supportsStartWithSystem();
void setStartWithSystem(bool enabled);

// System IDs for the device names JUCE lists, in the same order. Empty strings where the
// platform has no ID, in which case devices are matched by name.
std::vector<std::string> systemDeviceIds(bool input, const juce::StringArray& juceNames);

} // namespace platform
