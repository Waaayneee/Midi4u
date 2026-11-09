#!/bin/bash
# Updates Midi4u to the newest GitHub release. Double-click this file in Finder.
# Keep it in your Midi4u-macos folder. Your settings are kept.

# Everything is inside main, so bash has read the whole script before the update replaces this file.
main() {
    cd "$(dirname "$0")" || exit 1
    repo="Waaayneee/Midi4u"

    stop() {
        echo
        echo "$1"
        echo "You can close this window."
        exit 1
    }

    echo "Checking GitHub for the newest Midi4u..."
    version=$(curl -fsSL "https://api.github.com/repos/$repo/releases/latest" |
        sed -n 's/.*"tag_name": *"\([^"]*\)".*/\1/p')
    [ -n "$version" ] || stop "Couldn't reach GitHub. Check your internet connection and try again."

    if [ "$(cat version.txt 2>/dev/null)" = "$version" ]; then
        stop "Midi4u is already up to date ($version)."
    fi

    temp=$(mktemp -d) || stop "Couldn't make a temporary folder."
    trap 'rm -rf "$temp"' EXIT

    # Files curl downloads aren't marked "from the internet", so macOS doesn't block this update.
    echo "Downloading Midi4u $version..."
    curl -fsSL -o "$temp/update.zip" "https://github.com/$repo/releases/download/$version/Midi4u-macos.zip" ||
        stop "Download failed. Try again later."
    ditto -x -k "$temp/update.zip" "$temp" && [ -d "$temp/Midi4u-macos/Midi4u.app" ] ||
        stop "The download is damaged. Try again later."

    # Replace this folder's copies, so the folder always holds the newest version.
    rm -rf "Midi4u.app" "Midi4u Tap.vst3"
    ditto "$temp/Midi4u-macos" . || stop "Couldn't update this folder."

    bash "./Install Midi4u (click me).command" || exit 1
    echo "$version" > version.txt
}

main "$@"
exit
