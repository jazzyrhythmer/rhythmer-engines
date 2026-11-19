# Rhythmer Engines for MPC 3

By Jazzy Rhythmer · private instrument test build 0.1.0 / Test 8 · UID JFq7.

This instrument runs inside MPC OS. It is separate from RHYTHMER OS for MPC Sample.

## Install and first sound

1. Use MPC OS 3.10.1 or later. Live / Live II / One / X use ARM32; both architectures are included.
2. Extract the ZIP. Replace the complete earlier test folder inside `VST2 Plugins` on your drive. Keep the folder name `Jazzy Rhythmer - VST - Rhythmer Engines` unchanged, then restart MPC so it reloads the skin.
3. In **Preferences → Enhancements (Beta)**, enable **Enable Enhancements** and **External Plugin Support**.
4. Create a plug-in program; choose **External Plugins → Jazzy Rhythmer → Rhythmer Engines**.
5. MPC's native **PRESET** chooses one of 20 engines. The internal **ENGINE** card selects the same engine. **SOUND** selects its sound; **KIT** selects a whole Rhythmer Kit.
6. Tap a selector card to open the in-page list. Touch a row to apply it. **PREVIOUS / NEXT** navigate 16 choices per page; **CLOSE** leaves the current selection unchanged. The page counter shows your position and total choices.

The internal selector uses real parameter-bound rows rather than MPC's native external-VST drop-down picker. Original sound names and order are retained: 2,707 synth selections and 36 kits. These are selection entries, not a claim of unique synthesized programs.

## Pages and Q-Links

| Page | Controls |
| --- | --- |
| PLAY | Eight primary sound controls, four on each side of the original 3D artwork. |
| EDIT | Oscillator, model, tone and mixing controls, in named groups. |
| MOD | Envelope, LFO and modulation controls. |
| FX | The source engine's effects controls. |
| KIT | 4×4 pad selection, pad engine and sound replacement; selected pad controls only. Kit filter controls affect the whole kit. |
| ABOUT | Seven pages of creator links, source credits and licenses. Press ABOUT again for the next page; PLAY returns. |

EDIT, MOD and FX have independent **PAGE** selectors and counters. All available source parameters remain reachable, in their original group order. Empty sections are explicitly labelled. Changing an engine's model refreshes its parameter names and values.

On Live / Live II / One, the first Q-Link bank controls the four left knobs top to bottom; the next controls the four right knobs. The third bank contains page and source selectors. The fourth bank controls the choice browser: **PAGE, PREVIOUS, NEXT, CLOSE**. Hidden sound knobs are blocked while the browser is open. Contextual sound controls are not host-automatable.

Yellow Q-Link bank rectangles are hidden; hardware assignments and white individual-control focus remain. Animation is driven by note pitch, velocity and measured output level, and settles at silence instead of looping freely.

## Rhythmer Kit MIDI

P01 is bottom left and P16 top right. **MIDI ROOT** defaults to note 60, so notes 60–75 play P01–P16. Legacy drum notes 36–51 also work. MPC Pad Perform can change the first pad note and scale. MIDI ROOT must match the MIDI note sent by pad 1: for example, choose 24 if pad 1 sends 24. Use chromatic Notes mode for a one-note-per-pad kit. For a consistent starting setup, select C3 (MIDI 60 in MPC notation) in Pad Perform and keep MIDI ROOT at 60. MIDI numbers avoid octave-name differences between controllers. Root is saved with the sound state. Selecting a pad for editing does not change the MIDI mapping.

K-WASUB / 909 is the default kit. All 36 kits retain the existing 909 loudness reference, per-pad levels, saved kit trim and stereo peak guard. Save/reopen a test project to preserve edited sounds and routes. **STOP SOUND** clears held notes; host transport stop clears held notes and sustain, including during an engine replacement.

## Scope and credits

This build excludes GEN, CHORD GEN, Acid Engine and Break Engine. MPC displays creator URLs as text; it does not open a web browser. Required notices remain in CREDITS.md, SOURCE-INDEX.json and Licenses/.

Local ARM32/ARM64 host, audio, state and skin checks do not replace testing touch, Q-Link, CPU, memory and listening on a physical MPC Live 1. Original parameter IDs 0–88 remain in order; new editor and browser controls are appended.
