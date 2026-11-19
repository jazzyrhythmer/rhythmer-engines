# Sophie for Schwung

Sophie is a sixteen-pad metallic FM percussion synthesizer for Ableton Move, built for the
[Schwung](https://github.com/charlesvestal/schwung) framework. It makes kicks,
clangs, zaps, bells, shards and unstable digital impacts from synthesis alone.

## Controls

- **Tune** shifts each pad independently across a four-octave range without
  changing its drum-rack MIDI note.
- **Model** selects six operator structures: Fuse, Stack, Split, Shard,
  **Pipe** (FM plus ring modulation) and **Boom** (sine body with an FM attack).
- **Color** moves the oscillators through inharmonic frequency ratios.
- **Metal** controls FM depth and spectral density.
- **Feedback** feeds each voice back into its first operator.
- **Sweep** applies a fast bipolar pitch envelope.
- **Decay** is the approximate audible duration, from tiny clicks to
  four-second ringing hits.
- **Crush** combines bit-depth reduction with sample-rate reduction.
- **Drive** pushes the output through a bounded nonlinear stage.
- **Cutoff**, **Resonance** and **Filter Type** shape each pad with LPF, HPF,
  BPF, notch and DJ-style modes. The filter is the final per-pad stage, after
  Crush, Drive and Ring, so it can tame every harmonic those effects create.
- **Ring Time**, **Ring Feedback**, **Ring Mix** and **Ring Tone** are a
  per-pad short-delay/comb effect for metallic echoes and resonant rings.

## Character page (v0.6)

- **Metal Decay** sets the brightness contour independently of pitch, in ms.
  Zero preserves the original contour on Fuse/Stack/Split/Shard and uses a
  135 ms contour on Pipe/Boom. Try 100–400 ms with a longer audible Decay.
  On Shard it also fades the noise bloom; on the original kick it fades the
  parallel metallic texture.
- **Feedback Mode: Raw / Smooth**. Raw preserves the original response.
  Smooth gives finer low-feedback control, damps the feedback loop, limits
  FM depth at high pitches, and uses continuous detuned oscillators in
  Stack/Split and the kick variants. It is a different timbre, not alias-free DSP.
- **Tail Mode: Hit / Live**. Hit (default) captures each hit's settings, ideal
  for independent step p-locks. Live makes Color, Metal, Feedback, Cutoff and
  Resonance follow the pad with 5 ms smoothing on all its ringing voices.
  Host LFOs, recorded automation and p-locks affect those Live tails too:
  the module API does not distinguish their writes from knob edits.
  Select Live before triggering a note. Other controls remain per-hit.

Pipe and Boom work on every pad, including pad 1. Existing model numbers,
factory sounds and preset defaults are retained. A short transition softens
voice stealing when all twelve voices are busy.

To audition: choose Pipe, set Decay around 1 s, Metal around 50%, then compare
Metal Decay at 40 and 400 ms. Try Smooth with high Feedback. Choose Boom with
low Tune and positive Sweep for a round body and a metallic attack. Select
Live, trigger a long hit, and move Cutoff.

## Movy

The package includes a module-local `movy_config.json`. Movy detects it
automatically and exposes Sophie as a 16-pad drum module with per-pad Main,
Filter, Ring and Character pages, following the Move drum-pad note layout.

MIDI notes 36–51 follow Ableton Drum Rack and General MIDI ordering, with a
different factory sound stored for every pad. Hitting a pad selects that sound
in Schwung's knob grid, where all controls edit only the selected pad.
Velocity controls impact level and twelve voices allow overlapping percussion.

The kick uses a low tuned sine body with a downward pitch envelope. Its folded
FM layer is mixed quietly for texture, following the broad two-layer technique
used in many Ponyboy-style recreations without copying any proprietary engine.

The Ring page is bypassed at its default Mix of 0%. Short times around 2–10 ms
with moderate feedback produce metallic comb resonance; longer times and
brighter Tone settings create more obvious echoes.

Sophie is an original instrument assembled from established synthesis
techniques. It is not a Monomachine emulation, contains no Elektron code or
assets, and is not affiliated with or endorsed by Elektron or the estate of
SOPHIE.

## Build and test

```sh
./scripts/test.sh
./scripts/build.sh
```

The ARM64 package is written to `dist/sophie-module.tar.gz`.

To install a development build on a reachable Move:

```sh
./scripts/install.sh
# Or, if mDNS is unavailable:
MOVE_HOST=ableton@10.0.0.x ./scripts/install.sh
```

## Credits

Pipe, Boom, independent brightness decay and feedback-conditioning ideas are
adapted from [soejrd/digisophie](https://github.com/soejrd/digisophie), MIT,
source revision `ee4ff40`. This is a floating-point adaptation for Move, using
Sophie's own voice and effects chain.

## License

[MIT](LICENSE)
