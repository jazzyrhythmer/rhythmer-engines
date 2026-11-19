// SPDX-License-Identifier: AGPL-3.0-only
#include "LiveArt.h"
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
#include <chrono>
struct Owner{rhyvisual::Context*c=rhyvisual::create();~Owner(){rhyvisual::destroy(c);}};
static std::vector<rhyvisual::Face> read(Owner&c,const rhyvisual::Signal&s){unsigned count=0;auto*p=rhyvisual::frame(c.c,s,count);if(!count||count>384)throw std::runtime_error("Invalid face count");for(unsigned i=0;i<count;i++){for(float x:p[i].xy)if(!std::isfinite(x))throw std::runtime_error("Invalid coordinate");if(p[i].fill<0||p[i].fill>1||p[i].stroke<0||p[i].stroke>1)throw std::runtime_error("Invalid original shading");}return {p,p+count};}
static bool equal(const std::vector<rhyvisual::Face>&a,const std::vector<rhyvisual::Face>&b){return a.size()==b.size()&&std::memcmp(a.data(),b.data(),a.size()*sizeof(a[0]))==0;}
int main(){try{auto start=std::chrono::steady_clock::now();unsigned frames=0;for(int kind=0;kind<22;kind++){Owner a,b,noise;rhyvisual::Signal s;s.kind=kind;s.level=.65f;s.held=1;s.pad=3;s.preset="Original source";std::vector<rhyvisual::Face>first,last;for(unsigned f=0;f<180;f++){s.now=1000+f*16;auto x=read(a,s);auto other=s;other.kind=(kind+7)%22;other.note=84;other.now+=7000;read(noise,other);auto y=read(b,s);if(!equal(x,y))throw std::runtime_error("Editor state leaked between instances");if(!f)first=x;last=x;frames++;}if(equal(first,last))throw std::runtime_error("Input motion stuck");s.now+=16;s.held=0;s.level=0;auto released=read(a,s);if(released.size()!=last.size())throw std::runtime_error("Release changed mesh topology");float travel=0;for(const auto&face:released){float closest=1e9f;for(const auto&old:last){float distance=0;for(int j=0;j<8;j++)distance+=std::abs(face.xy[j]-old.xy[j]);closest=std::min(closest,distance/8);}travel+=closest;}if(travel/std::max(size_t(1),released.size())>24)throw std::runtime_error("Release snapped instead of easing");std::cout<<"ART "<<kind<<" continuous clock, original shading, release, independent editors PASS\n";}std::cout<<"LIVE_ART_PASS styles=22 frames="<<frames<<" seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<"\n";}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
