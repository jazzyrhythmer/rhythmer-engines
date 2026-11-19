// SPDX-License-Identifier: AGPL-3.0-or-later
#include "component_host.h"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){if(argc<3)return 2;re::Json cat;std::ifstream(argv[1])>>cat;for(int p=32;p<36;p++){auto&s=cat["sources"][13];re::Core c(argv[2],13);c.apply(s,s["sounds"][p]);c.reference(s,s["program_states"][p]);c.midi(0x90,60,110);double peak=0,sum=0;long frames=0;float l[512],r[512];for(int b=0;b<2584;b++){c.render(l,r,512);for(int n=0;n<512;n++){if(!std::isfinite(l[n])||!std::isfinite(r[n]))return 1;peak=std::max(peak,double(std::max(std::fabs(l[n]),std::fabs(r[n]))));sum+=l[n]*l[n]+r[n]*r[n];}frames+=512;}if(peak<1e-5)return 1;std::cout<<re::Json{{"name",s["sounds"][p]["name"]},{"seconds",frames/44100.},{"peak",peak},{"rms",std::sqrt(sum/(2*frames))},{"result","PASS"}}.dump()<<std::endl;}return 0;}
