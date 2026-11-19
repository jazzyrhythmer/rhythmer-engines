/* SPDX-License-Identifier: GPL-3.0-only
 * Elixir is the canonical LXR-family port. Original native engine/host boundary;
 * external factory banks are omitted; original GPL Initkit is retained; no duplicate LXR engine or device firmware image.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tests/engine_host.h"
static uint32_t cc_map_at(uint32_t cc){(void)cc;return 0;}
#define NATIVE_MAX_PARAMS 1024
static uint16_t ids[1024];static int count;
static unsigned selected;
static void apply_program(unsigned k){
    if(k<FACTORY_NKITS)kit_factory_load(k);
    else {uint8_t values[KIT_LEN];kit_init_values(values);for(unsigned i=0;i<KIT_LEN;i++)par_set(kit_par(i),values[i]);}
    selected=k;snd_service();
}
static int native_count(void){return count+1;}
static const char *native_name(int i){return i?par_name(ids[i-1]):"Kit";}
static int native_min(int i){return i?(int)par_min(ids[i-1]):0;}
static int native_max(int i){return i?(int)par_max(ids[i-1]):FACTORY_NKITS;}
static int native_value(int i){return i?par[ids[i-1]]:(int)selected;}
static void native_control(int i,int v){if(i)par_set(ids[i-1],(unsigned)v);else apply_program((unsigned)v);}
static void native_init(void){
    engine_init();
    for(unsigned i=0;i<KIT_LEN;++i){unsigned p=kit_par(i);if(par_max(p)>par_min(p)&&count<1023)ids[count++]=(uint16_t)p;}
    apply_program(FACTORY_NKITS);
}
static void native_note(const uint8_t *m){
    static const uint8_t notes[7]={36,45,47,38,49,42,46};
    unsigned kind=m[0]&0xf0;
    if(kind==0xb0&&(m[1]==120||m[1]==123)){for(unsigned i=0;i<6;++i){vc[i].aeg.value=0;vc[i].aeg.state=EG_STOPPED;vc[i].gain=0;}return;}
    if(kind!=0x90||!m[2])return;
    for(unsigned i=0;i<7;++i)if(notes[i]==m[1]){snd_service();voice_trigger(i,m[2],SEQ_DEFAULT_NOTE);break;}
}
static int32_t pcm[2*BLOCK];static unsigned cursor=BLOCK;
static void native_audio(int16_t *out,int frames){
    for(int f=0;f<frames;++f){if(cursor==BLOCK){engine_block(pcm);cursor=0;}
        for(unsigned c=0;c<2;++c)out[2*f+c]=(int16_t)sat16(pcm[2*cursor+c]);++cursor;
    }
}
#define NATIVE_DISPLAY 1
static int native_display(int i,char *out,int capacity){
    if(!i){if(selected>=FACTORY_NKITS)return snprintf(out,(size_t)capacity,"Initkit");char name[9]={0};kit_factory_name(selected,name);return snprintf(out,(size_t)capacity,"%s",name);}
    char value[192]={0};par_format(value,ids[i-1],(uint32_t)native_value(i),sizeof value);
    return snprintf(out,(size_t)capacity,"%s",value);
}
#define NATIVE_SYNC 1
static void native_sync(void){
    snd_service();
    /* A loaded/silent voice has no running ramp to preserve. Settle its
       mixer reference before its first hit; otherwise volume zero leaks
       the old Initkit level during the first few milliseconds. Live edits
       retain the original per-sample volume smoothing. */
    for(unsigned i=0;i<6;++i)if(vc[i].aeg.state==EG_STOPPED)vc[i].vol_s=vc[i].vol<<9;
}
#include "fm1_facade.h"
