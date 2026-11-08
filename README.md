# Midi4u

Lets Discord viewers hear Ableton Live (played through ASIO) together with your voice,
without adding any latency to your own headphones.

```
Ableton Master ─ Midi4u Tap (VST3) ─ untouched ─→ ASIO → Scarlett → headphones
                      │ copy (shared memory)
                      ▼
                Midi4u App (tray): music + mic → limiter → CABLE Input
                                                             │
                                          Discord input = "CABLE Output"
```

## How to install
### What you need

| Item | Notes |
|---|---|
| A Windows 10 or Windows 11 PC (64 bit) | The laptop you make music on |
| Ableton Live 12 | Any edition that can load VST3 plug-ins |
| Your Focusrite Scarlett, with its driver installed | Ableton should already be using it ("Focusrite USB ASIO") |
| A mic that Windows can see | A headset mic, a USB mic, or the laptop's built-in mic |
| The Discord desktop app | Download from [discord.com/download](https://discord.com/download) |
| VB-Cable (free) | Installed in step 1 below |
| The two Midi4u files | `Midi4u.exe` and the `Midi4u Tap.vst3` folder, from step 2 below |

### Step 1: Install VB-Cable

VB-Cable adds a "virtual cable" to Windows: Midi4u plays the mix into one end, and Discord
listens to the other end as if it were a microphone.

1. Open [vb-audio.com/Cable](https://vb-audio.com/Cable/) and click the **Download** button
   for Windows. You get a zip file with a name like `VBCABLE_Driver_Pack45.zip`.
2. Open your **Downloads** folder, right-click the zip file and choose **Extract All…**, then **Extract**.
3. In the extracted folder, right-click **VBCABLE_Setup_x64.exe** and choose **Run as administrator**.
   Click **Yes** when Windows asks.
4. Click **Install Driver**, wait for the message that it is done, then click **OK**.
5. **Restart your computer.**
6. Check that it worked: right-click the speaker icon at the bottom right of the screen,
   choose **Sound settings**, and look under **Output**. You should see
   **CABLE Input (VB-Audio Virtual Cable)**.
7. Important: on that same page, make sure your normal speakers or headphones are still
   selected as the output, **not** "CABLE Input". VB-Cable sometimes makes itself the default
   after installing. If it did, click your normal speakers or headphones to switch back.

### Step 2: Get the Midi4u files

1. Open [the newest Midi4u release](https://github.com/Waaayneee/Midi4u/releases/latest).
   You don't need a GitHub account.
2. Under **Assets**, click **Midi4u-windows.zip** to download it.

Then:

1. Right-click the zip and choose **Properties**. If you see an **Unblock** box at the
   bottom, tick it and click **OK**. This stops Windows from warning about the files later.
2. Right-click the zip, choose **Extract All…**, then **Extract**. Inside you will find
   `Midi4u.exe`, a folder called `Midi4u Tap.vst3`, `Update Midi4u.bat` and `update.ps1`.

### Step 3: Install the Midi4u Tap plug-in

1. Open File Explorer, click in the address bar at the top, type
   `C:\Program Files\Common Files` and press **Enter**.
2. If there is no folder called **VST3**, right-click an empty space, choose
   **New > Folder**, and name it `VST3`. Click **Continue** if Windows asks for permission.
3. Copy the whole **Midi4u Tap.vst3** folder (from the extracted zip) into that **VST3** folder.
   Click **Continue** if Windows asks for permission.

The result should be `C:\Program Files\Common Files\VST3\Midi4u Tap.vst3`.

### Step 4: Put the Midi4u app somewhere permanent

1. Open your **Documents** folder and create a new folder called `Midi4u`.
2. Move `Midi4u.exe`, `Update Midi4u.bat` and `update.ps1` into it. Don't run Midi4u from the
   Downloads folder, because "Start with Windows" remembers where the file is.
3. Optional: right-click `Midi4u.exe`, choose **Show more options > Send to > Desktop
   (create shortcut)** to get a desktop icon.

### Step 5: Let Ableton find the plug-in

1. Open Ableton Live.
2. Open **Options > Settings** and click the **Plug-Ins** tab.
3. Turn on **Use VST3 Plug-In System Folders**.
4. Click **Rescan**, wait a few seconds, then close the Settings window.

## How to run and use

### First time: setting up

1. Double-click **Midi4u.exe**.
   * If Windows shows **"Windows protected your PC"**, click **More info**, then **Run anyway**.
     This happens because the app is not code signed yet. You only need to do it once.
2. The first time, Midi4u shows a short **Set up Midi4u** list. Here is each point in detail:

   * **Virtual cable**: you did this in [Step 1](#step-1-install-vb-cable).
   * **Midi4u Tap plug-in**: in Ableton, click the **Master** track (the track at the bottom
     or far right). In the browser on the left, click **Plug-Ins**, open **VST3**, open
     **Midi4u**, and drag **Midi4u Tap** onto the Master track. A small window appears with
     a green light that says "Connected to Midi4u". If you can't find it, redo
     [Step 5](#step-5-let-ableton-find-the-plug-in).
   * **Mic and output**: Midi4u picks Windows' default microphone and **CABLE Input** by
     itself. You can change them in the main window after the next step.
   * **Discord settings**: open Discord, click the gear icon (**User Settings**) next to your
     name at the bottom left, then click **Voice & Video**. Change these:
     * **Input Device**: choose **CABLE Output (VB-Audio Virtual Cable)**.
     * **Output Device**: choose your Scarlett (it may be called "Focusrite USB Audio" or
       "Speakers (Focusrite…)").
     * **Noise Suppression**: set to **None**. **Echo Cancellation**: turn off.
       **Automatic Gain Control**: turn off. (Scroll down to find these. They are made for
       voices and would cut or warp the music.)
     * **Input Sensitivity**: turn off "Automatically determine input sensitivity", then
       drag the slider all the way to the left, so quiet music is never cut off.

3. Click **Done** to get to the main window.
4. Check that **Mic input** is your mic (talk and watch the Mic meter move) and that
   **Send mix to** is **CABLE Input**.
5. **Test**: in Discord's **Voice & Video** page, click **Let's Check** (the Mic Test). Then
   click **Play test tone** in Midi4u. You should hear a short beep in Discord. Talk, and you
   should hear yourself too. Click **Let's Check** again to stop the test.
6. Check that all three lights at the top of Midi4u are **green**. Midi4u remembers
   everything for next time.

### Every time you stream

1. Open Ableton and load your project. Midi4u Tap must be on the Master track. Tip: save a
   project with it already on the Master as your template, so you never have to add it again.
2. Open Midi4u (or turn on **Start with Windows** once, and it opens by itself).
3. Check that all three lights at the top of Midi4u are **green**.
4. Join your Discord voice channel and start **Go Live** as usual. If you share your whole
   screen, turn the stream's **Sound** off. Otherwise viewers hear the music and your voice
   twice. (Want the music on the stream instead? See the next section.)
5. Play. Your headphones sound exactly as before. People in the call hear the music and your voice.

### Voice only in voice chat, music only on the stream

Use this if you **Go Live** and want the music to come through the stream instead of voice
chat, so nobody hears anything twice.

Why it needs a setting: Ableton plays through ASIO, which Discord can't hear. So when you
share the Ableton window, the stream is silent. The **Sound for Discord stream** box makes
Midi4u Tap also play the music from inside Ableton, where Discord can hear it.

1. In Ableton, click the **Midi4u Tap** plug-in on the Master track to open its window, and
   tick **Sound for Discord stream**. Under it you should see **Playing into CABLE Input**.
   The box is saved with your Ableton project, so save it in your template too.
2. In Discord, open **User Settings > Voice & Video** and set **Input Device** to your real
   mic (for example the Scarlett), not "CABLE Output". You can turn **Noise Suppression**,
   **Echo Cancellation** and **Automatic Gain Control** back on, since it's only your voice now.
3. You don't need the Midi4u app for this. You can quit it (right-click the **M** icon >
   **Quit Midi4u**). The Tap's light then turns yellow, which is fine.
4. Start **Go Live**, choose the **Ableton Live** window, and make sure **Sound** is on.

If the stream is still silent, share your **whole screen** with **Sound** on instead, with
the Midi4u app quit. Other sounds on your PC (YouTube, notifications) then go into the stream too.

To go back to music in voice chat: untick **Sound for Discord stream**, set Discord's Input
Device back to **CABLE Output**, turn those three Discord settings off again, and open the
Midi4u app.

Don't leave Discord's Input Device on **CABLE Output** while the box is ticked. Voice chat
would get the music too.

Note: the stream runs about half a second to a second behind voice chat. Viewers hear your
voice a little before the music. That's fine for talking between songs, but if you sing or
play along, it will sound out of time. Keep the music in voice chat for that.

### The main window

| Control | What it does |
|---|---|
| **Plugin** light | Green: music is arriving from Ableton. Red: Ableton is closed, or Midi4u Tap is not on the Master track. |
| **Mic** light | Green: your mic is working. Red: it was unplugged. Plug it back in and it reconnects by itself. |
| **Output** light | Green: the mix is going to CABLE Input. Red: click **Fix**. |
| **Music** fader and **Mute** | How loud the music is for Discord. It doesn't change what you hear. |
| **Mic** fader and **Mute** | How loud your voice is for Discord. Mute it to play without talking, or for [music on the stream](#voice-only-in-voice-chat-music-only-on-the-stream). |
| **Mic input** | Which mic to use. Devices that would cause an echo loop are hidden. |
| **Send mix to** | Where the mix goes. Keep it on **CABLE Input**. |
| **Latency** | Leave on **Normal**. Use **Safe** if Discord listeners hear crackles. |
| **Start with Windows** | Opens Midi4u in the background when you log in. |
| **Set up Midi4u** | Shows the setup list again. |
| **Play test tone** | Plays a 2 second beep into the mix. |

Closing the window keeps Midi4u running. It lives as an **M** icon near the clock at the bottom
right (click the **^** arrow if you don't see it). Click the icon to open the window again.
Right-click it and choose **Quit Midi4u** to close it completely.

### Updating Midi4u

1. Close Ableton Live.
2. Open your `Documents\Midi4u` folder and double-click **Update Midi4u.bat**.
3. Click **Yes** when Windows asks. The updater needs this to update the plug-in in Program Files.
4. Wait for "Midi4u is updated" (or "already up to date"), then press **Enter**.

The updater downloads the newest release from GitHub. It updates both the app and the
plug-in, and keeps your settings. If Midi4u was open, it restarts it.

### If something goes wrong

| Problem | What to do |
|---|---|
| Listeners hear nothing at all | Check that Discord's Input Device is **CABLE Output** and that Midi4u's Output light is green. Make sure you aren't muted in Discord. |
| Listeners hear your voice but no music | The Plugin light is probably red. Put **Midi4u Tap** on Ableton's **Master** track, not on a normal track. |
| Music cuts in and out | Discord's Input Sensitivity is probably still automatic. Set it manually to the lowest level. |
| Music sounds underwater or warbly | Turn off Noise Suppression, Echo Cancellation and Automatic Gain Control in Discord. |
| Stream viewers hear your voice twice | You're sharing your whole screen with sound while the Midi4u app sends your voice. Turn the stream's Sound off, or follow [Voice only in voice chat, music only on the stream](#voice-only-in-voice-chat-music-only-on-the-stream). |
| Stream has no sound when sharing the Ableton window | Tick **Sound for Discord stream** in the Midi4u Tap window. See [Voice only in voice chat, music only on the stream](#voice-only-in-voice-chat-music-only-on-the-stream). |
| Midi4u Tap says "CABLE Input not found" | VB-Cable isn't installed. Do [Step 1](#step-1-install-vb-cable). |
| Crackles for listeners | Set Midi4u's **Latency** to **Safe**. |
| You can't hear YouTube or Windows sounds any more, and listeners hear them instead | Windows' default output got set to "CABLE Input". Switch it back to your speakers or Scarlett in Sound settings. |
| Your mic isn't in the Mic input list | Plug it in, wait a few seconds and open the list again. "CABLE Output" and "Stereo Mix" are hidden on purpose. |
| Midi4u Tap says "Another Tap is already active" | You added it twice. Remove the extra one; only the Master track needs it. |
| Something says "Please update Midi4u" | The plug-in and the app come from different versions. Close Ableton and run **Update Midi4u.bat**. |

Still stuck? Send Wayne the newest file from `%APPDATA%\Midi4u\logs` (paste that into the
File Explorer address bar to open the folder).

## How it works

* **Midi4u Tap** sits on Ableton's Master track. It passes the audio through untouched,
  so your headphones get exactly what they got before, and it copies each block into shared
  memory. It never waits, so even if the app crashes, Ableton keeps playing normally.
* **Midi4u App** reads that copy, resamples it to follow the output device's clock (Ableton
  and VB-Cable run on different clocks, which would otherwise drift apart over an hour), mixes
  in your mic, runs a limiter that keeps the mix under −1 dBFS, and plays it into CABLE Input.
* Discord records from **CABLE Output**, the other end of the virtual cable.

The Discord path adds roughly 20 to 40 ms of delay. Your own headphones get 0 ms extra.

## For developers

### Layout

| Folder | What |
|---|---|
| `core/` | Ring buffer, shared memory bridge, Sync (resampling and drift), Mixer, device rules and supervision. No UI code. |
| `plugin/` | Midi4u Tap, a thin JUCE wrapper around `core/TapPluginSide`. Its optional stream sound reuses `core/Engine` and `app/JuceDevice` |
| `app/` | Midi4u App (tray app, setup, manual mode) and the JUCE device sink |
| `updater/` | `Update Midi4u.bat` and `update.ps1`, shipped in the release zip |
| `tests/` | Catch2 unit tests (`[unit]`) and integration tests (`[integration]`) |
| `libs/` | JUCE 9.0.5, Catch2 3.16.1, libsamplerate 0.2.2 (git submodules) |

### Build

Needs CMake 3.22 or newer and a C++20 compiler (Visual Studio 2022 or newer on Windows, Xcode
command line tools on macOS).

```bash
git submodule update --init --recursive
```

```bash
cmake -S . -B build
```

```bash
cmake --build build --config Release --parallel
```

```bash
ctest --test-dir build -C Release --output-on-failure
```

Outputs:

* `build/plugin/Midi4uTap_artefacts/Release/VST3/Midi4u Tap.vst3`
* `build/app/Midi4u_artefacts/Release/Midi4u.exe` (or `Midi4u.app` on macOS)

Options: `-DMIDI4U_BUILD_JUCE=OFF` builds only core and tests (fast).
`-DMIDI4U_RTSAN=ON` turns on Clang RealtimeSanitizer for the audio callbacks. It needs
upstream LLVM 20 or newer, because Apple's clang doesn't include it.

CI (GitHub Actions, Windows) builds everything, runs the tests, runs pluginval at strictness
10 and uploads a `Midi4u-windows` zip as a build artifact.

### Releasing

The updater installs the newest [GitHub release](https://github.com/Waaayneee/Midi4u/releases).
To publish one, bump `VERSION` in the root `CMakeLists.txt`, commit, then push a tag. CI
builds, tests and attaches `Midi4u-windows.zip` to a new release.

```bash
git tag v0.1.0
```

```bash
git push origin v0.1.0
```

### Trying it on a Mac

The same app runs on macOS for development. Use
[BlackHole 2ch](https://existential.audio/blackhole/) instead of VB-Cable. Copy the plug-in to
`~/Library/Audio/Plug-Ins/VST3`, and pick "BlackHole 2ch" as Discord's input. Settings and
logs are in `~/Library/Application Support/Midi4u`.

### Notes

* WASAPI shared mode always uses the driver's period (about 10 ms), so on Windows the Latency
  setting changes the app's internal buffering but not the output buffer size.
* JUCE is licensed under AGPLv3 or a commercial JUCE licence. A private build is fine. A
  public release needs a licence decision.
