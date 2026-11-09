#!/bin/bash
# Installs Midi4u on this Mac, or updates it. Double-click this file in Finder.
# To update later, download Midi4u again and double-click the new copy of this file.
# Your settings are kept.

cd "$(dirname "$0")" || exit 1

stop() {
    echo
    echo "$1"
    echo "You can close this window."
    exit 1
}

if [ ! -d "Midi4u.app" ] || [ ! -d "Midi4u Tap.vst3" ]; then
    stop "Keep this file in the unzipped Midi4u-macos folder, next to Midi4u.app and Midi4u Tap.vst3, then double-click it again. Nothing was changed."
fi

apps="/Applications"
[ -w "$apps" ] || apps="$HOME/Applications" # accounts that can't write to /Applications
plugins="$HOME/Library/Audio/Plug-Ins/VST3"

echo "Installing Midi4u..."

# Close Midi4u so it can be replaced.
if pgrep -x Midi4u >/dev/null; then
    pkill -x Midi4u
    sleep 1
fi

mkdir -p "$apps" "$plugins" || stop "Couldn't create $apps or $plugins."
rm -rf "$apps/Midi4u.app" "$plugins/Midi4u Tap.vst3"
ditto "Midi4u.app" "$apps/Midi4u.app" || stop "Couldn't copy Midi4u.app. Download Midi4u again and retry."
ditto "Midi4u Tap.vst3" "$plugins/Midi4u Tap.vst3" || stop "Couldn't copy Midi4u Tap.vst3. Download Midi4u again and retry."

# The download marked the files as "from the internet". Clearing that mark stops macOS
# blocking Midi4u, and lets Ableton load the plug-in.
xattr -dr com.apple.quarantine "$apps/Midi4u.app" "$plugins/Midi4u Tap.vst3" 2>/dev/null
xattr -d com.apple.quarantine "Update Midi4u (click me).command" 2>/dev/null # so the updater opens without asking

echo "  Midi4u.app       -> $apps"
echo "  Midi4u Tap.vst3  -> $plugins"
echo
echo "Done! Midi4u is opening now."
echo "If Ableton is open, restart it. The first time, go to Live > Settings > Plug-Ins and click Rescan."
echo "You can close this window."
open "$apps/Midi4u.app"
