/* SPDX-License-Identifier: GPL-3.0-only
 * Original FMini-1 engine, allocator, envelopes, modulation and FX. The generated
 * prefix includes only original DSP units, ending before USB/platform/sequence
 * emulation. Rhythmer supplies MIDI, project storage and the sequencer.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fm1_core.h"
/* This original firmware setting lives in its sequencer unit. Keep its
 * original OFF default while exposing it independently of that sequencer. */
volatile uint8_t fm_vel_sens;
static void events_block(uint32_t frames){(void)frames;}
#include "preset_helpers.h"
#include "patch_names.h"
#define NATIVE_MAX_PARAMS 512
static unsigned selected;static unsigned track_ids[P_COUNT],ntrack;
static const uint8_t global_ids[]={G_DTIME,G_DFDBK,G_DCOLOR,G_DMIX,G_RSIZE,G_RDAMP,G_CRATE,G_CDEPTH};
enum { X_RTYPE, X_DRIVE_TYPE, X_DRIVE_TONE, X_DRIVE_OUTPUT, X_DELAY_TIME, X_CHORUS_TYPE,
       X_FILTER_RELEASE, X_MOD_WHEEL, X_BEND_RANGE, X_DETUNE2, X_DETUNE3, X_RELEASE_LINK, X_SWITCHES, X_COUNT=X_SWITCHES+10 };
static const unsigned extra_globals[]={G_RTYPE,G_INFO,G_DRLVL,G_MIDI,G_DRREV,G_ENGGO};
static const char* extra_names[]={"Reverb Type","Drive Type","Drive Tone","Drive Output","Delay Time","Chorus Type",
    "Filter Release","Mod Wheel","Pitch Bend Range","OSC 2 Detune Cents","OSC 3 Detune Cents","Release Follows Decay",
    "OSC 1 On","OSC 2 On","OSC 3 On","Noise On","Pink Noise","Mod to Osc","Mod to Filter","Glide On","LFO Square","High Pass On"};
static const uint8_t switch_bytes[]={FG_SW1,FG_SW1,FG_SW1,FG_SW1,FG_SW1,FG_SW1,FG_SW1,FG_SW2,FG_SW2,FG_SW2};
static const uint8_t switch_masks[]={FS_O1,FS_O2,FS_O3,FS_NZ,FS_PINK,FS_MOSC,FS_MFLT,FT_GLIDE,FT_SQR,FT_HP};
static unsigned bend_mode;
#include "factory_extras.h"
static int extra_base(void){return 2+(int)ntrack+FG_NUM+sizeof global_ids;}
static int native_count(void){return extra_base()+X_COUNT;}
static const char *native_name(int i){
    if(i>=extra_base())return extra_names[i-extra_base()];
    if(!i)return "Factory Program";--i;
    if((unsigned)i<ntrack)return track_desc(&trk[0],track_ids[i])->label;
    i-=(int)ntrack;if(i<FG_NUM)return patch_names[i];i-=FG_NUM;
    return i<(int)sizeof global_ids?GP[global_ids[i]].label:"Velocity Sensitivity";
}
static int native_min(int i){
    if(i>=extra_base()){i-=extra_base();return i==X_DETUNE2||i==X_DETUNE3?-700:i<6?GP[extra_globals[i]].min:0;}
    if(!i)return 0;--i;if((unsigned)i<ntrack)return track_desc(&trk[0],track_ids[i])->min;
    i-=(int)ntrack;if(i<FG_NUM)return 0;i-=FG_NUM;return i<(int)sizeof global_ids?GP[global_ids[i]].min:0;
}
static int native_max(int i){
    if(i>=extra_base()){i-=extra_base();return i==X_DETUNE2||i==X_DETUNE3?700:i<6?GP[extra_globals[i]].max:i>=X_SWITCHES||i==X_RELEASE_LINK?1:i==X_BEND_RANGE?3:127;}
    if(!i)return FMINI_NFACTORY-1;--i;if((unsigned)i<ntrack)return track_desc(&trk[0],track_ids[i])->max;
    i-=(int)ntrack;if(i<FG_NUM)return (int)fmini_max((unsigned)i);i-=FG_NUM;return i<(int)sizeof global_ids?GP[global_ids[i]].max:1;
}
static int native_value(int i){
    if(i>=extra_base()){i-=extra_base();if(i<6)return song.g[extra_globals[i]];
        if(i==X_FILTER_RELEASE)return FMINI_FREL(&trk[0]);if(i==X_MOD_WHEEL)return trk[0].mw;if(i==X_BEND_RANGE)return bend_mode;
        if(i==X_RELEASE_LINK)return !(trk[0].frel&128u);
        if(i==X_DETUNE2||i==X_DETUNE3)return fmini_det(fmini_patch[0],i==X_DETUNE2?FG_DC2L:FG_DC3L);
        return !!(fmini_patch[0][switch_bytes[i-X_SWITCHES]]&switch_masks[i-X_SWITCHES]);}
    if(!i)return (int)selected;--i;if((unsigned)i<ntrack)return trk[0].p[track_ids[i]];
    i-=(int)ntrack;if(i<FG_NUM)return fmini_patch[0][i];i-=FG_NUM;return i<(int)sizeof global_ids?song.g[global_ids[i]]:fm_vel_sens;
}
static void native_control(int i,int v){
    if(i>=extra_base()){i-=extra_base();if(i<6){song.g[extra_globals[i]]=v;return;}
        if(i==X_FILTER_RELEASE){if(FMINI_FREL(&trk[0])!=v)trk[0].frel=(uint8_t)(128u|v);return;}if(i==X_MOD_WHEEL){trk[0].mw=v;return;}if(i==X_BEND_RANGE){bend_mode=v;return;}
        if(i==X_RELEASE_LINK){trk[0].frel=v?0:(uint8_t)(128u|FMINI_FREL(&trk[0]));return;}
        uint8_t p[FG_SIZE];memcpy(p,fmini_patch[0],sizeof p);
        if(i==X_DETUNE2||i==X_DETUNE3)fmini_det_put(p,i==X_DETUNE2?FG_DC2L:FG_DC3L,v);
        else {unsigned n=i-X_SWITCHES;p[switch_bytes[n]]=(p[switch_bytes[n]]&~switch_masks[n])|(v?switch_masks[n]:0);}
        fmini_set_patch(0,p);fmini_adopt(0);return;}
    if(!i){selected=(unsigned)v;host_preset(&trk[0],ENGI_FMINI,(unsigned)v);fa_factory_extras(&trk[0],(unsigned)v);return;}--i;
    if((unsigned)i<ntrack){trk[0].p[track_ids[i]]=(int16_t)v;return;}
    i-=(int)ntrack;if(i<FG_NUM){uint8_t p[FG_SIZE];memcpy(p,fmini_patch[0],sizeof p);p[i]=(uint8_t)v;fmini_set_patch(0,p);fmini_adopt(0);}
    else {i-=FG_NUM;if(i<(int)sizeof global_ids)song.g[global_ids[i]]=(int16_t)v;else fm_vel_sens=(uint8_t)v;}
}
static void native_init(void){
    host_tracks_init();host_preset(&trk[0],ENGI_FMINI,0);fa_factory_extras(&trk[0],0);
    for(unsigned i=0;i<P_COUNT;++i){const param_desc_t *d=track_desc(&trk[0],i);if(d->label&&d->max>d->min)track_ids[ntrack++]=i;}
}
static void native_note(const uint8_t *m){
    unsigned kind=m[0]&0xf0;
    if(kind==0x90&&m[2])trk_note_on(&trk[0],m[1],m[2]);
    else if(kind==0x80||(kind==0x90&&!m[2]))trk_note_off(&trk[0],m[1]);
    else if(kind==0xb0&&(m[1]==120||m[1]==123))for(unsigned k=0;k<128;++k)trk_note_off(&trk[0],k);
    else if(kind==0xe0){int bend=(int)(m[1]|m[2]<<7)-8192;int semis=bend_mode==1?12:bend<0&&bend_mode>=2?(bend_mode==2?12:24):2;midi_bend_target[0]=bend*semis*256/(bend<0?8192:8191);}
    else mod_midi(&trk[0],kind,m[1],m[2]);
}
static int32_t pcm[2*CTL];static unsigned cursor=CTL;
static void native_audio(int16_t *out,int frames){
    for(int f=0;f<frames;++f){if(cursor==CTL){mix_block(pcm,CTL);cursor=0;}
        for(unsigned c=0;c<2;++c){int32_t v=pcm[2*cursor+c];out[2*f+c]=(int16_t)(v>32767?32767:v<-32768?-32768:v);}++cursor;
    }
}
#define NATIVE_TEMPO 1
static void native_tempo(float bpm){song.g[G_BPM]=(int16_t)(bpm>240?240:bpm<40?40:bpm);}
#define NATIVE_DISPLAY 1
static int native_display(int i,char *out,int capacity){
    const param_desc_t *d=0;
    if(i>=extra_base()){
        unsigned x=(unsigned)(i-extra_base());if(x<6)d=&GP[extra_globals[x]];
        else if(x==X_FILTER_RELEASE)d=&TP[P_REL];
        else if(x==X_DETUNE2||x==X_DETUNE3)return snprintf(out,(size_t)capacity,"%d ct",native_value(i));
    }else if(i>0){unsigned n=(unsigned)i-1;if(n<ntrack)d=track_desc(&trk[0],track_ids[n]);
        else if(n>=ntrack+FG_NUM&&n<ntrack+FG_NUM+sizeof global_ids)d=&GP[global_ids[n-ntrack-FG_NUM]];
    }
    if(!d)return snprintf(out,(size_t)capacity,"%d",native_value(i));
    char value[192]={0};const char *unit="";param_format(d,native_value(i),value,&unit);
    return snprintf(out,(size_t)capacity,"%s%s%s",value,unit[0]?" ":"",unit);
}
#define NATIVE_MIN_STATE_PARAMS (extra_base())
#include "fm1_facade.h"
