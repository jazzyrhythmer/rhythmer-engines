// SPDX-License-Identifier: AGPL-3.0-or-later
#define main old_instrument_main
#include "native_instrument.cpp"
#undef main
#include <set>
static void ensure(bool ok,const std::string&msg){if(!ok)throw std::runtime_error(msg);}
static void settle(re::Core&c){float l[128]{},r[128]{};for(int i=0;i<5;i++){c.render(l,r,128);std::this_thread::sleep_for(std::chrono::milliseconds(5));}}
static int pages(re::Core&c,int p){auto t=text(c,p);auto slash=t.find('/');ensure(slash!=std::string::npos,"Page display missing total");return std::stoi(t.substr(slash+1));}
static std::set<int>pageBindings(re::Core&c,int pageParam,int slot,int cell){std::set<int>ids;extra(c);int n=pages(c,pageParam);for(int page=0;page<n;page++){c.set(pageParam,page/511.f);settle(c);auto e=extra(c);for(int k=slot;k<slot+8;k++){uint64_t t=e["bindings"][k];if(t){int target=(t>>16)&0xffff,index=int(t&0xffff)-4;ensure(target==cell,"Editor bound to different pad");ids.insert(index);}}}c.set(pageParam,0);settle(c);return ids;}
static double renderNote(re::Core&c,int note,int vel=110,int blocks=120){c.midi(0x90,note,vel);float l[128]{},r[128]{};double energy=0;for(int b=0;b<blocks;b++){c.render(l,r,128);for(int i=0;i<128;i++){ensure(std::isfinite(l[i])&&std::isfinite(r[i]),"Nonfinite sample");energy+=l[i]*l[i]+r[i]*r[i];}}c.midi(0x80,note,0);return energy;}
int main(int argc,char**argv){try{if(argc<2)return 2;re::Core c("",0,argv[1]);ready(c);c.set(3,10.f/15);c.set(4,0);unsigned total=0;
for(int source=0;source<26;source++){c.set(23,source/25.f);ready(c);auto old=pageBindings(c,5,8,10);std::set<int>united;for(int bank=0;bank<3;bank++){auto ids=pageBindings(c,113+bank,16+8*bank,10);for(int id:ids)ensure(!united.count(id),"Duplicate kit section parameter");united.insert(ids.begin(),ids.end());}ensure(old==united,"Missing kit section parameter source="+std::to_string(source));total+=united.size();std::cout<<"KIT_SECTION "<<source<<" controls="<<united.size()<<" P11,full_coverage,no_duplicates\n";}
std::cout<<"KIT_SECTIONS_PASS sources=26 bindings="<<total<<" selected_pad=P11\n";
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}return 0;}
