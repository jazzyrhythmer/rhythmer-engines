// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct Fm1GenEvent {uint16_t step,length;uint8_t track,key,velocity,flags;int8_t micro;uint32_t tick,ticks;uint8_t ratchet,chance;};
struct Fm1GenResult {unsigned count,steps,tempo,root,scale,seed;unsigned role[4],recipe[4],engine[4],track_ticks[4],grid[4];struct Fm1GenEvent events[8192];};
unsigned rhythmer_fm1_bridge_size(void);
void rhythmer_fm1_generate(unsigned genre,unsigned seed,unsigned energy,unsigned complex,int press,struct Fm1GenResult* out);
void acid_generate(uint8_t*,uint8_t*,unsigned,const uint8_t[5],uint32_t*,int,unsigned,uint16_t);
#ifdef __cplusplus
}
#endif
