// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "rhythmer/model/input_config.h"
#include "rhythmer/model/limits.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Freeze plan F3, audio tracks: an AUDIO track, as on the MPC 3, records its
// input into its pattern and plays it back wherever that pattern plays (the
// pads, Patterns, the Song's clips). Seq Record + Play records one pass of the
// pattern from its top; recording again replaces the take, and Undo brings the
// old one back. Its pads play nothing.
//
// A pattern's take is a part of a WAV file in "<project> Audio/" beside the
// project (Pattern::audio, model/pattern.h). Each take gets a file of its own,
// never written again, so Undo and copies can always play an old one. Frames
// are counted at 44.1 kHz, as the project counts everything; a take's file is
// at the device's rate as it was recorded.
namespace rhythmer {

// What the track hears of its input: On while it is armed or recording (as the MPC's Auto), Off never.
enum class TrackMonitor : uint8_t { Off, On, Count };
inline const char* track_monitor_name(TrackMonitor m) { return m == TrackMonitor::Off ? "Off" : "On"; }

// The sources an audio track records from, in the order K1 turns through them: the inputs and USB-C (Rhythmer's own
// mix and tracks are Resample's and the Track source's, on Sample Record).
constexpr InputSource audio_sources[] {InputSource::Rear, InputSource::RearL, InputSource::RearR, InputSource::Mic,
                                       InputSource::Usb, InputSource::UsbL, InputSource::UsbR};
constexpr unsigned audio_source_count = sizeof(audio_sources) / sizeof(audio_sources[0]);
inline unsigned audio_source_index(InputSource s) {
    for (unsigned n = 0; n < audio_source_count; ++n) if (audio_sources[n] == s) return n;
    return 0;
}

// A source as the audio track's cards and chips name it.
inline const char* audio_source_label(InputSource s) {
    switch (s) {
    case InputSource::Rear: return "Rear L+R";
    case InputSource::Usb: return "USB L+R";
    default: return input_source_name(s);
    }
}

// An audio track's own settings (AUDT): where it records from and whether its input is heard.
struct AudioTrackSettings {
    InputSource source = InputSource::Rear;
    TrackMonitor monitor = TrackMonitor::On;
    bool operator==(const AudioTrackSettings& o) const { return source == o.source && monitor == o.monitor; }
};

// Warp's algorithms (kept with the take for when warp plays): Stretch keeps the pitch, Repitch plays it faster or slower.
enum class WarpMode : uint8_t { Stretch, Repitch, Count };

// A pattern's take: frames [offset, offset + frames) of take `take`, from the pattern's top, at its level, faded in
// and out, forwards or backwards. `tempo` is the tempo it was recorded at.
struct AudioRegion {
    uint16_t take = 0;        // its file: audio_take_name(take)
    uint32_t offset = 0;      // Edit Start: frames of the take skipped
    uint32_t frames = 0;      // what it plays from there (Edit End - Edit Start)
    uint32_t take_frames = 0; // the whole take
    uint16_t tempo = 1200;    // tenths of a BPM
    int16_t gain = 0;         // tenths of a dB, region_gain_min..region_gain_max
    uint32_t fade_in = 0, fade_out = 0; // frames
    bool reverse = false;
    bool warp = false;
    WarpMode mode = WarpMode::Stretch;
    int8_t semi = 0, fine = 0;
    bool used() const { return take != 0 && frames != 0; }
    bool operator==(const AudioRegion& o) const {
        return take == o.take && offset == o.offset && frames == o.frames && take_frames == o.take_frames && tempo == o.tempo &&
               gain == o.gain && fade_in == o.fade_in && fade_out == o.fade_out && reverse == o.reverse && warp == o.warp &&
               mode == o.mode && semi == o.semi && fine == o.fine;
    }
    bool operator!=(const AudioRegion& o) const { return !(*this == o); }
};
constexpr int region_gain_min = -600, region_gain_max = 120; // -60 .. +12 dB

// Frames (44.1 kHz) and ticks at a tempo (tenths of a BPM): a quarter is 600 / tempo seconds.
inline uint64_t frames_to_ticks(uint64_t frames, unsigned tempo) {
    return (frames * std::max(tempo, 1u) * ticks_per_quarter + 300 * 44100) / (600 * 44100);
}
inline uint64_t ticks_to_frames(uint64_t ticks, unsigned tempo) {
    return (ticks * 600 * 44100 + std::max(tempo, 1u) * ticks_per_quarter / 2) / (std::max(tempo, 1u) * ticks_per_quarter);
}

// The folder beside a project file that holds its takes ("" without a file), and a take's file name in it.
inline std::string audio_folder(const std::string& project_path) {
    if (project_path.empty()) return {};
    auto slash = project_path.find_last_of('/');
    auto dot = project_path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) dot = project_path.size();
    return project_path.substr(0, dot) + " Audio";
}
// Take numbers run through the project, whatever track recorded them, so a pattern copied to another track plays
// the same file.
inline std::string audio_take_name(unsigned take) {
    char name[24];
    std::snprintf(name, sizeof(name), "Take %03u.wav", take);
    return name;
}

// A take's overview for the screen: its loudest frame in each hundredth of a second (0..255, a column every
// take_peak_frames frames at 44.1 kHz), kept beside the take as "Take NNN.peaks". Every take's, by take number.
constexpr uint32_t take_peak_frames = 441;
using TakePeaks = std::map<unsigned, std::vector<uint8_t>>;
inline std::string audio_peaks_name(unsigned take) {
    char name[24];
    std::snprintf(name, sizeof(name), "Take %03u.peaks", take);
    return name;
}
// The loudest column of `peaks` over frames [from, to) (44.1 kHz), 0..1.
inline float peak_over(const std::vector<uint8_t>& peaks, uint32_t from, uint32_t to) {
    uint32_t a = from / take_peak_frames, b = std::max(a + 1, (to + take_peak_frames - 1) / take_peak_frames);
    uint8_t most = 0;
    for (uint32_t n = a; n < b && n < peaks.size(); ++n) most = std::max(most, peaks[n]);
    return most / 255.0f;
}

// The take a file name holds ("Take 012.wav": 12), or 0.
inline unsigned audio_take_of(const std::string& name) {
    unsigned take = 0;
    char tail[8] {};
    if (std::sscanf(name.c_str(), "Take %u%7s", &take, tail) == 2 && std::string(tail) == ".wav") return take;
    return 0;
}

} // namespace rhythmer
