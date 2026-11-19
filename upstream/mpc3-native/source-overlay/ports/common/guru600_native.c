/* SPDX-License-Identifier: GPL-3.0-only
 * Complete original Guru-600 voice/allocator/modulation/FX. Sequential factory
 * programs and the original P600 extension bank are retained. Unrelated sample
 * banks are not bundled; there is no firmware emulation. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fm1_core.h"
volatile uint8_t fm_vel_sens;
static void events_block(uint32_t frames){(void)frames;engine_block(&trk[0]);}
#include "preset_helpers.h"
#include "original_preset_params.h"
#define NATIVE_MAX_PARAMS 512
static unsigned selected,ids[P_COUNT],nids;
static const uint8_t globals[]={G_DTIME,G_DFDBK,G_DCOLOR,G_DMIX,G_RSIZE,G_RDAMP,G_CRATE,G_CDEPTH,G_RTYPE};
static void apply_program(unsigned k){selected=k%P600_NFIX;host_preset(&trk[0],ENGI_P600,selected);preset_params(ENGI_P600,selected,trk[0].p);}
static int native_count(void){return 1+nids+sizeof globals;}
static const char *native_name(int i){return !i?"Factory Program":(unsigned)(i-1)<nids?track_desc(&trk[0],ids[i-1])->label:globals[i-1-nids]==G_RTYPE?"Reverb Type":GP[globals[i-1-nids]].label;}
static int native_min(int i){return !i?0:(unsigned)(i-1)<nids?track_desc(&trk[0],ids[i-1])->min:GP[globals[i-1-nids]].min;}
static int native_max(int i){return !i?P600_NFIX-1:(unsigned)(i-1)<nids?track_desc(&trk[0],ids[i-1])->max:GP[globals[i-1-nids]].max;}
static int native_value(int i){return !i?selected:(unsigned)(i-1)<nids?trk[0].p[ids[i-1]]:song.g[globals[i-1-nids]];}
static void native_control(int i,int v){if(!i)apply_program(v);else if((unsigned)(i-1)<nids)trk[0].p[ids[i-1]]=v;else song.g[globals[i-1-nids]]=v;}
static void native_init(void){host_tracks_init();apply_program(0);for(unsigned i=0;i<P_COUNT;++i){const param_desc_t *d=track_desc(&trk[0],i);if(d->label&&*d->label&&d->max>d->min)ids[nids++]=i;}}
static void native_note(const uint8_t *m){unsigned k=m[0]&0xf0;
    if(k==0x90&&m[2])trk_note_on(&trk[0],m[1],m[2]);else if(k==0x80||(k==0x90&&!m[2]))trk_note_off(&trk[0],m[1]);
    else if(k==0xb0&&(m[1]==120||m[1]==123))for(unsigned n=0;n<128;++n)trk_note_off(&trk[0],n);
    else if(k==0xe0)midi_bend_target[0]=((int)(m[1]|m[2]<<7)-8192)/16;
    else mod_midi(&trk[0],k,m[1],m[2]);
}
static int32_t pcm[2*CTL];static unsigned cursor=CTL;
static void native_audio(int16_t *out,int n){for(int f=0;f<n;++f){if(cursor==CTL){mix_block(pcm,CTL);cursor=0;}
    for(int c=0;c<2;++c){int32_t v=pcm[2*cursor+c];out[2*f+c]=(int16_t)(v>32767?32767:v<-32768?-32768:v);}++cursor;}}
#define NATIVE_TEMPO 1
static void native_tempo(float bpm){song.g[G_BPM]=(int16_t)(bpm>240?240:bpm<40?40:bpm);}
#define NATIVE_DISPLAY 1
static int native_display(int i,char *out,int capacity){
    if(!i)return snprintf(out,(size_t)capacity,"%u",selected);
    const param_desc_t *d=(unsigned)(i-1)<nids?track_desc(&trk[0],ids[i-1]):&GP[globals[i-1-nids]];
    char value[192]={0};const char *unit="";param_format(d,native_value(i),value,&unit);
    return snprintf(out,(size_t)capacity,"%s%s%s",value,unit[0]?" ":"",unit);
}
#define NATIVE_MIN_STATE_PARAMS (native_count()-1)
#include "fm1_facade.h"
