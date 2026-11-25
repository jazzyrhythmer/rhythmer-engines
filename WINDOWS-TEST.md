# Windows installation and DAW test

Windows 10/11 x64. Format: VST3. Use a test DAW project for this early beta.

## Install
1. Quit the DAW. Right-click the Windows ZIP and choose Extract All.
2. Copy the complete `VST3/Rhythmer Engines.vst3` folder into
   `C:\Program Files\Common Files\VST3\`. Approve copy permission if prompted.
3. Keep the entire `Contents` tree, including engines, presets and fonts.
4. Start a 64-bit DAW and rescan VST3 plug-ins. Load Rhythmer Engines on a stereo
   instrument / MIDI track. Select your audio output and MIDI input; enable monitoring.

Ableton Live: Settings / Preferences → Plug-Ins → Use VST3 Plug-In System Folders →
On → Rescan. Search Plug-Ins for Rhythmer Engines and drag it to a MIDI track.
REAPER: Options → Preferences → Plug-ins → VST → Re-scan. Insert → Virtual instrument
on new track → Rhythmer Engines. Arm the track and enable input monitoring.

Factory sounds are selected inside the plug-in with ENGINE and PRESET; the DAW's own preset menu may show Default or saved host presets.

## Test
1. Rhythmer Kit / K-WASUB / 909 loads first. Play MIDI notes 36–51, all sixteen pads.
   DAW octave names differ; note numbers are the reference.
2. Select Amino, Fizzik and other engines. Change PRESET and turn PLAY / EDIT controls.
   Check that animations react to incoming notes rather than remaining a fixed loop.
3. Start DAW playback, hold notes with sustain, then stop. Check held notes stop.
   Use STOP SOUND for an immediate instrument panic.
4. Edit a synth and a kit pad. Save, close and reopen the DAW project. Confirm the
   selected engine / preset, edits and kit pad routing return.
5. Open GEN. Generate Melody / Chords or kit Drums, drag MIDI to the DAW or export
   and import the MIDI file. Play the clip in the DAW. GEN does not play internally.

## Offline diagnostic
1. After installing to the default folder, close the DAW.
2. Open the extracted ZIP folder → Checks → double-click Run-check.cmd.
3. Wait for the final INSTRUMENT PASS or an error. This may take several minutes.
   The checker renders offline, so no sound through the speakers is expected.
4. Press a key to close its window. Open Checks/check-results.txt.
5. Send that text file, plus your Windows and DAW versions and reproduction steps,
   to jazzyrhythmer@gmail.com or attach it to a GitHub issue.

The checker tests engine load, basic output, state restore, panic and kit labels.
It does not verify the VST3 host, audio device, native window, automation or MIDI drag.
If Windows blocks the diagnostic, do not disable security tools. Send the exact
message and continue with the DAW checklist if the plug-in itself loads.

Review the log before sharing; local usernames and paths may be redacted.
