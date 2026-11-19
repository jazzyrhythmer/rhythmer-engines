/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FS 44100u
#define HALF_FRAMES 256u
#define FM1_POOL
static uint32_t fm1_ticks(void){return 0;}
static int audio_worker_submit(void (*work)(void*),void *ctx){(void)work;(void)ctx;return 0;}
static void audio_worker_join(void){}
#include "cs1_dsp.h"
#include "cs1_voice.h"
#include "cs1_presets.h"
static struct{uint8_t sound,tone[2];}set;
#include "sounds.h"
#include "controls.h"
#define NATIVE_MAX_PARAMS 256
static int native_count(void){return 1+sizeof controls/sizeof controls[0];}
static const char *native_name(int i){return i?controls[i-1].name:"Factory Program";}
static int native_min(int i){(void)i;return 0;}
static int native_max(int i){return i?(int)controls[i-1].max:NSOUND-1;}
static int native_value(int i){return i?((uint8_t*)&patch)[controls[i-1].offset]:set.sound;}
static void native_control(int i,int v){if(i)((uint8_t*)&patch)[controls[i-1].offset]=(uint8_t)v;else sound_load((unsigned)v);}
static void native_init(void){patch=INIT_PATCH;sound_load(0);}
static void native_note(const uint8_t *m){
    unsigned kind=m[0]&0xf0;
    if(kind==0x90&&m[2])voice_on(m[1],m[2]);
    else if(kind==0x80||(kind==0x90&&!m[2]))voice_off(m[1]);
    else if(kind==0xb0&&(m[1]==120||m[1]==123))for(unsigned k=0;k<128;++k)voice_off(k);
    else if(kind==0xe0)bend256=((int)(m[1]|m[2]<<7)-8192)/16;
    else if(kind==0xa0)voices_pressure(m[1],m[2]);
    else if(kind==0xd0)voices_pressure(-1,m[1]);
}
static int32_t pcm[512];static unsigned cursor=256;
static void native_audio(int16_t *out,int frames){
    for(int f=0;f<frames;++f){if(cursor==256){memset(pcm,0,sizeof pcm);voices_block(pcm,256);cursor=0;}
        for(unsigned c=0;c<2;++c){int32_t v=pcm[2*cursor+c];out[2*f+c]=(int16_t)(v>32767?32767:v<-32768?-32768:v);}++cursor;
    }
}
#include "fm1_facade.h"
