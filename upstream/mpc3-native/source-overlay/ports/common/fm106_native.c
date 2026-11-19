/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define DINGHY_DUAL_CORE 0
#define FELUCCA_BLE 0
#define FELUCCA_UAC 0
#define FM106_RAM_FUNC
#define HALF_FRAMES 256u
#define FS 44100u
static uint32_t fm1_ms;
static uint32_t fm1_ticks(void){return 0;}
static void fm1_irq_off(void){}
static void fm1_irq_on(void){}
#include "fm106_engine.h"
#define NATIVE_MAX_PARAMS 128
static const uint8_t control_ids[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,
    23,24,25,27,28,29,30,31,33,34,35,36,37,38,39,45,46,47,48,49};
static const char *names[]={"Bend DCO","Bend VCF","Arp Rate","LFO Rate","LFO Delay","DCO LFO","DCO PWM",
    "Sub Level","Noise Level","HPF Mode","VCF Cutoff","VCF Resonance","VCF Envelope","VCF LFO","VCF Keytrack",
    "VCA Level","Attack","Decay","Sustain","Release","Pulse On","Saw On","Sub On","Chorus I","Chorus II",
    "DCO Range","Arp Mode","Arp Octaves","PWM Mode","Filter Polarity","VCA Mode","FX Type","FX Mix","FX Rate",
    "FX Character","Reverb On","Reverb Mix","Reverb Decay","Shimmer","Voice Mode"};
static int native_count(void){return 2+sizeof(control_ids);}
static const char *native_name(int i){return i==1+sizeof(control_ids)?"Native Arp Enable":i?names[i-1]:"Factory Program";}
static int native_min(int i){return i&&i<=sizeof(control_ids)&&control_ids[i-1]==31?1:0;}
static int native_max(int i){
    if(!i)return FM106_NUM_FACTORY_PRESETS-1;
    if(i==1+sizeof(control_ids))return 1;
    switch(control_ids[i-1]){
        case 9:return 3;case 23:case 24:case 25:case 27:case 28:case 33:case 34:case 35:case 45:return 1;
        case 29:case 30:case 49:return 2;case 31:return 3;case 36:return FM106_FX_COUNT-1;default:return 127;
    }
}
static int native_value(int i){return i==1+sizeof(control_ids)?g_synth.arp_on:i?fm106_param_get(&g_synth,control_ids[i-1]):g_synth.patch_idx;}
static void native_control(int i,int v){if(i==1+sizeof(control_ids))fm106_arp_enable(&g_synth,v);else if(i)fm106_param_set(&g_synth,control_ids[i-1],(uint8_t)v);else fm106_load_preset(&g_synth,(unsigned)v);}
static void native_init(void){fm106_engine_init(&g_synth);}
static void native_note(const uint8_t *m){
    unsigned kind=m[0]&0xf0;
    if(kind==0x90&&m[2])fm106_key_on(&g_synth,m[1],m[2]);
    else if(kind==0x80||(kind==0x90&&!m[2]))fm106_key_off(&g_synth,m[1]);
    else if(kind==0xe0)g_synth.pitch_bend_val=(int16_t)((m[1]|m[2]<<7)-8192);
    else if(kind==0xb0&&m[1]==64){unsigned old=g_synth.hold_pedal;g_synth.hold_pedal=m[2]>=64;if(old&&!g_synth.hold_pedal)for(unsigned vi=0;vi<FM106_NUM_VOICES;++vi)if(!g_synth.voice[vi].gate)fm106_adsr_gate_off(&g_synth.voice[vi].adsr);}
    else if(kind==0xb0&&(m[1]==120||m[1]==123)){g_synth.hold_pedal=0;for(unsigned k=0;k<128;++k)fm106_key_off(&g_synth,k);memset(g_synth.arp_pool,0,sizeof g_synth.arp_pool);}
}
static int32_t pcm[512];static unsigned cursor=256;
static void native_audio(int16_t *out,int frames){
    for(int f=0;f<frames;++f){
        if(cursor==256){memset(pcm,0,sizeof pcm);fm106_audio_render(pcm,256);cursor=0;}
        for(unsigned c=0;c<2;++c){int32_t v=pcm[2*cursor+c];out[2*f+c]=(int16_t)(v>32767?32767:v<-32768?-32768:v);}++cursor;
    }
}
#define NATIVE_MIN_STATE_PARAMS (native_count()-1)
#include "fm1_facade.h"
