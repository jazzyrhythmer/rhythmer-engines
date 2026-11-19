// SPDX-License-Identifier: GPL-3.0-only
// Native boundary by jazzy rhythmer. Original voice and factory data in original/.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" {
#include "engine.h"
}
#include "params.h"
#include "felucca_tables.h"
#include "helpers.h"
#define CTL 32
#include "core.h"

namespace {
constexpr int globals=13, lane_params=19;
struct Native {
    DrumXCore core{};
    int values[NPARAMS]{};
    DrumXCore::dx_voice_t voices[8]{};
    int velocity[8]{}, transpose[8]{};
    int16_t cache[64]{};
    unsigned position=32;
    int index(const char *key) const {
        for(int i=0;i<NPARAMS;++i)if(!std::strcmp(key,PARAMS[i].key))return i;
        return -1;
    }
    void apply() {
        core.dx_tone=uint8_t(values[8]|(values[9]<<4));core.dx_hands=uint8_t(values[10]);
        for(unsigned l=0;l<8;++l) {
            const auto *p=values+globals+l*lane_params;
            core.dx_kit[l].mode=uint8_t(p[0]|(p[1]<<2)|(p[2]<<4)|(p[3]<<5)|(p[4]<<7));
            core.dx_lay[l]=uint8_t(p[5]|(p[6]<<4));
            for(unsigned j=0;j<5;++j){core.dx_kit[l].a[j]=uint8_t(p[9+j]);core.dx_kit[l].b[j]=uint8_t(p[14+j]);}
        }
    }
    void kit(int k) {
        // Original DRUM_PRESETS global controls; firmware-only bus drive/FX excluded.
        static const int settings[9][7]={{64,64,70,64,64,0,0},{60,64,76,64,64,0,0},
            {58,60,56,76,60,0,0},{70,64,50,54,76,0,0},{72,66,88,70,74,20,0},
            {56,64,58,62,64,0,0},{60,60,52,70,60,0,0},{66,64,80,62,66,10,0},
            {62,66,72,62,70,0,0}};
        values[0]=k;for(int j=0;j<7;++j)values[1+j]=settings[k][j];
        values[8]=values[9]=values[10]=0;
        for(unsigned l=0;l<8;++l) {
            auto *p=values+globals+l*lane_params;const auto& a=core.DX_KITS[k].lane[l];
            p[0]=a.mode&3;p[1]=(a.mode>>2)&3;p[2]=(a.mode>>4)&1;p[3]=(a.mode>>5)&3;p[4]=a.mode>>7;
            p[5]=core.DX_KITS[k].lay[l]&15;p[6]=core.DX_KITS[k].lay[l]>>4;
            p[7]=100;p[8]=64;
            for(unsigned j=0;j<5;++j){p[9+j]=a.a[j];p[14+j]=a.b[j];}
        }
        apply();
    }
    Native(){values[11]=100;kit(0);}
    void block() {
        int64_t mix[64]{};int32_t mono[32];
        for(unsigned l=0;l<8;++l) {
            auto *p=values+globals+l*lane_params;
            core.dx_run(&core.dx_kit[l],&voices[l],values[1],(values[2]-64)*3+transpose[l]*16,
                values[4]-64,values[3]-64,values[5]-64,values[7],values[6],mono,32);
            for(unsigned f=0;f<32;++f) {
                int64_t s=int64_t(mono[f])*velocity[l]*p[7]*values[11]/(127*127*127);
                mix[2*f]+=s*(127-p[8])/127;mix[2*f+1]+=s*p[8]/127;
            }
        }
        for(unsigned i=0;i<64;++i)cache[i]=int16_t(std::clamp<int64_t>(mix[i],-32768,32767));
        position=0;
    }
};
void *create(const char*) {return new Native;}
void destroy(void *i){delete static_cast<Native*>(i);}
void midi(void *i,const uint8_t *m,int size) {
    if(size<3)return;auto& n=*static_cast<Native*>(i);
    const unsigned status=m[0]&0xf0;
    if(status==0xb0&&(m[1]==120||m[1]==123)) {
        std::memset(n.voices,0,sizeof n.voices);std::memset(n.cache,0,sizeof n.cache);return;
    }
    if(status!=0x90||!m[2])return;
    unsigned lane;int tune=0;
    if(!n.values[12]){if(m[1]<36||m[1]>43)return;lane=m[1]-36;}
    else {
        // Original JIANT GM lane/tuning table, notes 35..81.
        static const int8_t gm[47][2]={{0,-2},{0,0},{6,0},{1,0},{2,0},{1,2},{5,-7},{3,0},{5,-4},{3,-2},{5,0},
            {4,0},{5,3},{5,5},{7,0},{5,8},{7,-3},{7,2},{7,5},{3,5},{7,4},{7,0},{7,1},{6,-12},{7,-2},{5,10},
            {5,7},{5,5},{5,5},{5,0},{5,10},{5,7},{7,7},{7,3},{3,3},{3,7},{6,7},{6,5},{3,-4},{3,-6},
            {6,0},{6,-4},{6,-7},{5,12},{5,8},{7,12},{7,12}};
        if(m[1]<35||m[1]>81)return;lane=unsigned(gm[m[1]-35][0]);tune=gm[m[1]-35][1];
    }
    n.velocity[lane]=m[2];n.transpose[lane]=tune;
    if(lane==3)n.core.dx_choke(&n.voices[4]);
    n.core.dx_trigger(&n.voices[lane]);
}
void set(void *i,const char *key,const char *value) {
    auto& n=*static_cast<Native*>(i);
    if(!std::strcmp(key,"state")) {
        if(std::strlen(value)!=5+NPARAMS*8||std::strncmp(value,"DXS1:",5))return;
        int restored[NPARAMS];
        for(int j=0;j<NPARAMS;++j) {
            char s[9];std::memcpy(s,value+5+j*8,8);s[8]=0;char *end;
            unsigned long v=std::strtoul(s,&end,16);if(*end)return;
            int hi=PARAMS[j].nopts?PARAMS[j].nopts-1:int(PARAMS[j].max);
            if(v>unsigned(hi))return;restored[j]=int(v);
        }
        std::memcpy(n.values,restored,sizeof restored);n.apply();return;
    }
    int j=n.index(key);if(j<0)return;int hi=PARAMS[j].nopts?PARAMS[j].nopts-1:int(PARAMS[j].max);
    n.values[j]=std::clamp(std::atoi(value),0,hi);
    if(!j)n.kit(n.values[j]);else n.apply();
}
int get(void *i,const char *key,char *out,int capacity) {
    auto& n=*static_cast<Native*>(i);
    if(!std::strcmp(key,"state")) {
        const int size=5+NPARAMS*8;if(capacity<=size)return 0;
        std::memcpy(out,"DXS1:",5);
        for(int j=0;j<NPARAMS;++j)std::snprintf(out+5+j*8,9,"%08x",unsigned(n.values[j]));
        return size;
    }
    int j=n.index(key);return j<0?0:std::snprintf(out,capacity,"%d",n.values[j]);
}
void render(void *i,int16_t *out,int frames) {
    auto& n=*static_cast<Native*>(i);
    for(int f=0;f<frames;++f) {
        if(n.position==32)n.block();out[2*f]=n.cache[2*n.position];out[2*f+1]=n.cache[2*n.position+1];++n.position;
    }
}
const mpc_engine_t engine={create,destroy,midi,set,get,render,nullptr,nullptr};
}
extern "C" const mpc_engine_t *mpc_engine(){return &engine;}
