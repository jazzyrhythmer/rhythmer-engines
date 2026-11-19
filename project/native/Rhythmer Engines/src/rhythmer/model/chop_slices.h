// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

// Chop's slices as plain data (parity phase 5; shell/chop.h cuts them), and a
// sample's chop: its slices, as the MPC keeps them in the sample's WAV file
// (shell/mpc_wav_slices.h), read when the sample loads and written back when
// Chop changes them; the region they span, and the type and amount they were
// cut with while the project is open.
namespace rhythmer {

enum class ChopMode : uint8_t { Threshold, Regions, Beats, Manual, Count };
inline const char* chop_mode_name(ChopMode m) {
    static const char* names[] {"Threshold", "Regions", "Beats", "Manual"};
    return names[unsigned(m) % unsigned(ChopMode::Count)];
}
constexpr unsigned max_slices = 64;
constexpr unsigned manual_slices = 16;   // the MPC's: "up to 16 slices per sample"
constexpr uint32_t min_slice = 882;      // 20 ms: no slice shorter

// Each slice its own start and end, as the MPC 3 keeps a sample's slices (its WAV's "atem" chunk, "Slice 0"...): slices
// cut from one region touch, one ending where the next starts; slices read from an MPC file may leave gaps between
// them. Where two touch, moving the edge they share moves both.
struct Slices {
    std::array<uint32_t, max_slices> start {};
    std::array<uint32_t, max_slices> stop {}; // where each ends (past its last frame); 0: the sample's end
    unsigned count = 0;
    uint32_t end(unsigned n, uint32_t frames) const {
        uint32_t e = n < count ? stop[n] : 0;
        return e && e <= frames ? e : frames;
    }
    bool touches(unsigned n, uint32_t frames) const { return n + 1 < count && end(n, frames) == start[n + 1]; } // slice n and the next
    bool same(const Slices& o) const {
        if (count != o.count) return false;
        for (unsigned n = 0; n < count; ++n) if (start[n] != o.start[n] || stop[n] != o.stop[n]) return false;
        return true;
    }
    // One slice, from `from` to `to` (0: the sample's end).
    void whole(uint32_t from, uint32_t to) { start = {}; stop = {}; start[0] = from; stop[0] = to; count = 1; }
    // A cut inside a slice: it ends there and a new slice runs on to where it ended. False when full, outside every
    // slice, or too close to either edge.
    bool add(uint32_t frame, uint32_t frames, unsigned most = max_slices) {
        if (!count || count >= std::min<unsigned>(most, max_slices)) return false;
        unsigned at = 0;
        while (at < count && !(start[at] < frame && frame < end(at, frames))) ++at;
        if (at == count || start[at] + min_slice > frame || frame + min_slice > end(at, frames)) return false;
        for (unsigned n = count; n > at + 1; --n) { start[n] = start[n - 1]; stop[n] = stop[n - 1]; }
        start[at + 1] = frame; stop[at + 1] = stop[at]; stop[at] = frame; ++count;
        return true;
    }
    // Takes slice n away; what it held (and any gap before it) joins the slice before (the MPC's Merge, and
    // Erase + pad). The first slice has none before it.
    void remove(unsigned n) {
        if (n == 0 || n >= count) return;
        stop[n - 1] = stop[n];
        for (unsigned k = n + 1; k < count; ++k) { start[k - 1] = start[k]; stop[k - 1] = stop[k]; }
        --count;
        start[count] = stop[count] = 0;
    }
    // Slice n cut in half (Split); false when it is too short or the slices are full.
    bool split(unsigned n, uint32_t frames) {
        if (n >= count) return false;
        uint32_t a = start[n], b = end(n, frames);
        return b - a >= 2 * min_slice && add(a + (b - a) / 2, frames);
    }
    // K1 and K2: slice n's start or end; a neighbour touching it there moves with it.
    void move_start(unsigned n, uint32_t frame, uint32_t frames) {
        if (n >= count) return;
        const bool joined = n && touches(n - 1, frames);
        uint32_t lo = !n ? 0 : joined ? start[n - 1] + min_slice : end(n - 1, frames), hi = end(n, frames);
        hi = hi > min_slice ? hi - min_slice : 0;
        start[n] = std::clamp(frame, lo, std::max(lo, hi));
        if (joined) stop[n - 1] = start[n];
    }
    void move_end(unsigned n, uint32_t frame, uint32_t frames) {
        if (n >= count) return;
        uint32_t lo = start[n] + min_slice;
        if (touches(n, frames)) {
            uint32_t hi = end(n + 1, frames);
            hi = hi > min_slice ? hi - min_slice : 0;
            stop[n] = start[n + 1] = std::clamp(frame, lo, std::max(lo, hi));
        } else {
            uint32_t hi = n + 1 < count ? start[n + 1] : frames;
            stop[n] = std::clamp(frame, std::min(lo, hi), hi);
        }
    }
};

// A sample's chop (roadmap P3): the region of it Chop cut (to 0: the sample's end), how, and the slices.
struct SampleChop {
    uint32_t from = 0, to = 0;
    ChopMode mode = ChopMode::Threshold;
    int16_t amount = 0;
    Slices slices {};
    bool unwritten = false; // changed here (or read from an older project file) and not yet in its WAV file
    bool used() const { return slices.count > 0; }
};

} // namespace rhythmer
