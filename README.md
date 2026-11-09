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

> [!IMPORTANT]
> **Download the zip for your computer.** Both links always get the newest version, and you don't need a GitHub
> account.
>
> | Your computer | Download | Virtual cable | You double-click |
> |---|---|---|---|
> | **Windows 10 or 11** (64-bit) | [Midi4u-windows.zip](https://github.com/Waaayneee/Midi4u/releases/latest/download/Midi4u-windows.zip) | VB-Cable | `Install Midi4u (click me).bat` |
> | 🍎 **Intel Mac** | [Midi4u-macos.zip](https://github.com/Waaayneee/Midi4u/releases/latest/download/Midi4u-macos.zip) | BlackHole 2ch | `Install Midi4u (click me).command` |
> | 🍎 **Apple silicon Mac** (M1, M2, M3, M4…) | [Midi4u-macos.zip](https://github.com/Waaayneee/Midi4u/releases/latest/download/Midi4u-macos.zip) | BlackHole 2ch | `Install Midi4u (click me).command` |
>
> Both Macs use the same download: it runs natively on either chip. **Not sure which Mac you have?** Go to
> **Apple menu → About This Mac**: **Chip: Apple M…** means Apple silicon, **Processor: … Intel** means Intel.

## Contents

<table align="center">
<tr><th width="300" align="center">Get started</th><th width="300" align="center">Use Midi4u</th><th width="300" align="center">Keep it working</th></tr>
<tr><td align="center"><a href="#what-you-need">What you need</a></td><td align="center"><a href="#first-time-the-setup-checklist">First time: the checklist</a></td><td align="center"><a href="#updating-midi4u">Updating</a></td></tr>
<tr><td align="center"><a href="#step-1-install-the-virtual-cable">Install the virtual cable</a></td><td align="center"><a href="#every-time-you-stream">Every time you stream</a></td><td align="center"><a href="#if-something-goes-wrong">If something goes wrong</a></td></tr>
<tr><td align="center"><a href="#step-2-download-midi4u">Download Midi4u</a></td><td align="center"><a href="#voice-only-in-voice-chat-music-only-on-the-stream">Music only on the stream</a></td><td align="center"></td></tr>
<tr><td align="center"><a href="#step-3-install-midi4u">Install Midi4u</a></td><td align="center"><a href="#the-main-window">The main window</a></td><td align="center"></td></tr>
</table>

## How to install

Wherever Windows and Mac differ, the Windows steps are on the left and the 🍎 Mac steps on the right.

### What you need

| Item | Notes |
|---|---|
| A computer | **Windows:** Windows 10 or 11 (64-bit). 🍎 **Mac:** Intel or Apple silicon, macOS 10.15 Catalina or newer |
| Ableton Live 12 | Any edition that can load VST3 plug-ins |
| Your audio interface (for example a Focusrite Scarlett), with its driver installed | **Windows:** Ableton should already be using it ("Focusrite USB ASIO"). 🍎 **Mac:** no driver needed for most interfaces |
| A mic your computer can see | A headset mic, a USB mic, or the built-in mic |
| The Discord desktop app | Download from [discord.com/download](https://discord.com/download) |
| A free virtual cable | **Windows:** VB-Cable. 🍎 **Mac:** BlackHole 2ch. Installed in Step 1 below |

### Step 1: Install the virtual cable

A virtual cable is a pretend sound device: Midi4u plays the mix into one end, and Discord listens to the other end as
if it were a microphone.

<table>
<tr><th width="50%">Windows: VB-Cable</th><th width="50%">🍎 Mac: BlackHole 2ch</th></tr>
<tr><td valign="top">

1. Open [vb-audio.com/Cable](https://vb-audio.com/Cable/) and click the **Download** button for Windows. You get a
   zip file with a name like `VBCABLE_Driver_Pack45.zip`.
2. Open your **Downloads** folder, right-click the zip file and choose **Extract All…**, then **Extract**.
3. In the extracted folder, right-click **VBCABLE_Setup_x64.exe** and choose **Run as administrator**.
   Click **Yes** when Windows asks.
4. Click **Install Driver**, wait for the message that it is done, then click **OK**.
5. **Restart your computer.**
6. Check that it worked: right-click the speaker icon at the bottom right of the screen, choose **Sound settings**,
   and look under **Output**. You should see **CABLE Input (VB-Audio Virtual Cable)**.
7. Important: on that same page, make sure your normal speakers or headphones are still selected as the output,
   **not** "CABLE Input". VB-Cable sometimes makes itself the default after installing. If it did, click your normal
   speakers or headphones to switch back.

</td><td valign="top">

1. Open [existential.audio/blackhole](https://existential.audio/blackhole/). Fill in your name and email (it's free)
   and download **BlackHole 2ch**. Not 16ch or 64ch.
2. Open your **Downloads** folder and double-click the file, which has a name like `BlackHole2ch-….pkg`.
3. Click **Continue** through the installer, then **Install**, and enter your Mac password when asked.
4. Check that it worked: open **System Settings → Sound** and look under **Output**. You should see
   **BlackHole 2ch**.
5. Important: on that same page, make sure your normal speakers or headphones are still selected as the output,
   **not** "BlackHole 2ch". Otherwise you won't hear anything from your Mac.

Use Homebrew? `brew install blackhole-2ch` does the same.

</td></tr>
</table>

### Step 2: Download Midi4u

<table>
<tr><th width="50%">Windows</th><th width="50%">🍎 Mac (Intel and Apple silicon)</th></tr>
<tr><td valign="top">

1. Click [Download Midi4u-windows.zip](https://github.com/Waaayneee/Midi4u/releases/latest/download/Midi4u-windows.zip).
2. Right-click the zip and choose **Properties**. If you see an **Unblock** box at the bottom, tick it and click
   **OK**. This stops Windows from warning about the files later.
3. Right-click the zip, choose **Extract All…**, then **Extract**. Inside you will find:

| File | What it is |
|---|---|
| `Install Midi4u (click me).bat` | **Open this first.** Installs everything |
| `Update Midi4u (click me).bat` | Double-click to update later |
| `Midi4u.exe` | The app |
| `Midi4u Tap.vst3` | The plug-in for Ableton (a folder). The installer copies it for you |
| `update.ps1` | Used by the installer and the updater. Don't open it |

</td><td valign="top">

1. Click [Download Midi4u-macos.zip](https://github.com/Waaayneee/Midi4u/releases/latest/download/Midi4u-macos.zip).
2. Open your **Downloads** folder. Safari may already have unzipped it into a folder called **Midi4u-macos**. If you
   only see `Midi4u-macos.zip`, double-click it. Inside you will find:

| File | What it is |
|---|---|
| `Install Midi4u (click me).command` | **Open this first.** Installs everything |
| `Update Midi4u (click me).command` | Double-click to update later |
| `Midi4u.app` | The app. The installer copies it for you |
| `Midi4u Tap.vst3` | The plug-in for Ableton. The installer copies it for you |

</td></tr>
</table>

### Step 3: Install Midi4u

<table>
<tr><th width="50%">Windows: double-click the installer</th><th width="50%">🍎 Mac: double-click the installer</th></tr>
<tr><td valign="top">

1. Move the extracted **Midi4u-windows** folder into your **Documents** folder and keep it there. Midi4u runs from
   it, and the updater lives in it.
2. Double-click **`Install Midi4u (click me).bat`**.
3. Click **Yes** when Windows asks. The installer needs this to put the plug-in in
   `C:\Program Files\Common Files\VST3`, where Ableton looks for it.
4. A window shows **"Midi4u is installed and opening now"**. Press **Enter** to close it.

Optional: right-click `Midi4u.exe`, choose **Show more options > Send to > Desktop (create shortcut)** to get a
desktop icon.

</td><td valign="top">

1. Move the **Midi4u-macos** folder into your **Documents** folder and keep it there. The updater lives in it.
2. Double-click **`Install Midi4u (click me).command`**.
3. **The first time, macOS blocks it**, because Midi4u isn't registered with Apple. You approve it once:
   * **macOS 15 Sequoia or newer:** click **Done** on the message. Open **System Settings → Privacy & Security**,
     scroll down to **Security**, and next to *"Install Midi4u (click me).command" was blocked*, click
     **Open Anyway**. Enter your Mac password, then click **Open Anyway** again.
   * **macOS 14 or older:** **right-click** (or control-click) the file, choose **Open**, then **Open** again.
4. A Terminal window opens and shows:
   ```
   Installing Midi4u...
     Midi4u.app       -> /Applications
     Midi4u Tap.vst3  -> ~/Library/Audio/Plug-Ins/VST3
   Done! Midi4u is opening now.
   ```
   You can close Terminal.
5. Midi4u opens, and macOS asks **"Midi4u" would like to access the microphone**. Click **Allow**. Midi4u needs this
   to hear your mic.

Don't open `Midi4u.app` from the download folder: the installer has already put it in **Applications** and told macOS
it's safe. From now on, open Midi4u from **Applications** (or press **⌘ Space** and type `Midi4u`).

</td></tr>
</table>

### Step 4: Let Ableton find the plug-in

1. Open Ableton Live.
2. Open the settings: **Options > Settings** on Windows, **Live > Settings** on a 🍎 Mac. Click the **Plug-Ins** tab.
3. Turn on **Use VST3 Plug-In System Folders**.
4. Click **Rescan**, wait a few seconds, then close the Settings window.

## How to run and use

> [!NOTE]
> **On a 🍎 Mac, the virtual cable is called BlackHole 2ch everywhere.** Wherever this guide says **CABLE Input** or
> **CABLE Output**, pick **BlackHole 2ch**.

### First time: the setup checklist

1. Open Midi4u.
   * **Windows:** the installer already opened it. Next time, double-click **Midi4u.exe** in your
     `Documents\Midi4u-windows` folder. If Windows shows **"Windows protected your PC"**, click **More info**, then
     **Run anyway**. This happens because the app is not code signed yet. You only need to do it once.
   * 🍎 **Mac:** the installer already opened it. Next time, open it from **Applications**.
2. The first time, Midi4u opens the **Set up Midi4u** checklist. Each step turns green when it is done. If a step
   stays grey, it shows a button that helps fix it.

   1. **Virtual cable**: green if the virtual cable is installed. If not, click **Download** and follow
      [Step 1](#step-1-install-the-virtual-cable). The step turns green by itself once the cable appears.
   2. **Midi4u Tap plug-in**: in Ableton, click the **Master** track (the track at the bottom or far right). In the
      browser on the left, click **Plug-Ins**, open **VST3**, open **Midi4u**, and drag **Midi4u Tap** onto the
      Master track. A small window appears with a green light that says "Connected to Midi4u", and the step turns
      green. If you can't find it, redo [Step 4](#step-4-let-ableton-find-the-plug-in).
   3. **Mic**: Midi4u picks your computer's default microphone. Talk and check that the green meter moves. Wrong
      mic? Finish the checklist, then pick the right one under **Mic input** in the main window.
   4. **Output**: Midi4u picks **CABLE Input** by itself. If it doesn't, click **Select CABLE Input**.
   5. **Discord settings**: open Discord, click the gear icon (**User Settings**) next to your name at the bottom
      left, then click **Voice & Video**. Change these, ticking each box in Midi4u as you go:
      * **Input Device**: choose **CABLE Output (VB-Audio Virtual Cable)** (🍎 **BlackHole 2ch**).
      * **Output Device**: choose your Scarlett or headphones (on Windows it may be called "Focusrite USB Audio"
        or "Speakers (Focusrite…)").
      * **Noise Suppression**: set to **None**. **Echo Cancellation**: turn off. **Automatic Gain Control**: turn
        off. (Scroll down to find these. They are made for voices and would cut or warp the music.)
      * **Input Sensitivity**: turn off "Automatically determine input sensitivity", then drag the slider all the
        way to the left, so quiet music is never cut off.
   6. **Test**: in Discord's **Voice & Video** page, click **Let's Check** (the Mic Test). Then click **Play test
      tone** in Midi4u. You should hear a short beep in Discord. Talk, and you should hear yourself too. Click
      **Let's Check** again to stop the test.

3. Click **Done**. Midi4u remembers everything for next time.

### Every time you stream

1. Open Ableton and load your project. Midi4u Tap must be on the Master track. Tip: save a project with it already on
   the Master as your template, so you never have to add it again.
2. Open Midi4u. On Windows you can turn on **Start with Windows** once, and it opens by itself.
3. Check that all three lights at the top of Midi4u are **green**.
4. Join your Discord voice channel and start **Go Live** as usual. If you share your whole screen, turn the stream's
   **Sound** off. Otherwise viewers hear the music and your voice twice. (Want the music on the stream instead? See
   the next section.)
5. Play. Your headphones sound exactly as before. People in the call hear the music and your voice.

### Voice only in voice chat, music only on the stream

Use this if you **Go Live** and want the music to come through the stream instead of voice chat, so nobody hears
anything twice.

Why it needs a setting: on Windows, Ableton plays through ASIO, which Discord can't hear. So when you share the
Ableton window, the stream is silent. The **Sound for Discord stream** box makes Midi4u Tap also play the music from
inside Ableton, where Discord can hear it.

1. In Ableton, click the **Midi4u Tap** plug-in on the Master track to open its window, and tick **Sound for Discord
   stream**. Under it you should see **Playing into CABLE Input**. The box is saved with your Ableton project, so save
   it in your template too.
2. In Discord, open **User Settings > Voice & Video** and set **Input Device** to your real mic (for example the
   Scarlett), not "CABLE Output". You can turn **Noise Suppression**, **Echo Cancellation** and **Automatic Gain
   Control** back on, since it's only your voice now.
3. You don't need the Midi4u app for this. You can quit it (from the **M** icon > **Quit Midi4u**). The Tap's light
   then turns yellow, which is fine.
4. Start **Go Live**, choose the **Ableton Live** window, and make sure **Sound** is on.

> [!TIP]
> **🍎 On a Mac, try it without the box first.** Ableton on a Mac doesn't use ASIO, so Discord may already hear the
> Ableton window. Do steps 2 to 4 and listen. Only tick the box if the stream is silent, otherwise viewers hear the
> music twice.

If the stream is still silent, share your **whole screen** with **Sound** on instead, with the Midi4u app quit. Other
sounds on your computer (YouTube, notifications) then go into the stream too.

To go back to music in voice chat: untick **Sound for Discord stream**, set Discord's Input Device back to
**CABLE Output**, turn those three Discord settings off again, and open the Midi4u app.

Don't leave Discord's Input Device on **CABLE Output** while the box is ticked. Voice chat would get the music too.

Note: the stream runs about half a second to a second behind voice chat. Viewers hear your voice a little before the
music. That's fine for talking between songs, but if you sing or play along, it will sound out of time. Keep the music
in voice chat for that.

### The main window

| Control | What it does |
|---|---|
| **Plugin** light | Green: music is arriving from Ableton. Red: Ableton is closed, or Midi4u Tap is not on the Master track. |
| **Mic** light | Green: your mic is working. Red: it was unplugged. Plug it back in and it reconnects by itself. |
| **Output** light | Green: the mix is going to CABLE Input. Red: click **Fix**. |
| **Music** fader and **Mute** | How loud the music is for Discord. It doesn't change what you hear. |
| **Mic** fader and **Mute** | How loud your voice is for Discord. Mute it to play without talking. |
| **Mic input** | Which mic to use. Devices that would cause an echo loop are hidden. |
| **Send mix to** | Where the mix goes. Keep it on **CABLE Input**. |
| **Latency** | Leave on **Normal**. Use **Safe** if Discord listeners hear crackles. |
| **Start with Windows** | Windows only. Opens Midi4u in the background when you log in. |
| **Set up Midi4u** | Opens the checklist again. |
| **Play test tone** | Plays a 2 second beep into the mix. |

Closing the window keeps Midi4u running, as an **M** icon:

<table>
<tr><th width="50%">Windows: system tray</th><th width="50%">🍎 Mac: menu bar</th></tr>
<tr><td valign="top">

The **M** icon sits near the clock at the bottom right (click the **^** arrow if you don't see it). Click it to open
the window again. Right-click it and choose **Quit Midi4u** to close it completely.

</td><td valign="top">

The **M** icon sits in the menu bar at the top right of the screen. Click it for a menu with **Show Midi4u** and
**Quit Midi4u**. If you can't see it, other menu bar icons may be hiding it behind the camera notch.

</td></tr>
</table>

### Updating Midi4u

<table>
<tr><th width="50%">Windows: run the updater</th><th width="50%">🍎 Mac: run the updater</th></tr>
<tr><td valign="top">

1. Close Ableton Live.
2. Open your `Documents\Midi4u-windows` folder and double-click **Update Midi4u (click me).bat**.
3. Click **Yes** when Windows asks. The updater needs this to update the plug-in in Program Files.
4. Wait for "Midi4u is updated" (or "already up to date"), then press **Enter**.

The `.bat` file is the one you double-click. It runs `update.ps1`, which does the actual work. (Windows won't run a
`.ps1` file when you double-click it.) Keep both next to `Midi4u.exe`.

Lost the folder? Download
[Midi4u-windows.zip](https://github.com/Waaayneee/Midi4u/releases/latest/download/Midi4u-windows.zip) again and follow
[Step 3](#step-3-install-midi4u).

It downloads the newest release from GitHub, updates both the app and the plug-in, and keeps your settings. If Midi4u
was open, it restarts it.

</td><td valign="top">

1. Close Ableton Live.
2. Open your `Documents/Midi4u-macos` folder and double-click **Update Midi4u (click me).command**.
3. Wait for "Done!" (or "already up to date"), then close Terminal.

It downloads the newest release from GitHub, updates both the app and the plug-in, and keeps your settings. If Midi4u
was open, it closes it and opens the new one. macOS doesn't ask you to approve anything: the installer already did that.

Lost the folder? Download [Midi4u-macos.zip](https://github.com/Waaayneee/Midi4u/releases/latest/download/Midi4u-macos.zip)
again and follow [Step 3](#step-3-install-midi4u).

</td></tr>
</table>

### If something goes wrong

| Problem | What to do |
|---|---|
| Listeners hear nothing at all | Check that Discord's Input Device is **CABLE Output** and that Midi4u's Output light is green. Make sure you aren't muted in Discord. |
| Listeners hear your voice but no music | The Plugin light is probably red. Put **Midi4u Tap** on Ableton's **Master** track, not on a normal track. |
| Music cuts in and out | Discord's Input Sensitivity is probably still automatic. Set it manually to the lowest level. |
| Music sounds underwater or warbly | Turn off Noise Suppression, Echo Cancellation and Automatic Gain Control in Discord. |
| Stream viewers hear your voice twice | You're sharing your whole screen with sound while the Midi4u app sends your voice. Turn the stream's Sound off, or follow [Voice only in voice chat, music only on the stream](#voice-only-in-voice-chat-music-only-on-the-stream). |
| Stream has no sound when sharing the Ableton window | Tick **Sound for Discord stream** in the Midi4u Tap window. See [Voice only in voice chat, music only on the stream](#voice-only-in-voice-chat-music-only-on-the-stream). |
| Midi4u Tap says "CABLE Input not found" (🍎 "BlackHole 2ch not found") | The virtual cable isn't installed. Do [Step 1](#step-1-install-the-virtual-cable). |
| Crackles for listeners | Set Midi4u's **Latency** to **Safe**. |
| You can't hear YouTube or other sounds any more, and listeners hear them instead | Your computer's default output got set to the virtual cable. Switch it back to your speakers or Scarlett: **Sound settings** on Windows, **System Settings → Sound → Output** on a 🍎 Mac. |
| Your mic isn't in the Mic input list | Plug it in, wait a few seconds and open the list again. "CABLE Output", "Stereo Mix" and 🍎 "BlackHole 2ch" are hidden on purpose. |
| Midi4u Tap says "Another Tap is already active" | You added it twice. Remove the extra one; only the Master track needs it. |
| Something says "Please update Midi4u" | The plug-in and the app come from different versions. Close Ableton and [update Midi4u](#updating-midi4u). |
| 🍎 *"Install Midi4u (click me).command" Not Opened* or *"cannot be opened"* | Expected the first time. Approve it as in [Step 3](#step-3-install-midi4u). |
| 🍎 *"Midi4u.app" is damaged* or *can't be opened* | You opened the app from the download folder. Run `Install Midi4u (click me).command` instead, then open Midi4u from **Applications**. |
| 🍎 The Mic meter doesn't move | Midi4u wasn't allowed to use the mic. Open **System Settings → Privacy & Security → Microphone** and turn on **Midi4u**, then quit and reopen Midi4u. |
| Ableton doesn't list Midi4u Tap | Do [Step 4](#step-4-let-ableton-find-the-plug-in). The plug-in should be in `C:\Program Files\Common Files\VST3` (Windows) or `~/Library/Audio/Plug-Ins/VST3` (Mac). If it isn't, run the installer again. |

Still stuck? Send Wayne the newest log file:

* **Windows:** paste `%APPDATA%\Midi4u\logs` into the File Explorer address bar.
* 🍎 **Mac:** in Finder, choose **Go → Go to Folder…** and paste `~/Library/Application Support/Midi4u/logs`.
