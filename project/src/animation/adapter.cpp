// SPDX-License-Identifier: AGPL-3.0-only
#include "mini.h"
#include "performance_art.inc"
static char preset_buffer[512]{};
static const char* names[]{"Sophie","Aphex","Elixir","FM-106","FMini-1","CS-1","OberVave","K-WASUB","Hera","Braids","Plaits","Hortator","MONO2K","Prophet","S1Vave","Dexed","Guru-600","Acid Engine","Rhythmer Kit","Break Engine","Frodhes","Tenebrae","Amino","Fizzik"};
extern "C" {
__attribute__((visibility("default"))) char* art_preset(){return preset_buffer;}
__attribute__((visibility("default"))) float* art_faces(){return reinterpret_cast<float*>(polygons);}
__attribute__((visibility("default"))) unsigned art_count(){return polygon_count;}
__attribute__((visibility("default"))) unsigned art_frame(int kind,unsigned now,int note,int held,float level,int pad,float w,float h,int fit){
 polygon_count=0;ArtSignal s;s.now=now;s.part.last_note=note;s.held_keys.fill(-1);if(held)s.held_keys[0]=note;s.meters[0]=level;
 ui::Paint p;performance_art(p,s,names[std::clamp(kind,0,23)],juce::String(preset_buffer),0,0,w,h,pad,1,fit!=0);return polygon_count;
}
}
// A native reference executable exercises the exact same unchanged source.
#ifdef ART_NATIVE_CHECK
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char** argv){
 if(argc<3)return 2;int kind=atoi(argv[1]);const char* name=argv[2];for(unsigned i=0;i<511&&name[i];i++)preset_buffer[i]=name[i];
 printf("[");for(unsigned step=0;step<4;step++){if(step)printf(",");unsigned now=1000+step*150;art_frame(kind,now,60,step<3,step<3?.5f:.2f,10,920,300,1);printf("[");for(unsigned i=0;i<polygon_count;i++){if(i)printf(",");const auto& p=polygons[i];printf("[");for(int j=0;j<8;j++)printf("%s%.7g",j?",":"",p.xy[j]);printf(",%.7g,%.7g]",p.fill,p.stroke);}printf("]");}printf("]\n");return 0;
}
#endif
