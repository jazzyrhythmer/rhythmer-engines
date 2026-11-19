/* SPDX-License-Identifier: GPL-3.0-only
 * Original OberVave / S1Vave synthesis and FX units, with a native host boundary.
 * Platform, screen, flash, updater and firmware sequencer are not included.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FS 44100u
#define HALF_FRAMES 256u
#include "obv_dsp.h"
#include "obv_params.h"
#include "obv_fx.h"
#include "obv_engine.h"
#include "obv_patches.h"
static unsigned program;
#if FM1_S1
static int16_t draw[16];static uint16_t masks[4];
static uint32_t seq_b_own(void){return 0;}
static uint32_t seq_b_dry(void){return 0;}
static inline const int16_t *seq_draw_table(void){return draw;}
static inline const uint16_t *seq_chop_masks(void){return masks;}
static void apply_program(uint32_t p){
    program=p%NFACT;patch_from_factory(&live,&FACTORY[program]);
    memcpy(draw,FACT_WAVE[program].draw,sizeof draw);memcpy(masks,FACT_WAVE[program].chop,sizeof masks);
}
#else
static int seq_phase(uint32_t *p){*p=0;return 0;}
static int seq_starting(void){return 0;}
static uint32_t seq_q8_now(uint32_t p){(void)p;return 0xffffffffu;}
static void apply_program(uint32_t p){program=p%NFACT;patch_from_factory(&live,&FACTORY[program]);}
#endif
static void seq_tick(void){}
#define NATIVE_MAX_PARAMS 256
static int native_count(void){return 1+NPARAM;}
static const char *native_name(int i){return i?PDEF[i-1].name:"Factory Program";}
static int native_min(int i){(void)i;return 0;}
static int native_max(int i){return i?PDEF[i-1].max:NFACT-1;}
static int native_value(int i){return i?live.v[i-1]:(int)program;}
static void native_control(int i,int v){if(!i)apply_program((unsigned)v);else live.v[i-1]=(uint8_t)v;}
static void native_init(void){
    apply_program(0);
#if !FM1_S1
    for(unsigned i=1;i<NPART;++i)patch_defaults(&live_p[i]);
#endif
    engine_init();
}
static void native_note(const uint8_t *m){
    unsigned kind=m[0]&0xf0;
    if(kind==0x90||kind==0x80)note_event(kind==0x90&&m[2],m[1],m[2]);
    else if(kind==0xe0)ctl_bend=(int)(m[1]|(m[2]<<7))-8192;
    else if(kind==0xb0){if(m[1]==120||m[1]==123)ctl_panic=1;else if(m[1]==64)ctl_sustain=m[2]>=64;else if(m[1]==7)ctl_cc7=m[2];}
}
static int32_t pcm[512];static unsigned cursor=256;
static void native_audio(int16_t *out,int frames){
    for(int f=0;f<frames;++f){
        if(cursor==256){memset(pcm,0,sizeof pcm);engine_audio(pcm,256);cursor=0;}
        /* These original engines already return Q15, unlike Niagara's Q23. */
        for(unsigned c=0;c<2;++c){int32_t v=pcm[2*cursor+c];out[2*f+c]=(int16_t)(v>32767?32767:v<-32768?-32768:v);}
        ++cursor;
    }
}
#define NATIVE_TEMPO 1
static void native_tempo(float bpm){ctl_tempo=(unsigned)(bpm<40?40:bpm>240?240:bpm);ctl_ext_quarter=0;}
#if FM1_S1
#include "s1vave_display.h"
#else
#include "obervave_display.h"
#endif
#define NATIVE_DISPLAY 1
static int native_display(int i,char *out,int capacity){if(!i)return snprintf(out,capacity,"%d",native_value(i));char value[64]={0};param_value(value,(unsigned)i-1,(unsigned)native_value(i));return snprintf(out,capacity,"%s",value);}
#include "fm1_facade.h"
