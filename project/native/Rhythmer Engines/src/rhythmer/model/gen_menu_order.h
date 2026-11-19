// Original FM-1 GEN_ORDER mapped by name into host tables; SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "rhythmer/model/gen8_tables.h"
namespace rhythmer {
inline constexpr unsigned gen_fm1_menu[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,53,54,55,56,58,52,57};
inline const char* gen_menu_name(unsigned value){return value==0?"EXPLORE":value==1?"RANDOM":gen8_genres[gen_fm1_menu[(value-2)%59]].name;}
}
