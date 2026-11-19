# S1Vave

Pinned original synthesis sources are compiled natively. No reduced JAZZY DSP, device firmware image, hardware emulator, updater or platform UI is used. The common loader isolates the original C globals per instrument instance outside the audio callback. The adapter supplies MIDI, control metadata, complete control state and bounded block buffering. Rhythmer supplies project storage and sequencing.

The original value-formatting functions from `firmware/src/s1vave.c` are retained in `ports/common/s1vave_display.h` under GPL-3.0-only; exact revision and fragment hashes are recorded in SOURCE.json. Only the source UI patch lookup is bound to the single hosted patch. Numeric parameter values and DSP are unchanged.
