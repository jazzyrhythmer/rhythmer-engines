// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "rhythmer/engine/multisample.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

// A sample, and what the screen needs about it. Read and analysed on the
// service thread (it allocates and reads files); the audio thread only reads
// `left`, `right` and `frames` (or `stream`'s) through a pointer handed over by
// the engine host. A sample is held once, as its file or capture holds it: its
// frames at its own rate, never converted, with no second copy at the
// device's rate or at 44.1 kHz. A long WAV is not read into memory at all: it
// is read from the card as it plays (`stream`). The project's positions count 44.1 kHz frames
// (a region, a slice, the overview's columns, the hits): model() and own()
// turn a position from one to the other, and a voice reads the frames that
// much further each device frame (engine/sampler.cpp).
namespace rhythmer {

// A sample still being read (the browser's preview, SampleFill): its frames are laid out at their full length, silent,
// before the audio thread has it, and filled in from the start while it plays. How far it is filled in, published as
// it lands; the audio thread reads below that only. `cut`: no more will come (the file ran out early, or it is no
// longer wanted), so a voice plays to there and ends; `whole`: everything is there.
struct SampleGrowth {
    std::atomic<uint32_t> frames {0}; // of `left` and `right`
    std::atomic<bool> cut {false}, whole {false};
};

struct BreakPcm { std::vector<float> mono; std::atomic<bool> ready {false}; };

struct SampleData {
    std::vector<float> left, right; // at `rate`
    uint32_t frames = 0;            // of `left` and `right`, or of `stream`
    unsigned rate = 44100;          // Hz: the file's own, or the capture's
    std::shared_ptr<Multisample> multi; // a multisampled instrument: no frames of its own, its zones' instead
    std::shared_ptr<SampleGrowth> growth; // still being read (SampleFill); none: all of it is there
    // A long WAV read in place from the card (engine/stream_pager.h), `left` and `right` empty: a voice reads the
    // part's frames, which the pager fills a chunk at a time ahead of it. The chunks the pads start on (and end and
    // loop on) are held (`held`, the service thread's: shell/sample_library.cpp), so a hit sounds at once.
    // Random-access Break performance needs a bounded, immutable mono loop.
    // Filled off audio; published once and owned until sample retirement.
    std::shared_ptr<BreakPcm> break_cache = std::make_shared<BreakPcm>();
    std::shared_ptr<ZonePart> stream;
    mutable std::vector<uint32_t> held; // the stream's chunks held, in order
    // Frame `own` of its own as the project counts it (44.1 kHz), and the other way, within its frames.
    uint32_t model(uint32_t own) const { return rate == 44100 ? own : uint32_t(std::llround(double(own) * 44100.0 / rate)); }
    uint32_t own(uint32_t model) const {
        if (rate == 44100) return std::min(model, frames);
        return uint32_t(std::min<double>(frames, double(std::llround(double(model) * rate / 44100.0))));
    }
    uint32_t model_frames() const { return model(frames); } // its length as the project counts it
    size_t bytes() const { return (left.capacity() + right.capacity() + (break_cache->ready.load(std::memory_order_acquire) ? break_cache->mono.capacity() : 0)) * sizeof(float); }
};

// Band-limited rate conversion (the base's dsp::SincResampler, as plugin/polyphase.h works it out for a ratio that
// never changes: flat to 0.907 of the lower Nyquist, images 90 dB down): `frames` of left and right at `from` Hz to
// `to` Hz, frame n of the result at n * from / to of the input (no delay), as many as the ratio gives. Allocates: not
// for the audio thread. For what needs another rate for a while (a program that takes 44.1 kHz), never to keep.
void resample(const float* left, const float* right, uint32_t frames, unsigned from, unsigned to,
              std::vector<float>& out_left, std::vector<float>& out_right);
// Frames read or recorded at `rate` Hz as `sample`, kept as they are. Takes the vectors (by value: the sample's own
// may be moved in).
void adopt(SampleData& sample, std::vector<float> left, std::vector<float> right, unsigned rate);

// What a file says of itself, beyond its frames: its rate and sides, and the
// first loop of its smpl chunk (frames at its own rate, end past the last; -1 none).
struct WavInfo {
    unsigned channels = 0, rate = 0;
    int64_t loop_start = -1, loop_end = -1;
};

// The waveform as the pages draw it: the peak of each of 512 columns, 0..255.
using Overview = std::array<uint8_t, 512>;
// Where the hits are, for chopping: a frame and how strong the rise was, 0..255.
struct Hit { uint32_t frame = 0; uint8_t strength = 0; };
// Enough for a minute of busy playing (about 8 a second), so a long take keeps the hits early in it that Beats
// lands its cuts on, not only its strongest 128.
constexpr unsigned max_hits = 512;

// Reads a WAV file (PCM 16, 24 or 32 bit, or 32-bit float; mono or stereo;
// any rate, kept at it) of at most `max_seconds`. Returns an empty message
// on success, else what went wrong.
std::string read_wav(const std::string& path, SampleData& out, unsigned max_seconds = 60, WavInfo* info = nullptr);

// How a PCM WAV lies in its file: where its frames start, how many there are and how each is kept. Enough to read
// any stretch of it in place (a multisampled instrument's file streamed from the card as it is, engine/stream_pager.h).
struct WavLayout {
    uint64_t data_at = 0;   // bytes from the file's start
    uint32_t frames = 0;
    unsigned channels = 0, rate = 0, bits = 0;
    bool floating = false;  // 32-bit float; else integer PCM of `bits`
    unsigned block() const { return bits / 8 * channels; } // bytes a frame
};
// `frames` frames as they lie in such a file (`bytes`) as floats, as read_wav() makes them: left and right apart, a
// mono file's on `left` alone when `right` is null (else on both).
void wav_frames(const WavLayout& layout, const uint8_t* bytes, uint32_t frames, float* left, float* right);

// A WAV file's frames a piece at a time, as read_wav() reads them whole (it is read_wav()'s reader): open() reads
// the head (and, with `info`, the smpl chunk wherever it lies) and finds the frames, at most `max_seconds` of them;
// read() hands on the next ones as floats at the file's own rate (a mono file's on both sides), fewer at the end of
// the file. open() is empty on success, else what went wrong (read_wav()'s words).
class WavReader {
public:
    std::string open(const std::string& path, unsigned max_seconds = 60, WavInfo* info = nullptr);
    unsigned rate() const { return hz; }
    uint32_t frames() const { return count; }
    uint32_t read(float* left, float* right, uint32_t frames);
    bool seek(uint32_t frame); // the next read() from this frame on
    WavLayout layout() const { return {data_start, count, channels, hz, bits, format == 3}; } // after open()
private:
    std::ifstream file;
    unsigned format = 0, channels = 0, hz = 0, bits = 0;
    uint32_t count = 0, done = 0;
    uint64_t data_start = 0;
    std::vector<uint8_t> bytes;
};

// A sample laid out at its full length before its frames are there, then filled in a piece at a time from the start
// (the browser's preview: it plays while the rest is read). Its frames are the file's own, as read_wav() keeps them;
// how far they are filled in is published through the sample's growth as they land (SampleGrowth). Service thread:
// allocates.
class SampleFill {
public:
    SampleFill();
    ~SampleFill();
    SampleFill(const SampleFill&) = delete;
    SampleFill& operator=(const SampleFill&) = delete;
    // `frames` to come at `rate` Hz: `sample` laid out for them, silent, nothing landed.
    void begin(SampleData& sample, uint32_t frames, unsigned rate);
    // The next frames from the file; any past the `frames` begun with are left out.
    void add(const float* left, const float* right, uint32_t frames);
    // The end: the sample is whole, or, short of the frames begun with (the file ran out early, or it is no longer
    // wanted), cut there.
    void finish();
    bool filling() const { return sample != nullptr; }
    uint32_t to_come() const { return left_to_add; }
private:
    SampleData* sample = nullptr;
    std::shared_ptr<SampleGrowth> growth;
    uint32_t written = 0, left_to_add = 0;
};
// The loop a FLAC file keeps from the WAV it was made of (flac --keep-foreign-metadata: an
// APPLICATION block "riff" holding the smpl chunk), into `info`; false when it has none.
bool read_flac_loop(const std::string& path, WavInfo& info);
// An MP3's first frame: the frames each of its frames holds (1152 for MPEG 1, 576 for MPEG 2 and 2.5) and, when it
// carries a LAME tag (LAME, and FFmpeg's "Lavc"/"Lavf", in the Xing or Info frame), the encoder delay and padding
// it records. False when the file does not start with a Layer III frame.
struct Mp3Head { unsigned frame_samples = 0, delay = 0, padding = 0; bool gapless = false; };
bool read_mp3_head(const std::string& path, Mp3Head& head);
// Writes the sample's `left` and `right` as stereo at its rate: 24-bit PCM, or 32-bit float when the sound goes past
// full scale (so the file never clips what plays); empty on success.
std::string write_wav(const std::string& path, const SampleData& sample);
// The sample's `left` and `right` as write_wav keeps them: on 24-bit steps (read back by read_wav to the same floats),
// unchanged when it writes 32-bit float. What plays is then what the file holds, to the bit.
void as_written(SampleData& sample);
// A capture (a take, Audio Recall, a render), its `left` and `right` at `rate` Hz, as a sample at that rate: kept as a
// file holds it (as_written) and written to `path` (not when empty). write_wav's error, if any.
std::string keep_capture(SampleData& sample, unsigned rate, const std::string& path);
// Stereo 32-bit float at `rate` Hz written as it comes (the song's mixdown,
// kept whole however loud): open, add interleaved stereo frames, then close,
// which puts the lengths in the header. Roadmap P4: or integer PCM, 24 or 16
// bits, TPDF dithered (only there: a float file needs none) and clipped.
class WavStream {
public:
    ~WavStream() { close(); }
    // `data_at` (freeze plan F2b's prints): the frames start that many bytes in (a JUNK chunk before them), a page's
    // boundary, so a mapping of the file pins and lets go of whole pages of them; 0: right after the header.
    // `channels` interleaved (2: stereo; a kit's print with pad sends has a pair more for each bus it sends into).
    static constexpr unsigned max_wav_channels = 10;
    bool open(const std::string& path, unsigned rate = 44100, unsigned bits = 32, unsigned data_at = 0, unsigned channels = 2);
    bool add(const float* interleaved, unsigned frames);
    bool close(); // false when anything failed
    // Integer PCM is dithered; off, each value is rounded as it is (a copy of frames that came from a file of as many
    // bits or fewer stays the same to the bit).
    void set_dither(bool on) { dither = on; }
    uint32_t frames() const { return written; }
private:
    std::FILE* file = nullptr;
    uint32_t written = 0;
    unsigned rate = 44100, bits = 32, data_at = 0, channels = 2;
    bool failed = false, dither = true;
    std::vector<unsigned char> header(uint32_t frames) const;
    uint32_t noise = 0x9e3779b9u; // the dither's
};
// `out` the louder side of each column; with `sides`, the left and right apart. True when the sides differ (a
// stereo file; a mono one is read onto both sides alike).
bool make_overview(const SampleData& sample, Overview& out, std::array<Overview, 2>* sides = nullptr);
// The hits (engine/onsets.cpp: changes in the spectrum, each cut moved back to
// the quiet point before its attack), strongest kept, sorted by frame; the
// first is always frame 0. Strength is relative to the strongest hit.
unsigned find_hits(const SampleData& sample, std::array<Hit, max_hits>& out);

} // namespace rhythmer
