// SPDX-License-Identifier: AGPL-3.0-or-later
#define main previous_main
#include "native_instrument.cpp"
#undef main
int main(int argc,char**argv){if(argc<2)return 2;try{re::Core c("",0,argv[1]);ready(c);c.set(1,1/19.f);ready(c);c.tempo(120,0,true);c.midi(0x90,60,112);float l[512]{},r[512]{};for(int i=0;i<8;i++)c.render(l,r,512);c.set(1,18/19.f);for(int i=0;i<2000&&text(c,26)!="READY";i++)std::this_thread::sleep_for(std::chrono::milliseconds(5));if(text(c,26)!="READY")throw std::runtime_error("Load");c.tempo(120,.5,false);double peak=0;for(int b=0;b<80;b++){c.render(l,r,512);for(int n=0;n<512;n++)peak=std::max(peak,double(std::max(std::abs(l[n]),std::abs(r[n]))));}if(peak>1e-7)throw std::runtime_error("Stop during graph switch left audio: "+std::to_string(peak));std::cout<<"STOP_SWAP_PASS no residual notes after switching while held\n";}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}return 0;}
