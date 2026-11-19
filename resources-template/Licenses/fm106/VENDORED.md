# FM-106

Pinned original synthesis sources are compiled natively. No reduced JAZZY DSP, device firmware image, hardware emulator, updater or platform UI is used. The common loader isolates the original C globals per instrument instance outside the audio callback. The adapter supplies MIDI, control metadata, complete control state and bounded block buffering. Rhythmer supplies project storage and sequencing.
