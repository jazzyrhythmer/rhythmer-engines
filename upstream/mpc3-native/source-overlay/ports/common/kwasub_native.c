/* SPDX-License-Identifier: GPL-3.0-only
 * Original K-WASUB voice/FX and its 808/909/dub/acoustic drum synthesis.
 * The generated sample layer is made mathematically by the original generator,
 * not recordings or ROM. The original files and generator remain in original/.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FS 44100u
#define HALF_FRAMES 256u
#define FM1_TICKS_PER_US 24u
#define KS_DUAL 0
#define DINGHY_HOST 1
#define POOL
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
static uint32_t fm1_ticks(void){return 0;}
#include "ksub_tables.h"
#include "ksub_samples.h"
#include "ksub_data.c"
#include "ksub_dsp.c"
#define NATIVE_MAX_PARAMS 256
#ifndef FM1_KWASUB_KIT
#define FM1_KWASUB_KIT 0
#endif
static unsigned selected;
static const char *lanes[8]={"Kick","Snare","Clap","Closed Hat","Open Hat","Tom","Percussion","Cymbal"};
static const char *knobs[4]={"Level","Tune","Decay","Tone"};
static int native_count(void){return FM1_KWASUB_KIT?34:1+NP;}
static const char *native_name(int i){
    if(!FM1_KWASUB_KIT)return i?PD[i-1].name:"Factory Program";
    if(i<2)return i?"Volume":"Kit";static char text[80];snprintf(text,sizeof text,"%s %s",lanes[(i-2)/4],knobs[(i-2)%4]);return text;
}
static int native_min(int i){return FM1_KWASUB_KIT?(i>=2&&(i-2)%4==1?-64:0):(i?PD[i-1].lo:0);}
static int native_max(int i){return FM1_KWASUB_KIT?(!i?3:i>=2&&(i-2)%4==1?63:127):(i?PD[i-1].hi:NFACTORY-1);}
static int native_value(int i){
    if(!FM1_KWASUB_KIT)return i?patch[i-1]:(int)selected;
    return !i?dk.kit:i==1?dk.vol:dk.prm[(i-2)/4][(i-2)%4];
}
static void native_control(int i,int v){
    if(!FM1_KWASUB_KIT){if(i)patch[i-1]=(int8_t)v;else{char name[64];selected=(unsigned)v;preset_factory((unsigned)v,patch,name);}return;}
    if(!i)dk.kit=(uint8_t)v;else if(i==1)dk.vol=(uint8_t)v;else dk.prm[(i-2)/4][(i-2)%4]=(int8_t)v;
}
static void native_init(void){
    char name[64];preset_factory(0,patch,name);
    static const int8_t defaults[8][4]={{105,0,64,40},{95,0,56,70},{90,0,60,50},{80,0,60,64},
        {78,0,60,64},{95,0,64,64},{85,0,64,64},{70,0,64,64}};
    dk.magic=DK_MAGIC;dk.vol=100;memcpy(dk.prm,defaults,sizeof defaults);
    if(FM1_KWASUB_KIT)dk.mode=1;
}
static void native_note(const uint8_t *m){
    unsigned kind=m[0]&0xf0;
    if(kind==0xb0&&(m[1]==120||m[1]==123)){ev_push(EV_PANIC,0,0);for(unsigned i=0;i<NDK;++i)dkv[i].on=0;return;}
    if(FM1_KWASUB_KIT){if(kind==0x90&&m[2]&&m[1]>=36&&m[1]<44)ev_push(EV_DRUM,m[1]-36,m[2]);return;}
    if(kind==0x90&&m[2])ev_push(EV_ON,m[1],m[2]);
    else if(kind==0x80||(kind==0x90&&!m[2]))ev_push(EV_OFF,m[1],0);
}
static int32_t pcm[512];static unsigned cursor=256;
static void native_audio(int16_t *out,int frames){
    for(int f=0;f<frames;++f){if(cursor==256){memset(pcm,0,sizeof pcm);app_audio(pcm,256);cursor=0;}
        for(unsigned c=0;c<2;++c){int32_t v=pcm[2*cursor+c];out[2*f+c]=(int16_t)(v>32767?32767:v<-32768?-32768:v);}++cursor;
    }
}
#define NATIVE_TEMPO 1
static void native_tempo(float bpm){ks_bpm10=(uint32_t)(bpm>300?3000:bpm<10?100:bpm*10);}
#include "fm1_facade.h"
