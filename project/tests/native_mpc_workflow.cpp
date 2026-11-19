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
int main(int argc,char**argv){if(argc<3)return 2;try{re::Core c("",0,argv[1]);ready(c);ensure(c.count()==179,"Extended schema count");
 // The same 16 kit sounds must work from legacy drum notes and default MPC notes.
 if(argc<=3)for(int root:{36,60})for(int pad=0;pad<16;pad++){c.set(27,1);settle(c);double e=renderNote(c,root+pad);ensure(e>1e-8,"Kit note silent root="+std::to_string(root)+" pad="+std::to_string(pad+1));}
 c.set(116,48.f/112);settle(c);ensure(renderNote(c,48)>1e-8,"Configurable kit root failed");c.set(116,60.f/112);settle(c);std::cout<<"KIT_MIDI_PASS 32 notes at roots 36/60, selectable root48\n";
 re::Json cat;std::ifstream(argv[2])>>cat;unsigned audited=0;
 for(int engine=argc>3?std::atoi(argv[3]):0;engine<(argc>3?std::atoi(argv[3])+1:20);engine++){c.set(1,engine/19.f);ready(c);c.set(4,0);c.set(5,0);c.set(3,10.f/15);settle(c);int cell=engine==0?10:0;auto old=pageBindings(c,5,8,cell);std::set<int>united;
  for(int bank=0;bank<3;bank++){auto ids=pageBindings(c,113+bank,16+8*bank,cell);for(int id:ids)ensure(!united.count(id),"Parameter appears in two editor sections");united.insert(ids.begin(),ids.end());}
  if(old!=united){std::cerr<<"COVERAGE_DIFF old="<<old.size()<<" new="<<united.size()<<" missing:";for(auto id:old)if(!united.count(id))std::cerr<<id<<",";std::cerr<<" extra:";for(auto id:united)if(!old.count(id))std::cerr<<id<<",";std::cerr<<std::endl;}ensure(old==united,"Missing parameter after splitting EDIT/MOD/FX: "+std::to_string(engine));audited+=united.size();
  // Edit each populated section and round-trip the actual sound graph.
  for(int bank=0;bank<3;bank++){auto before=extra(c);for(int slot=0;slot<8;slot++){int p=89+bank*8+slot;uint64_t token=before["bindings"][16+bank*8+slot];if(!token)continue;int index=int(token&0xffff)-4;if(index<0)continue;if(engine==12)std::cerr<<"EDIT_TARGET bank="<<bank<<" native_index="<<(int(token&0xffff)-4)<<" name="<<c.name(p)<<std::endl;float value=c.get(p);c.set(p,value<.5f?.7f:.3f);settle(c);auto edited=extra(c);auto state=c.state();c.set(p,.9f);settle(c);c.restore(state);ready(c);settle(c);if(extra(c)["graph"]!=edited["graph"]){std::cerr<<"RESTORE_DIFF engine="<<engine<<" bank="<<bank<<" param="<<p<<" "<<re::Json::diff(edited["graph"],extra(c)["graph"]).dump()<<std::endl;throw std::runtime_error("Section edit state restore failed");}break;}}
  std::cout<<"SECTIONS_PASS "<<engine<<" controls="<<united.size()<<" full_coverage,no_duplicates,source_pad,save_restore\n";
 }
 // No MIDI/audio means no free-running visual loop. Pitch/velocity alter pose.
 c.set(1,5/19.f);ready(c);c.set(27,1);settle(c);float l[128]{},r[128]{};for(int i=0;i<500;i++)c.render(l,r,128);float idle=c.get(25);for(int i=0;i<500;i++)c.render(l,r,128);ensure(std::abs(c.get(25)-idle)<.001f,"Idle animation keeps looping");
 c.midi(0x90,48,30);for(int i=0;i<80;i++)c.render(l,r,128);float low=c.get(25);c.set(27,1);settle(c);c.midi(0x90,84,120);for(int i=0;i<80;i++)c.render(l,r,128);float high=c.get(25);ensure(std::abs(high-low)>.03f,"Animation ignores pitch/velocity");c.midi(0x80,84,0);c.set(27,1);settle(c);for(int i=0;i<1000;i++)c.render(l,r,128);ensure(c.get(25)<.01f,"Animation does not settle after release");
 std::cout<<"MOTION_INPUT_PASS idle="<<idle<<" low="<<low<<" high="<<high<<" release="<<c.get(25)<<"\n";
 std::cout<<"MPC_WORKFLOW_PASS engines="<<(argc>3?1:20)<<" sections=3 controls="<<audited<<" schema=179\n";
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}return 0;}
