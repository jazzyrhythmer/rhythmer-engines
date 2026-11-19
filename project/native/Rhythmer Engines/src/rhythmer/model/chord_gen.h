// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 jazzy rhythmer
#pragma once
#include "rhythmer/model/gen8.h"
#include <string>
namespace rhythmer {
struct ChordGenSettings {
    uint8_t type=0,genre=41,progression=0,chords=4,bars=4,rhythm=35,gate=80,color=3,voicing=0,octave=4;
    uint8_t energy=65,play=0,rate=1,local=0,root=0,scale=0;
    uint32_t seed=1,harmony=1;
};
int chord_gen_value(const ChordGenSettings&,unsigned id,unsigned project_root,unsigned project_scale);
std::array<int,2> chord_gen_bounds(unsigned id);
void chord_gen_set(ChordGenSettings&,unsigned id,int value,unsigned project_root,unsigned project_scale);
const char* chord_gen_param_name(unsigned);
std::string chord_gen_param_text(const ChordGenSettings&,unsigned,unsigned,unsigned);
const char* chord_play_name(unsigned);
const char* chord_progression_name(unsigned);
const char* chord_color_name(unsigned);
const char* chord_voicing_name(unsigned);
Pattern generate_chords(const Pattern&,const ChordGenSettings&,const Gen8Voice&,unsigned root,unsigned scale,unsigned bar_ticks,bool mutate=false);
int gen_follow_note(int note,unsigned from_root,unsigned from_mask,unsigned to_root,unsigned to_mask,const Gen8Voice&);
}
