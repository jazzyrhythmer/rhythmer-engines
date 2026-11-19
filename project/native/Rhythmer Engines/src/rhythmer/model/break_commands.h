// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <string>
namespace rhythmer {
inline constexpr const char* break_roll_labels[16]={"1/4 REPEAT","BUILD","STRETCH","ECHO","1/8 REPEAT","1/8 DOWN","1/8 UP","1/8 SHRINK","1/16 REPEAT","1/16 DOWN","1/16 UP","1/16 SHRINK","1/32 REPEAT","1/32 DOWN","1/32 UP","1/32 SHRINK"};
inline std::string break_event_label(int key){
    if(key>=0&&key<16)return break_roll_labels[key];
    if(key==16)return "SOURCE A";
    if(key==17)return "SOURCE B";
    if(key==18)return "RETURN";
    if(key>=36&&key<164)return "CHOP "+std::to_string((key-36)%16+1);
    return "UNASSIGNED";
}
}
