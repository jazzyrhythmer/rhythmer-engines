// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "rhythmer/model/project.h"
#include <algorithm>
#include <cstring>

// KEYGROUP tracks (model/project.h EngineKind::Keygroup) out of kits' instrument pads. Before keygroup tracks an
// instrument (an .sfz, an MPC keygroup or drum pad, an .exs, a DecentSampler or .sf2 preset) sat on a kit pad and
// played its key 60 + the pad's pitch + each note's semitones (engine/sampler.cpp zones_on()). A pad of an MPC drum
// program or project ("<file>#<pad>": velocity layers of one drum) stays a kit pad. Each other such pad becomes
// a keygroup track: its sound on pad 1 (its pitch its tune), its notes MIDI notes 60 + their semitones, which the
// keyed sampler plays at the same keys. A kit with nothing else (no other pad, no note of another) turns into the
// keygroup track itself and keeps everything; otherwise the pad and its notes go to the first empty track, named
// after its sound (the kit's name when it has none), with the kit's mix, effects, sound parameters, pattern, chain
// and song clips, and the kit keeps the rest. With no
// empty track the pad stays on its kit (counted as left).
namespace rhythmer {

struct KeygroupMove { unsigned moved = 0, left = 0; };

namespace keygroup_detail {
// An MPC drum program's or project's pad, "<file>.xpm#<pad>" (.xty, .xpj too): a drum's layers, not keys.
inline bool drum_program_pad(const char* path) {
    const char* mark = std::strrchr(path, '#');
    if (!mark || mark - path < 4) return false;
    const char kind[3] = {char(mark[-3] | 32), char(mark[-2] | 32), char(mark[-1] | 32)};
    return mark[-4] == '.' && kind[0] == 'x' && ((kind[1] == 'p' && (kind[2] == 'm' || kind[2] == 'j')) || (kind[1] == 't' && kind[2] == 'y'));
}
inline bool instrument_pad(const Project& p, const PadSound& pad) {
    if (!pad.used()) return false;
    const auto& ref = p.samples[pad.sample % sample_slots];
    return ref.instrument() && !drum_program_pad(ref.path);
}
// A kit pattern's notes of `pad` as a keygroup's: MIDI notes, ordered as a pattern keeps them.
inline Pattern keyed_pattern(const Pattern& kit, unsigned pad) {
    Pattern out = kit;
    out.events.clear();
    out.lanes.clear();
    out.euclids.clear();
    out.punches.clear();
    for (auto e : kit.events) {
        if (e.key != sampler_first_note + pad) continue;
        e.key = uint8_t(std::clamp(60 + int(e.pitch), 0, 127));
        e.pitch = 0;
        e.pitch_ramp = 0;
        e.cutoff = 0;
        out.events.push_back(e);
    }
    std::stable_sort(out.events.begin(), out.events.end(), [](const Event& a, const Event& b) { return a.tick != b.tick ? a.tick < b.tick : a.key < b.key; });
    return out;
}
inline void drop_pad_notes(Pattern& kit, unsigned pad) {
    const unsigned key = sampler_first_note + pad;
    kit.events.erase(std::remove_if(kit.events.begin(), kit.events.end(), [&](const Event& e) { return e.key == key; }), kit.events.end());
    kit.lanes.erase(std::remove_if(kit.lanes.begin(), kit.lanes.end(), [&](const LaneLength& l) { return l.key == key; }), kit.lanes.end());
    kit.euclids.erase(std::remove_if(kit.euclids.begin(), kit.euclids.end(), [&](const EuclidLane& l) { return l.key == key; }), kit.euclids.end());
}
// Whether the kit plays nothing but `pad`: no other pad holds a sound, no note is another's.
inline bool only_pad(const Track& kit, const std::array<Pattern, pattern_count>& patterns, unsigned pad) {
    for (unsigned n = 0; n < sampler_pads; ++n) if (n != pad && kit.pads[n].used()) return false;
    for (const auto& pattern : patterns)
        for (const auto& e : pattern.events) if (e.key != sampler_first_note + pad) return false;
    return true;
}
} // namespace keygroup_detail

inline KeygroupMove migrate_instrument_pads(Project& project, PatternBank& bank) {
    using namespace keygroup_detail;
    KeygroupMove out;
    for (unsigned t = 0; t < track_count; ++t) {
        if (project.tracks[t].engine != EngineKind::Sampler) continue;
        for (unsigned n = 0; n < sampler_pads; ++n) {
            Track& kit = project.tracks[t];
            if (kit.engine != EngineKind::Sampler || !instrument_pad(project, kit.pads[n])) continue;
            const bool alone = only_pad(kit, bank.at[t], n);
            int target = alone ? int(t) : -1;
            for (unsigned k = 0; k < track_count && target < 0; ++k) if (!project.tracks[k].used()) target = int(k);
            if (target < 0) { ++out.left; continue; }

            Track made = alone ? kit : Track {};
            if (!alone) { // what the kit's sound went through, and how it played; named after the sound
                const char* sound = project.samples[kit.pads[n].sample % sample_slots].name;
                made.set_name(sound[0] ? sound : kit.name);
                made.params = kit.params;
                made.mix = kit.mix;
                made.fx = kit.fx;
                made.mods = kit.mods;
                made.pattern = kit.pattern;
                made.chain = kit.chain; made.chain_count = kit.chain_count; made.chain_at = kit.chain_at;
                made.speed = kit.speed;
                made.midi_channel = 0; // the kit keeps its MIDI channel
            }
            made.engine = EngineKind::Keygroup;
            made.pads = {};
            made.pads[0] = kit.pads[n];
            made.pad_fx = {};
            made.pad_fx.at[0] = kit.pad_fx.at[n];
            made.zone_trims = {};
            unsigned kept = 0;
            for (const auto& z : kit.zone_trims)
                if (z.used() && z.pad == n && kept < made.zone_trims.size()) { auto moved = z; moved.pad = 0; made.zone_trims[kept++] = moved; }
            made.pad_mutes = {};
            made.set_pad_muted(0, kit.pad_muted(n));
            made.last_pad = 0; made.bank = 0; made.levels = false; made.levels_pad = 0;
            made.layout = PadLayout::Scale;
            made.last_note = 60;

            std::array<Pattern, pattern_count> notes;
            for (unsigned p = 0; p < pattern_count; ++p) notes[p] = keyed_pattern(bank.at[t][p], n);
            if (!alone) {
                for (auto& pattern : bank.at[t]) drop_pad_notes(pattern, n);
                kit.pads[n] = PadSound {};
                kit.pad_fx.at[n] = PadChains {}.at[0]; // an empty pad chain, its slots counted
                kit.set_pad_muted(n, false);
                for (auto& z : kit.zone_trims) if (z.used() && z.pad == n) z = ZoneTrim {};
                // its clips, the same on its own lane
                const unsigned count = project.song.count;
                for (unsigned c = 0; c < count; ++c) {
                    Clip clip = project.song.clips[c];
                    if (clip.track != t) continue;
                    clip.track = uint8_t(target);
                    project.song.place(clip);
                }
            }
            project.tracks[size_t(target)] = made;
            bank.at[size_t(target)] = std::move(notes);
            ++out.moved;
        }
    }
    return out;
}

} // namespace rhythmer
