/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m2k_core.c"
#define NATIVE_MAX_PARAMS 128
static const uint8_t machines[]={M_GSIN,M_GPRC,M_SID,M_FMS,M_FMD,M_SAW,M_ENS,M_WAV,M_VOX,M_FMP,M_PUL,M_GNOI,M_DKIK,M_DSNR,M_DHAT,M_DCYM};
static unsigned selected,factory_sound;
static void load_factory(unsigned k){
    factory_sound=k%LIB_N;m2k_sound_t sound;lib_sound((int)factory_sound,&sound);
    for(unsigned i=0;i<sizeof machines;i++)if(machines[i]==sound.machine)selected=i;
    PAT(cur_pat)->machine[0]=sound.machine;m_cpy(PAT(cur_pat)->kit[0],sound.kit,P_COUNT);
    for(unsigned l=0;l<3;l++)PAT(cur_pat)->kit[0][P_LFO+8*l+L_TRCK]=trck_enc(0);
    machine_reset(0,sound.machine);m_cpy(trk[0].eff,PAT(cur_pat)->kit[0],P_COUNT);
}
static int native_count(void){return 2+P_COUNT;}
static const char *native_name(int i){return i==P_COUNT+1?"Factory Sound":i?pinfo(0,i-1)->name:"Machine";}
static int native_min(int i){(void)i;return 0;}
static int native_max(int i){return i==P_COUNT+1?LIB_N-1:i&&pinfo(0,i-1)->kind==K_DEST?P_COUNT:i?127:sizeof machines-1;}
static int native_value(int i){return i==P_COUNT+1?(int)factory_sound:i?PAT(cur_pat)->kit[0][i-1]:(int)selected;}
static void native_control(int i,int v){
    if(i==P_COUNT+1){load_factory((unsigned)v);return;}
    if(!i){selected=(unsigned)v;PAT(cur_pat)->machine[0]=machines[v];kit_init(PAT(cur_pat),0,machines[v]);machine_reset(0,machines[v]);m_cpy(trk[0].eff,PAT(cur_pat)->kit[0],P_COUNT);}
    else{unsigned p=(unsigned)i-1;
        /* This host instance owns one sound, not the source's six-track kit. */
        for(unsigned l=0;l<3;l++)if(p==P_LFO+8*l+L_TRCK)v=trck_enc(0);
        PAT(cur_pat)->kit[0][p]=(uint8_t)v;trk[0].eff[p]=(uint8_t)v;}
}
static void native_init(void){
    m2k_init(1);cur_pat=16;sq.playing=0;PAT(cur_pat)->mute=(uint8_t)(M2K_TMASK&~1u);
    for(unsigned t=0;t<M2K_TRACKS;++t)memset(PAT(cur_pat)->step[t],0,sizeof PAT(cur_pat)->step[t]);
    native_control(0,0);seq_tempo();
}
static void native_note(const uint8_t *m){m2k_midi((m[0]&0xf0),m[1],m[2]);}
static int16_t cache[2*M2K_SUB];static unsigned cursor=M2K_SUB;
static void native_audio(int16_t *out,int frames){
    for(int f=0;f<frames;++f){if(cursor==M2K_SUB){m2k_render(cache,M2K_SUB);cursor=0;}
        out[2*f]=cache[2*cursor];out[2*f+1]=cache[2*cursor+1];++cursor;
    }
}
#define NATIVE_CUSTOM_STATE 1
static int native_state_get(char *out,int capacity){
    if(capacity<5+4+P_COUNT*2+1)return 0;
    memcpy(out,"M2K2:",5);snprintf(out+5,5,"%02x%02x",selected,factory_sound);
    for(unsigned j=0;j<P_COUNT;j++)snprintf(out+9+j*2,3,"%02x",PAT(cur_pat)->kit[0][j]);return 9+P_COUNT*2;
}
static void native_state_set(const char* state){
    if(strlen(state)!=9+P_COUNT*2||strncmp(state,"M2K2:",5))return;
    unsigned values[P_COUNT+2];for(unsigned j=0;j<P_COUNT+2;j++){char text[3],*end;memcpy(text,state+5+j*2,2);text[2]=0;values[j]=(unsigned)strtoul(text,&end,16);if(*end)return;}
    if(values[0]>=sizeof machines||values[1]>=LIB_N)return;for(unsigned j=2;j<P_COUNT+2;j++)if(values[j]>127)return;
    native_control(0,(int)values[0]);factory_sound=values[1];for(unsigned j=0;j<P_COUNT;j++)native_control(1+j,(int)values[2+j]);
}
#define NATIVE_TEMPO 1
static void native_tempo(float bpm){unsigned value=(unsigned)(bpm<30?30:bpm>285?285:bpm)-30;if(G()[G_TMPO]!=value){PAT(cur_pat)->glob[G_TMPO]=(uint8_t)value;seq_tempo();}}
#define NATIVE_DISPLAY 1
static int native_display(int i,char *out,int capacity){
    if(i<=0||i>P_COUNT)return snprintf(out,(size_t)capacity,"%d",native_value(i));
    const pi_t *info=pinfo(0,i-1);int v=native_value(i);
    if(info->kind==K_DEST)return snprintf(out,(size_t)capacity,"%s",v?pinfo(0,v-1)->name:"Off");
    char value[192]={0};const char *unit="";fmt(info,0,v,value,&unit);
    return snprintf(out,(size_t)capacity,"%s%s%s",value,unit[0]?" ":"",unit);
}
#include "fm1_facade.h"
