# Rhythmer Engines — Mac & Windows

By Jazzy Rhythmer. Mac: Apple Silicon, macOS 13 or later, AU / VST2 / VST3. Windows: Windows 10/11 x64, VST3.

Extract the ZIP and copy the complete bundle into the matching user plug-in folder:
`~/Library/Audio/Plug-Ins/Components`, `VST` or `VST3` on Mac. On Windows, copy the entire `.vst3` folder to `C:\Program Files\Common Files\VST3`. Restart/rescan the DAW.

## First sound

Start with Rhythmer Kit / K-WASUB / 909. Play notes 36–51 for its sixteen pads.
Use ENGINE and PRESET to choose an individual synth and play chromatically.
MIDI comes from the DAW, pads or the on-screen keyboard. GEN creates MIDI clips for your DAW. There is no separate internal sequencer,
dedicated Acid Engine or Break Engine.

## Screens

- PLAY: original live animation; eight direct sound controls, four on either side.
- EDIT: all available sound-design controls, organized as SOUND, MOD and FX.
  Up to sixteen controls fill each EDIT page. Related groups stay together; large
  groups follow the original source subpanels. ALL combines small whole groups;
  use SOUND, MOD or FX to show just that section.
  Use the page arrows and current/total indicator to reach each parameter group.
- KIT (Rhythmer Kit only): 4×4 pad grid, PAD ENGINE and PAD SOUND. Click a pad
  to select and audition it; release stops the held note. The eight knobs and
  EDIT target the selected pad; kit cutoff/resonance act on the complete kit.
  MIDI playback does not move edit focus. Pad 1 is at bottom left, pad 16 top right.
- GEN: compose a MIDI pattern, review notes, drag it to a DAW or export a `.mid` file.
- ABOUT: creator, clickable website/social/support links and original credits.

STOP SOUND releases the on-screen keyboard and silences the instrument.
DAW transport stop also clears held notes. Normal note-offs preserve sound releases.
Save the DAW project to preserve the engine, sound edits, pad routes and kit mix.
The contextual sound controls are manually editable, not host-automatable.

The 69 factory kits use K-WASUB / 909 as their mix reference. Original source
presets are retained. The original 36 kits retain their pad calibration; the 33
additional kits retain their original voice balance with a kit output trim.
KrautDrums has shorter transients and remains slightly quieter in the test groove. A stereo peak guard acts only when the combined kit exceeds its ceiling.
Individual synth programs are not normalized by this kit calibration.

Built on retained Melodee components by Kerem Kilic / Ellic Studio and original
open-source sound engines. Full notices and source identities are in the bundle.

Transport stop clears held notes and sustain, including a simultaneous engine replacement.

## GEN / MIDI pattern

1. Select a synth and sound, or a Rhythmer Kit preset. Open GEN.
2. Choose MELODY or CHORDS for a synth; a kit uses DRUMS. Drum notes follow the
   selected kit's kick/snare/hat pad roles, rather than General MIDI mapping.
   Kit GEN only lists genres containing drum patterns.
3. Choose GENRE, BARS, KEY and SCALE. RHYTHM changes note placement/complexity;
   ENERGY changes activity. For CHORDS, also set CHORD COUNT, HARMONY, PLAY STYLE
   (Block / Arp / Strum), NOTE LENGTH and OCTAVE.
4. Press GENERATE for a new pattern, MUTATE for a variation, or UNDO to toggle
   back to the previous generated result. Click a note to adjust its velocity.
   The preview is a note display, not an audio playback button.
5. Press and drag DRAG MIDI TO DAW onto a MIDI track/clip lane. If the DAW does
   not accept a file drag from the plug-in, use EXPORT MIDI and import that file.
6. Play the imported clip in the DAW through Rhythmer Engines. Use the DAW piano
   roll for moving/pitching notes, changing lengths, swing and further editing.

MIDI files preserve velocities and note lengths and end on the chosen bar
boundary. They use 4/4 and contain no BPM change. Tempo remains the DAW's tempo.
A synth's envelope/release can continue after a note-off; STOP SOUND or DAW stop
clears held sound. GEN does not add a second playback layer to incoming MIDI.
Your pattern/settings are saved with the plug-in's DAW session state.

This version has no realtime MIDI output port. AU/VST2/VST3 clip import and file drag
support depend on the host. A file export is always available separately.

## Original preset envelopes

Some Aphex drone programs have an original Hold of 8–20 seconds. Hold a note
long enough to hear these sounds. Their factory envelopes have not been shortened.
