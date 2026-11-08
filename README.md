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

## Layout

| Folder | What |
|---|---|
| `core/` | Ring buffer, shared memory bridge, Sync (resampling and drift), Mixer, device rules and supervision. No UI code. |
| `plugin/` | Midi4u Tap, a thin JUCE wrapper around `core/TapPluginSide` |
| `app/` | Midi4u App (tray app, setup, manual mode) and the JUCE device sink |
| `tests/` | Catch2 unit tests (`[unit]`) and integration tests (`[integration]`) |
| `libs/` | JUCE 9.0.5, Catch2 3.16.1, libsamplerate 0.2.2 (git submodules) |

## Build

Needs CMake 3.22+ and a C++20 compiler (Visual Studio 2022 or newer on Windows, Xcode tools on macOS).

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

Options: `-DMIDI4U_BUILD_JUCE=OFF` builds only core and tests (fast). `-DMIDI4U_RTSAN=ON`
turns on Clang RealtimeSanitizer for the audio callbacks; it needs upstream LLVM 20 or newer,
since Apple's clang does not include it.

CI (GitHub Actions, Windows) builds everything, runs the tests and runs pluginval at strictness 10.

## Setting up the friend's laptop

1. Install [VB-Cable](https://vb-audio.com/Cable/) and restart if asked.
2. Copy `Midi4u Tap.vst3` into `C:\Program Files\Common Files\VST3`.
3. Run `Midi4u.exe`. The first launch opens **Set up Midi4u**, which walks through the rest:
   drag Midi4u Tap onto Ableton's Master track, pick the mic, send the mix to "CABLE Input",
   set Discord's input to "CABLE Output" and turn off its noise suppression, echo
   cancellation and automatic gain control, then play the test tone.

Settings live in `%APPDATA%\Midi4u\settings.json` and logs in `%APPDATA%\Midi4u\logs`
(the newest five are kept). Send the latest log when something goes wrong.

On macOS (development) the same app works with [BlackHole 2ch](https://existential.audio/blackhole/)
in place of VB-Cable, and the plugin goes in `~/Library/Audio/Plug-Ins/VST3`.

## Notes

* WASAPI shared mode always uses the driver's period (about 10 ms), so the Latency setting
  changes the ring buffer targets on Windows but not the output buffer size.
* JUCE is licensed under AGPLv3 or a commercial JUCE licence. A private build is fine; a
  public v2 release needs a licence decision.
