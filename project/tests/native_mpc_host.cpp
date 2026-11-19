// SPDX-License-Identifier: AGPL-3.0-or-later
// Exercise the actual VST2 host boundary, not only internal Core values.
#include "component_host.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
using namespace re;
static std::array<std::atomic<int>,179> updates{};static std::array<std::atomic<float>,179> cache{};static std::atomic<int> displays{0},audioCallbacks{0};static thread_local bool audio=false;static VstTimeInfo clockInfo{};
static VstIntPtr host(AEffect*,VstInt32 op,VstInt32 index,VstIntPtr,void*,float value){if(op==audioMasterVersion)return 2400;if(op==audioMasterGetSampleRate)return 44100;if(op==audioMasterGetBlockSize)return 128;if(op==audioMasterGetTime)return reinterpret_cast<VstIntPtr>(&clockInfo);if(op==audioMasterAutomate&&index>=0&&index<179){cache[index]=value;updates[index]++;if(audio)audioCallbacks++;}if(op==audioMasterUpdateDisplay){displays++;if(audio)audioCallbacks++;}return 0;}
static std::string txt(AEffect*e,int i){char s[64]{};e->dispatcher(e,effGetParamDisplay,i,0,s,0);return s;}
static void ready(AEffect*e){for(int n=0;n<4000;n++){if(txt(e,26)=="READY"){std::this_thread::sleep_for(std::chrono::milliseconds(60));return;}std::this_thread::sleep_for(std::chrono::milliseconds(5));}throw std::runtime_error("Engine READY timeout: "+txt(e,26));}
static void require(bool yes,const std::string&why){if(!yes)throw std::runtime_error(why);}
int main(int argc,char**argv){try{if(argc<3)return 2;Json cat;std::ifstream(argv[2])>>cat;void*lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);require(lib,dlerror()?"dlopen failed":"dlopen");auto fn=reinterpret_cast<AEffect*(*)(audioMasterCallback)>(dlsym(lib,"VSTPluginMain"));require(fn,"VST entry");AEffect*e=fn(host);require(e,"Create");e->dispatcher(e,effOpen,0,0,nullptr,0);e->dispatcher(e,effSetSampleRate,0,0,nullptr,44100);e->dispatcher(e,effSetBlockSize,0,128,nullptr,0);e->dispatcher(e,effMainsChanged,0,1,nullptr,0);ready(e);
 require(e->numPrograms==20,"Native MPC preset count must be 20 engines");require(e->numParams==179,"Stable parameter count");
 for(int engine=0;engine<20;engine++){
  char name[64]{};require(e->dispatcher(e,effGetProgramNameIndexed,engine,0,name,0)==1,"Indexed program names");require(std::string(name)==cat["entries"][engine]["name"].get<std::string>(),"Native engine name mismatch");
  e->dispatcher(e,effSetProgram,0,engine,nullptr,0);ready(e);require(e->dispatcher(e,effGetProgram,0,0,nullptr,0)==engine,"Native selected program");require(std::lround(e->getParameter(e,1)*19)==engine,"Engine parameter did not follow native preset");
  require(txt(e,1)==name,"Engine display did not follow native preset");
  int count=engine==0?int(cat["kits"].size()):int(cat["sources"][int(cat["entries"][engine]["source_index"])]["sounds"].size());int p=engine==0?22:28+engine-1;
  e->setParameter(e,p,1);ready(e);std::string expected=(engine==0?cat["kits"][count-1]:cat["sources"][int(cat["entries"][engine]["source_index"])]["sounds"][count-1])["name"];
  require(txt(e,2)==expected.substr(0,32),"Sound/kit selector display mismatch "+std::string(name));require(e->dispatcher(e,effGetProgram,0,0,nullptr,0)==engine,"Sound change incorrectly changed native engine");
  std::cout<<"MPC_ENGINE_PASS "<<engine<<" "<<name<<" sounds="<<count<<" native_preset_and_sound_linked\n";
 }
 // A native engine change must also follow an Engine parameter (e.g. state restore).
 e->setParameter(e,1,0);ready(e);require(e->dispatcher(e,effGetProgram,0,0,nullptr,0)==0,"Parameter to native preset reverse sync");
 // Every wheel step is one real detail page; out-of-range pages wrap back
 // to a valid visible page, and the host receives the updated page/group name.
 e->setParameter(e,5,1.f/511.f);ready(e);char pageName[64]{};e->dispatcher(e,effGetParamName,5,0,pageName,0);require(std::string(pageName).rfind("2/",0)==0,"Detail page count/name stale");
 e->setParameter(e,5,1);ready(e);require(e->getParameter(e,5)<1,"Detail page out of range was not normalised");e->setParameter(e,5,0);ready(e);
 // Live's knob and the duplicate EDIT control must stay in the same displayed state.
 e->setParameter(e,6,.37f);float l[128]{},r[128]{};float*out[]={l,r};for(int i=0;i<25;i++){audio=true;e->processReplacing(e,nullptr,out,128);audio=false;std::this_thread::sleep_for(std::chrono::milliseconds(5));}
 ready(e);require(std::abs(cache[6].load()-.37f)<.01f,"Host cached main knob value stale");require(std::abs(cache[14].load()-.37f)<.01f,"Host cached EDIT duplicate stale");require(txt(e,6)==txt(e,14),"Main and EDIT text mismatch");
 // Exercise note-on, audio-driven motion and bounded off-audio notifications.
 VstMidiEvent midi{};midi.type=kVstMidiType;midi.byteSize=sizeof(midi);midi.midiData[0]=char(0x90);midi.midiData[1]=36;midi.midiData[2]=110;VstEvents events{};events.numEvents=1;events.events[0]=reinterpret_cast<VstEvent*>(&midi);e->dispatcher(e,effProcessEvents,0,0,&events,0);
 int before=updates[25];for(int i=0;i<800;i++){audio=true;e->processReplacing(e,nullptr,out,128);audio=false;std::this_thread::sleep_for(std::chrono::milliseconds(1));}
 require(updates[25]>before+1,"MPC host receives no motion frame updates");require(updates[25]-before<100,"Animation host callback flooding");require(audioCallbacks==0,"Host UI callback issued on audio thread");
 // State comes back with both native engine identity and source sound selection.
 e->dispatcher(e,effSetProgram,0,3,nullptr,0);ready(e);e->setParameter(e,30,1);ready(e);void*chunk=nullptr;auto len=e->dispatcher(e,effGetChunk,0,0,&chunk,0);std::vector<unsigned char>saved((unsigned char*)chunk,(unsigned char*)chunk+len);std::string sound=txt(e,2);e->dispatcher(e,effSetProgram,0,18,nullptr,0);ready(e);e->dispatcher(e,effSetChunk,0,len,saved.data(),0);ready(e);require(e->dispatcher(e,effGetProgram,0,0,nullptr,0)==3&&txt(e,2)==sound,"Native preset / sound state restore");
 std::cout<<"MPC_HOST_PASS engines=20 count=179 native_names,indexed_selection,reverse_sync,sound_banks,main_edit_cache,motion_notifications,off_audio,state_restore updates="<<updates[25]<<"\n";
 e->dispatcher(e,effClose,0,0,nullptr,0);dlclose(lib);
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}return 0;}
