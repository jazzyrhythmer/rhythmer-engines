// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "rhythmer/model/chords.h"
#include "rhythmer/model/scales.h"
#include <cstdio>
#include <cstring>
#include <string>

// Chord progressions (the PROG pad layout): names as the pads show them, and chords stored from held notes. The
// keys each pad plays are prog_keys() (model/scales.h); the files are shell/progression_file.cpp's.
namespace rhythmer {

// A chord's name in the short form a pad shows: "C#min9" C#m9, "F#Maj(add2)/A#" F#add2/A#, "EMaj9" Emaj9,
// "C7(#9#5)" C7#5#9. Names it does not know stay as they are.
inline std::string short_chord_name(const std::string& name) {
    std::string s = name;
    auto replace = [&](const char* from, const char* to) {
        size_t at = 0, n = std::strlen(from), m = std::strlen(to);
        while ((at = s.find(from, at)) != std::string::npos) { s.replace(at, n, to); at += m; }
    };
    replace("Maj(add2)", "add2"); replace("Maj(add9)", "add9"); replace("Maj(addb5)", "(b5)"); replace("Maj(#5)", "aug");
    replace("Maj7", "maj7"); replace("Maj9", "maj9"); replace("Maj13", "maj13");
    replace("min", "m");
    replace("m9(11)", "m11"); replace("9(13)", "13"); replace("(#9#5)", "#5#9"); replace("(#9)", "#9"); replace("(b9)", "b9");
    replace("6/9(#11)", "6/9#11");
    if (s.size() > 1 && s.back() == 'j' && s.compare(s.size() - 3, 3, "Maj") == 0) s.erase(s.size() - 3); // a plain major: its letter
    return s;
}

// The name split for a pad: the chord, and its slash bass ("F#add2/A#": F#add2 and A#); the bass empty without one.
// A 6/9 is no slash chord.
struct PadChordName { std::string chord, bass; };
inline PadChordName pad_chord_name(const std::string& name) {
    std::string s = short_chord_name(name);
    size_t slash = s.rfind('/');
    if (slash == std::string::npos || slash + 1 >= s.size() || s[slash + 1] < 'A' || s[slash + 1] > 'G') return {s, {}};
    return {s.substr(0, slash), s.substr(slash + 1)};
}

// A name moved by `semitones`: each note letter in it ("C#", "Db", "A") becomes the one that many semitones away
// (sharps, as Rhythmer names notes). Alterations ("b5", "#9") follow a digit or "(", never a letter, so they stay.
inline std::string transpose_chord_name(const std::string& name, int semitones) {
    static const int letters[] {9, 11, 0, 2, 4, 5, 7}; // A..G
    std::string out;
    for (size_t k = 0; k < name.size(); ++k) {
        char c = name[k];
        if (c < 'A' || c > 'G') { out += c; continue; }
        int pc = letters[c - 'A'];
        if (k + 1 < name.size() && name[k + 1] == '#') { ++pc; ++k; }
        else if (k + 1 < name.size() && name[k + 1] == 'b') { --pc; ++k; } // a flat: "b" right after the letter
        out += note_letter(unsigned(((pc + semitones) % 12 + 12) % 12));
    }
    return out;
}

// What a pad of the track's progression is called on the pads, in the key K1 moved it to.
inline PadChordName prog_pad_name(const Track& track, unsigned index) {
    if (index >= prog_chords || !track.prog.chords[index].used()) return {};
    return pad_chord_name(transpose_chord_name(track.prog.chords[index].name, track.prog_shift));
}

// Keys held stored as chord `index` of a progression: lowest first, a key once, up to notes_per_step; named as
// name_chord() names them ("" when it does not know the shape: the pad then shows its lowest note).
inline bool store_prog_chord(Progression& prog, unsigned index, const int* keys, unsigned count) {
    if (index >= prog_chords) return false;
    std::array<int, notes_per_step> sorted {};
    unsigned n = 0;
    for (unsigned k = 0; k < count; ++k) {
        if (keys[k] < 0 || keys[k] > 127) continue;
        bool seen = false;
        for (unsigned j = 0; j < n; ++j) seen |= sorted[j] == keys[k];
        if (!seen && n < notes_per_step) sorted[n++] = keys[k];
    }
    if (!n) return false;
    sort_notes(sorted, n);
    auto& chord = prog.chords[index];
    chord = ProgChord {};
    for (unsigned k = 0; k < n; ++k) chord.notes[k] = uint8_t(sorted[k]);
    chord.count = uint8_t(n);
    std::string name = name_chord(sorted.data(), n);
    if (name.empty()) name = note_name(sorted[0]);
    std::snprintf(chord.name, sizeof(chord.name), "%s", name.c_str());
    prog.count = uint8_t(std::max<unsigned>(prog.count, index + 1));
    return true;
}

} // namespace rhythmer
