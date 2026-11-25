# RHYTHMER ENGINES for Mac & Windows

**Find a sound. Shape it. Build a pattern.**

22 sound engines, 69 Rhythmer Kits and 3,062 synth preset selections in one
instrument. By Jazzy Rhythmer. **0.1.0 · Desktop Early Beta**.

[Website & installation](https://jazzyrhythmer.github.io/rhythmer-engines/#release) ·
[Download for Mac & Windows](https://github.com/jazzyrhythmer/rhythmer-engines/releases/tag/v0.1.0-desktop-beta.1) ·
[First session](https://jazzyrhythmer.github.io/rhythmer-engines/#guide)

![Rhythmer Engines for Mac & Windows](docs/play.png)

## Compatibility

- Mac: Apple Silicon, macOS 13 or later; AU / VST2 / VST3.
- Windows: Windows 10/11 x64; VST3. Windows DAW compatibility is still being checked.
- A compatible stereo MIDI / instrument host.
- This ZIP uses ad-hoc signing. It is not Developer ID-signed or Apple-notarized;
  macOS or your DAW may block it. See the installation guide if that happens.

## Install

### Mac

1. Download and extract the **Mac AU + VST2 + VST3 ZIP** from the release page. Quit your DAW.
2. In Finder, choose **Go → Go to Folder** (`Shift–Command–G`).
3. Copy `AU/Rhythmer Engines.component` to
   `~/Library/Audio/Plug-Ins/Components/`.
4. Copy `VST2/Rhythmer Engines.vst` to `~/Library/Audio/Plug-Ins/VST/`.
5. Copy `VST3/Rhythmer Engines.vst3` to `~/Library/Audio/Plug-Ins/VST3/`.
   Create the destination folders if needed. You can install any supported format.
6. Reopen/rescan your DAW. Add **Rhythmer Engines** to a MIDI instrument track.

Copy the **complete .component/.vst/.vst3 bundle**, not the files inside it. Keep one
installation location for each format: an older system-wide copy under
`/Library/Audio/Plug-Ins/` may otherwise be scanned instead.

### Windows

1. Quit your DAW. Download the **Windows x64 VST3 ZIP** and use **Extract All**.
2. Copy the entire `VST3/Rhythmer Engines.vst3` folder to
   `C:\Program Files\Common Files\VST3\`.
3. Keep `Contents/x86_64-win` and `Contents/Resources` together. Do not copy only
   the inner plug-in file.
4. Reopen your 64-bit DAW, enable/rescan VST3 and load Rhythmer Engines on an
   instrument track. Set your audio output and MIDI input/monitoring.

In Ableton Live, enable **Use VST3 Plug-In System Folders** under
Settings / Preferences → Plug-Ins, then Rescan. In REAPER, open
Options → Preferences → Plug-ins → VST and Re-scan; use
Insert → Virtual instrument on new track to load it.

Start with **Rhythmer Kit / K-WASUB / 909**. Play MIDI notes **36–51** for its pads,
or select an individual synth with ENGINE and choose a sound with PRESET.

On Mac, if it does not appear, check the plug-in folder, native Apple Silicon host mode
and the host's AU/VST2/VST3 settings, then rescan. For a macOS security message, review
[Apple's Privacy & Security guidance](https://support.apple.com/102445).

## Work with the sound

- **PLAY:** eight main controls and the engine's original 3D animation.
- **EDIT:** full sound, modulation and FX controls, grouped across compact pages.
- **KIT:** 4×4 pads; replace and edit the selected pad's engine and sound.
- **GEN:** create Melody, Chords or Drums MIDI patterns; drag/export the MIDI into
  your DAW for playback at its tempo. There is no separate internal sequencer.
- **ABOUT:** original credits and creator links.

Save your DAW project to retain the instrument state. STOP SOUND releases held
sound; DAW transport stop also clears held notes and sustain. See [GUIDE.md](GUIDE.md).

## Engines

Rhythmer Kit, Sophie, Elixir, Aphex, FM-106, FMini-1, CS-1, OberVave, K-WASUB,
Hera, Braids, Plaits, Hortator, MONO2K, Prophet, S1Vave, Dexed, Guru-600,
Frodhes, Tenebrae, Amino and Fizzik.

[Hear the engines](https://jazzyrhythmer.github.io/rhythmer-engines/#engines) ·
[Original authors and licences](CREDITS.md)

## Windows test and feedback

Try all sixteen kit pads and the new Amino / Fizzik engines. Change presets and
sound controls; check note-responsive animation. Start DAW playback, hold notes
with sustain, then stop and check that held sound stops. Save an edited sound
and kit in a DAW project, close/reopen it and check recall. Generate and import a
MIDI pattern with GEN; the DAW handles playback and tempo.

After installing to the default folder above, close the DAW and run
`Checks/Run-check.cmd` from the extracted ZIP. Wait for `INSTRUMENT PASS` or an
error. It writes `Checks/check-results.txt` beside the tool. This is an offline
engine check, not a speaker or VST3 host test.

Attach that log to an [issue](https://github.com/jazzyrhythmer/rhythmer-engines/issues)
or email jazzyrhythmer@gmail.com. Include Windows / DAW versions, engine / preset,
and the exact steps for any problem. Review logs for local usernames/paths before
sharing them. See [WINDOWS-TEST.md](WINDOWS-TEST.md) for the checklist.

## Source and licences

This repository contains the matching build-source snapshot, including the
custom frontend, native adapters, original engine inputs, JUCE and runtime data.
See [BUILDING.md](BUILDING.md) to build it. A matching full-source ZIP is also
provided with the binary release.

Rhythmer integration is AGPL-3.0; each bundled component retains its own licence
and copyright notices. Read [LICENSE](LICENSE), [CREDITS.md](CREDITS.md), and the
original LICENSE/COPYING/NOTICE files. Retained Melodee components are credited
to Kerem Kilic / Ellic Studio. Engine source identities and notices are also in
`resources-template/SOURCE-INDEX.json` and `resources-template/Licenses/`.

## From the studio

In search of feel-good rhythms. Music, instruments & open-source firmware.
Building JR Audio Software.

[YouTube](https://www.youtube.com/@jazzyrhythmer) ·
[Instagram](https://www.instagram.com/jazzyrhythmer/) ·
[Support on Ko-fi](https://ko-fi.com/jazzyrhythmer) ·
[Patreon](https://www.patreon.com/JazzyRhythmer/) ·
[Contact](mailto:jazzyrhythmer@gmail.com)
