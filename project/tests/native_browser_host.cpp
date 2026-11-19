// SPDX-License-Identifier: AGPL-3.0-or-later
// Verify UI choice labels and visibility at the host notification boundary.
#define main old_host_main
#include "native_mpc_host.cpp"
#undef main
int main(int argc,char**argv){try{if(argc<2)return 2;void*lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);require(lib,"Load instrument");auto fn=reinterpret_cast<AEffect*(*)(audioMasterCallback)>(dlsym(lib,"VSTPluginMain"));AEffect*e=fn(host);e->dispatcher(e,effOpen,0,0,nullptr,0);e->dispatcher(e,effSetSampleRate,0,0,nullptr,44100);e->dispatcher(e,effMainsChanged,0,1,nullptr,0);ready(e);
 auto wait=[&](auto predicate,const char*message){for(int i=0;i<2000;i++){if(predicate())return;std::this_thread::sleep_for(std::chrono::milliseconds(5));}throw std::runtime_error(message);};
 e->setParameter(e,117,1);wait([&]{return cache[177]>.5f&&updates[142]>0&&txt(e,176)=="ENGINES";},"Host browser did not become visible with labels");require(txt(e,142)=="Rhythmer Kit","First engine row");
 e->setParameter(e,128,1);wait([&]{return cache[177]<.5f&&e->dispatcher(e,effGetProgram,0,0,nullptr,0)==2;},"Row selection not reflected in MPC preset/visibility");ready(e);
 int before=updates[142];e->setParameter(e,118,1);wait([&]{return cache[177]>.5f&&updates[142]>before&&txt(e,142)!="";},"Preset row labels not published to MPC host");require(txt(e,176)=="Elixir / SOUNDS","Preset browser engine title");
 e->setParameter(e,125,1);wait([&]{return cache[177]<.5f;},"Close not published");require(audioCallbacks==0,"Audio-thread UI callback");
 std::cout<<"BROWSER_HOST_PASS visibility,label_refresh,row_selection,native_engine_sync,close,off_audio schema="<<e->numParams<<"\n";e->dispatcher(e,effClose,0,0,nullptr,0);dlclose(lib);
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}return 0;}
