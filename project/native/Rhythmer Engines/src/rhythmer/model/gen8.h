// SPDX-License-Identifier: GPL-3.0-only
// Musical adaptation of JIANT (Juan Ignacio Antonucci) and Rhythmer (jazzy rhythmer).
#pragma once
#include "rhythmer/model/pattern.h"
#include <array>
#include <cstdint>

namespace rhythmer {
struct Gen8Settings {
    uint16_t seed = 1;
    uint8_t genre = 0, amount = 70, complexity = 50;
    uint8_t first=0,last=3, assign=1, bars=2; // inclusive track range, instruments or MIDI only
    uint8_t length=0; // 0 AUTO; otherwise requested bars on generated targets
    uint8_t group=0; // song target: T1-4 / T5-8 / T9-12 / T13-16
};
inline unsigned gen_song_first(const Gen8Settings& settings){return 4u*(settings.group<4?settings.group:3u);}
enum class Gen8Role : uint8_t { Bass, Chords, Melody1, Melody2, Kick, Snare, Hat, Percussion };
struct Gen8Voice {
    bool drums = false, mono = false, acid = false;
    uint8_t low = 24, high = 96;
    // Kick, snare, clap, closed hat, open hat, tom, rim, percussion.
    std::array<uint8_t, 8> drum_keys {{36,38,39,42,46,45,37,49}};
};
const char* gen8_role_name(Gen8Role role);
// Changes notes only. Sound, automation, feel, follow actions and pattern length survive.
Pattern generate8(const Pattern& source, Gen8Role role, const Gen8Settings& settings,
                  const Gen8Voice& voice, unsigned root, unsigned scale);
Pattern mutate8(const Pattern& source, Gen8Role role, const Gen8Settings& settings,
                const Gen8Voice& voice, unsigned root, unsigned scale);
enum class BreakAction : uint8_t { Genre, Straight, Random, Mutate, Fill };
struct BreakMap { std::array<uint8_t,16> slices {}; unsigned bars=2; };
// Source: Rhythmer gen_break_midi + break_sequence.c. Native sample keys, no firmware command notes.
Pattern break_generate(const Pattern& source, BreakAction action, const Gen8Settings& settings,
                       const BreakMap& map, unsigned bpm);
} // namespace rhythmer
