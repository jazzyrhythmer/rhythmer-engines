// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "rhythmer/model/limits.h"
#include <array>
#include <cstdint>

// A chord progression as a track keeps it (the PROG pad layout, model/progression.h): the chords of an MPC 3
// .progression file, one a pad over banks A-C. The file is read and written by shell/progression_file.cpp.
namespace rhythmer {

constexpr unsigned prog_chords = 48;     // banks A-C of sixteen
constexpr unsigned prog_chord_name = 20; // "F#Maj(add2)/A#" and the like, as the file names it
constexpr unsigned prog_name_length = 64;

struct ProgChord {
    std::array<uint8_t, notes_per_step> notes {}; // MIDI notes, lowest first
    uint8_t count = 0;                             // 0: no chord on this pad
    bool root = false;                             // the file's "Root" role: the chord the key is named after
    char name[prog_chord_name] {};
    bool used() const { return count > 0; }
    bool operator==(const ProgChord& o) const;
};

struct Progression {
    char name[prog_name_length] {}; // the file's own name ("Prisms 2-A Lydian or C#m or E (R&B)")
    char file[prog_name_length] {}; // its file name without .progression; empty: made on the MPC and not saved yet
    uint8_t root = 0;               // the file's rootNote, a pitch class
    uint8_t count = 0;              // chords up to the last one used
    std::array<ProgChord, prog_chords> chords {};
    bool empty() const { return count == 0; }
    bool operator==(const Progression& o) const;
};

inline bool ProgChord::operator==(const ProgChord& o) const {
    if (count != o.count || root != o.root || notes != o.notes) return false;
    for (unsigned n = 0; n < prog_chord_name; ++n) if (name[n] != o.name[n]) return false;
    return true;
}
inline bool Progression::operator==(const Progression& o) const {
    if (root != o.root || count != o.count || chords != o.chords) return false;
    for (unsigned n = 0; n < prog_name_length; ++n) if (name[n] != o.name[n] || file[n] != o.file[n]) return false;
    return true;
}

} // namespace rhythmer
