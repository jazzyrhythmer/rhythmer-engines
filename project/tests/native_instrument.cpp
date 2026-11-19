// SPDX-License-Identifier: AGPL-3.0-or-later
#include "component_host.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
std::string text(re::Core& c,int index){char s[1024]{};c.dispatch(effGetParamDisplay,index,0,s);return s;}
void ready(re::Core&c){for(int i=0;i<2000;i++){auto s=text(c,26);if(s=="READY")return;if(s!="LOADING")throw std::runtime_error(s);std::this_thread::sleep_for(std::chrono::milliseconds(5));}throw std::runtime_error("Timed out loading engine");}
re::Json extra(re::Core&c){auto bytes=c.state();size_t pos=8;auto u32=[&](){if(pos+4>bytes.size())throw std::runtime_error("Bad state");uint32_t x=bytes[pos]|(bytes[pos+1]<<8)|(bytes[pos+2]<<16)|(bytes[pos+3]<<24);pos+=4;return x;};auto n=u32();for(unsigned i=0;i<n;i++){unsigned len=bytes.at(pos++);pos+=len+8;}auto len=u32();return re::Json::parse(bytes.data()+pos,bytes.data()+pos+len);}
double play(re::Core&c,int note=60,bool generated=false){c.tempo(120,0,true);if(!generated)c.midi(0x90,note,110);float l[512],r[512];double sum=0;unsigned frames=0;for(int block=0;block<(generated?600:200);block++){int n=std::array<int,5>{32,64,128,256,512}[block%5];std::fill(l,l+n,0);std::fill(r,r+n,0);c.tempo(120,frames/22050.,true);c.render(l,r,n);for(int i=0;i<n;i++){if(!std::isfinite(l[i])||!std::isfinite(r[i]))throw std::runtime_error("Non-finite output");sum+=l[i]*l[i]+r[i]*r[i];}frames+=n;}if(!generated)c.midi(0x80,note,0);return std::sqrt(sum/(2*frames));}
int main(int argc,char**argv){if(argc<3)return 2;re::Json catalog;std::ifstream(argv[2])>>catalog;
 try{re::Core instrument("",0,argv[1]);ready(instrument);std::cout<<"parameters "<<instrument.count()<<std::endl;if(instrument.count()<89)throw std::runtime_error("Parameter count");if(extra(instrument)["graph"]["entry"]!=0||text(instrument,22)!="K-WASUB / 909")throw std::runtime_error("Default kit");
 for(int engine=0;engine<20;engine++){
  instrument.set(1,engine/19.f);ready(instrument);int note=engine==0?36:60;
  auto rms=play(instrument,note);if(rms<1e-7)throw std::runtime_error("Instrument output unexpectedly silent");auto data=extra(instrument);if(data["graph"]["entry"]!=engine)throw std::runtime_error("Wrong engine after switch");
  auto before=data["graph"];instrument.set(6,.371f);play(instrument,note);auto changed=extra(instrument);auto state=instrument.state();instrument.set(6,.817f);play(instrument,note);instrument.restore(state);ready(instrument);auto after=extra(instrument);
  if(changed["graph"]!=after["graph"])throw std::runtime_error("Edited sound state lost on restore: "+catalog["entries"][engine]["name"].get<std::string>());
  instrument.set(27,1);play(instrument,note,true);float l[256]{},r[256]{};instrument.render(l,r,256);double stopped=0;for(int i=0;i<256;i++)stopped+=l[i]*l[i]+r[i]*r[i];if(stopped>1e-12)throw std::runtime_error("STOP SOUND left audio active");
  if(data.contains("schedule")||data.contains("pattern"))throw std::runtime_error("Generator state exposed");
  std::cout<<re::Json{{"engine",catalog["entries"][engine]["name"]},{"rms",rms},{"state","PASS"},{"panic","PASS"}}.dump()<<std::endl;

 }
 for(int kit=0;kit<int(catalog["kits"].size());kit++){
  instrument.set(1,0);instrument.set(22,kit/float(catalog["kits"].size()-1));ready(instrument);auto data=extra(instrument);if(data["graph"]["cells"].size()!=16)throw std::runtime_error("Kit pad count");
  for(int p=0;p<16;p++){instrument.set(3,p/15.f);for(int wait=0;wait<200&&instrument.name(8).rfind("P"+std::to_string(p+1)+" ",0)!=0;wait++)std::this_thread::sleep_for(std::chrono::milliseconds(5));if(instrument.name(8).rfind("P"+std::to_string(p+1)+" ",0)!=0)throw std::runtime_error("Kit pad label mismatch kit="+std::to_string(kit)+" pad="+std::to_string(p+1)+" name="+instrument.name(8));}
  std::cout<<"KIT "<<catalog["kits"][kit]["name"]<<" PASS\n"<<std::flush;
 }
 }catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}
 std::cout<<"INSTRUMENT PASS\n";return 0;
}
