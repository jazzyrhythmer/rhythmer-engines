/* SPDX-License-Identifier: GPL-3.0-only
 * Native boundary around the complete original Prophet/Hydra voice, modulation
 * and FX units. Original MPL files retain their notices in original/.
 * Firmware UI, USB, storage and sequencer units are not executed here. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fm1_core.h"
static void events_block(uint32_t frames){
#if !FM1_HYDRA
    lfo_block_units=int_clock_units(frames);
#else
    (void)frames;
#endif
    engine_block(&trk[0]);
}
static int midi_track_emit(uint32_t packet){(void)packet;return 1;}
static void midi_out_event(uint32_t packet){(void)packet;}
#if !FM1_HYDRA
static uint32_t punch_clock_samples(void){return 0;}
#endif
#include "preset_helpers.h"
#define NATIVE_MAX_PARAMS 1024
#if FM1_HYDRA
#define NATIVE_ENGINE ENGI_HYDRA
#else
#define NATIVE_ENGINE ENGI_PROPHET
#endif
static unsigned selected,track_ids[P_COUNT],ntrack,patch_ids[512],npatch;
static const uint8_t global_ids[]={G_BPM,G_TUNE,G_FXMODE,G_FXRETURN,G_MFXTYPE,G_MFXMIX,G_MFXRATE,G_MFXCHAR,G_MFXTONE,G_MFX2TYPE,G_MFX2MIX,G_MFX2RATE,G_MFX2CHAR,G_MFX2TONE,G_MFX3TYPE,G_MFX3MIX,G_MFX3RATE,G_MFX3CHAR,G_MFX3TONE};
static int native_count(void){return 1+(int)(ntrack+npatch+sizeof global_ids);}
static const param_desc_t *patch_desc(unsigned i){
#if FM1_HYDRA
    return &HY_PANEL[patch_ids[i]];
#else
    return &P5_PANEL[patch_ids[i]];
#endif
}
static const char *native_name(int i){
    if(!i)return "Factory Program";--i;if((unsigned)i<ntrack)return track_desc(&trk[0],track_ids[i])->label;
    i-=ntrack;if((unsigned)i<npatch)return patch_desc(i)->label;return global_ids[i-npatch]==G_FXMODE?"FX Routing":global_ids[i-npatch]==G_FXRETURN?"FX Return":GP[global_ids[i-npatch]].label;
}
static int native_min(int i){
    if(!i)return 0;--i;if((unsigned)i<ntrack)return track_desc(&trk[0],track_ids[i])->min;
    i-=ntrack;return (unsigned)i<npatch?patch_desc(i)->min:GP[global_ids[i-npatch]].min;
}
static int native_max(int i){
    if(!i)return ENGINES[NATIVE_ENGINE]->npresets-1;--i;if((unsigned)i<ntrack)return track_desc(&trk[0],track_ids[i])->max;
    i-=ntrack;return (unsigned)i<npatch?patch_desc(i)->max:GP[global_ids[i-npatch]].max;
}
static int native_value(int i){
    if(!i)return selected;--i;if((unsigned)i<ntrack)return trk[0].p[track_ids[i]];i-=ntrack;
    if((unsigned)i<npatch){
#if FM1_HYDRA
        return hy_get(&trk[0],patch_ids[i]);
#else
        return p5_patch_of(&trk[0])->raw[patch_ids[i]];
#endif
    }return song.g[global_ids[i-npatch]];
}
static void native_control(int i,int v){
    if(!i){selected=v;host_preset(&trk[0],NATIVE_ENGINE,(unsigned)v);return;}--i;
    if((unsigned)i<ntrack){trk[0].p[track_ids[i]]=v;return;}i-=ntrack;
    if((unsigned)i<npatch){
#if FM1_HYDRA
        hy_edit_value(&trk[0],patch_ids[i],v);
#else
        p5_edit_value(&trk[0],patch_ids[i],v);
#endif
    }else {unsigned id=global_ids[i-npatch];
        if(id>=G_MFXTYPE&&id<=G_MFX3CHAR&&(id-G_MFXTYPE)%FX_PARAMS==0)shared_fx_select(song.g,(id-G_MFXTYPE)/FX_PARAMS,v);
        else song.g[id]=v;
    }
}
static void native_init(void){
#if !FM1_HYDRA
    trk_voices_bind();
#endif
#if !FM1_HYDRA
    midi_insert_init();
#endif
    cz_init();
    for(unsigned i=0;i<G_COUNT;++i)song.g[i]=GP[i].def;
    for(unsigned k=0;k<NTRK;++k)for(unsigned i=0;i<P_E0;++i)trk[k].p[i]=TP[i].def;
    song.master_q12=4096;host_preset(&trk[0],NATIVE_ENGINE,0);
    for(unsigned i=0;i<P_COUNT;++i){const param_desc_t *d=track_desc(&trk[0],i);if(d->label&&d->max>d->min)track_ids[ntrack++]=i;}
#if FM1_HYDRA
    for(unsigned i=0;i<HY_SPARE;++i)if(HY_PANEL[i].label&&HY_PANEL[i].max>HY_PANEL[i].min)patch_ids[npatch++]=i;
#else
    for(unsigned i=0;i<NELEM(P5_PANEL);++i)if(P5_PANEL[i].label&&P5_PANEL[i].max>P5_PANEL[i].min)patch_ids[npatch++]=i;
#endif
    eng_state_prepare(&trk[0]);
}
static void native_note(const uint8_t *m){unsigned kind=m[0]&0xf0;
    if(kind==0x90&&m[2])trk_note_on(&trk[0],m[1],m[2]);
    else if(kind==0x80||(kind==0x90&&!m[2]))trk_note_off(&trk[0],m[1]);
    else if(kind==0xb0&&(m[1]==120||m[1]==123))for(unsigned k=0;k<128;++k)trk_note_off(&trk[0],k);
    else if(kind==0xe0)midi_bend_target[0]=((int)(m[1]|m[2]<<7)-8192)/16;
    else mod_midi(&trk[0],kind,m[1],m[2]);
}
static int32_t pcm[2*CTL];static unsigned cursor=CTL;
static void native_audio(int16_t *out,int frames){for(int f=0;f<frames;++f){
    if(cursor==CTL){mix_block(pcm,CTL);cursor=0;}
    for(int c=0;c<2;++c){int32_t v=pcm[2*cursor+c];out[2*f+c]=(int16_t)(v>32767?32767:v<-32768?-32768:v);}++cursor;
}}
#define NATIVE_TEMPO 1
static void native_tempo(float bpm){song.g[G_BPM]=(int16_t)(bpm>240?240:bpm<40?40:bpm);}
#define NATIVE_DISPLAY 1
static int native_display(int i,char *out,int capacity){
    if(!i)return snprintf(out,(size_t)capacity,"%u",selected);
    unsigned n=(unsigned)i-1;const param_desc_t *d=n<ntrack?track_desc(&trk[0],track_ids[n]):n<ntrack+npatch?patch_desc(n-ntrack):global_desc(global_ids[n-ntrack-npatch]);
    char value[192]={0};const char *unit="";param_format(d,native_value(i),value,&unit);
    return snprintf(out,(size_t)capacity,"%s%s%s",value,unit[0]?" ":"",unit);
}
#define NATIVE_MIN_STATE_PARAMS (native_count()-17)
#include "fm1_facade.h"
