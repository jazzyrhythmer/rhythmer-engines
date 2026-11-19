// SPDX-License-Identifier: GPL-3.0-only
#include "rhythmer/engine/break_manipulator.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <stdexcept>
using rhythmer::BreakManipulator;using rhythmer::SampleData;
static void require(bool ok,const char* s){if(!ok)throw std::runtime_error(s);}
static std::vector<float> render(BreakManipulator&b,int frames,float rate){std::vector<float>l(frames),r(frames);b.render(l.data(),r.data(),frames,rate);for(float x:l)require(std::isfinite(x),"Nonfinite output");return l;}
static double energy(const std::vector<float>&v){double e=0;for(float x:v)e+=x*x;return e;}
static void init(BreakManipulator&b,SampleData&a,SampleData&c,int bars){b.configure(bars,0);b.sources(&a,&c);b.tempo(1200);b.transport(true,0);b.note(36,100);}
int main(){try{SampleData a,c;a.frames=c.frames=88200;a.left.resize(a.frames);c.left.resize(c.frames);for(unsigned i=0;i<a.frames;i++){a.left[i]=.23f*std::sin(i*.031)+.11f*std::sin(i*.079);c.left[i]=.13f*std::sin(i*.057);}
 for(auto*s:{&a,&c}){s->right=s->left;s->break_cache->mono=s->left;s->break_cache->ready=true;}
 unsigned checks=0;for(float rate:{44100.f,48000.f,96000.f})for(int bars=1;bars<=4;bars++)for(int fx=0;fx<16;fx++){
  BreakManipulator dry;init(dry,a,c,bars);render(dry,2048,rate);auto plain=render(dry,int(rate),rate);
  std::vector<float>reference;for(int velocity:{1,32,64,127}){BreakManipulator b;init(b,a,c,bars);render(b,2048,rate);b.note(fx,velocity);auto v=render(b,int(rate),rate);require(energy(v)>1e-9,"Silent PERF command");if(reference.empty()){if(v==plain)throw std::runtime_error("PERF unchanged fx="+std::to_string(fx)+" bars="+std::to_string(bars)+" rate="+std::to_string(rate));reference=v;}else require(v==reference,"PERF changed with velocity");b.off(fx);render(b,2048,rate);b.stop();require(energy(render(b,1024,rate))<1e-12,"Stop left audio");checks++;}
 }
 BreakManipulator b;init(b,a,c,2);b.note(2,1);render(b,256,44100);require((b.activity()&12)==12,"Stretch hold missing");for(int i=0;i<16;i++){b.note(36+i,80);b.off(36+i);b.transport(true,i*24);render(b,512,44100);require((b.activity()&12)==12,"Generated chops cleared live PERF");}
 b.note(4,127);b.off(4);render(b,128,44100);require((b.activity()&12)==12,"Overlap failed to return to held Stretch");b.note(2,64);b.off(2);render(b,128,44100);require(!(b.activity()&8),"Repeated down stuck hold");b.off(2);b.note(16,1);require(b.source()==0,"Source A");b.note(17,1);require(b.source()==1,"Source B");b.note(36,100);require(energy(render(b,2048,44100))>1e-9,"Silent source B");b.note(18,1);for(unsigned id=0;id<10;id++)b.param(id,id==3?100:0);render(b,512,44100);b.release();render(b,44100,44100);require(!b.sounding(),"Release stuck");
 BreakManipulator low,high;init(low,a,c,2);init(high,a,c,2);low.note(36,32);high.note(36,127);require(energy(render(high,4096,44100))>energy(render(low,4096,44100))*8,"Chop velocity lost");
 std::cout<<"BREAK_PERF_PASS velocity_cases="<<checks<<" 16 commands, 4 bar lengths, 3 rates, held chops, overlap, retrigger, A/B, return, release, stop, chop velocity\n";
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
