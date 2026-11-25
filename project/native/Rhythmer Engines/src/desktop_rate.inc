// Mac DAWs may run above the fixed 44.1 kHz source-engine rate.
#if defined(__APPLE__) || defined(_WIN32)
 rhythmer::PluginRateBridge desktopRate_;
 std::array<mpc::MidiEvent,2048>desktopMidi_{};int desktopMidiCount_=0;
#endif
