// SPDX-License-Identifier: AGPL-3.0-or-later
// Fixed, bundled source cores only. No third-party scanning or firmware hooks.
#pragma once
#include "mpc_vst2_abi.h"
#include "third_party/json.hpp"
#include "component_library.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
namespace re {
using Json=nlohmann::json;
inline constexpr const char* coreIds[]={"cs1","drumx","elixir","fm106","fmini","guru600","hortator","kwasub","kwasub-kit","mono2k","obervave","prophet","s1vave","aphex","braids","dexed","hera","plaits","sophie","6w6","8w8","9w9","cw78","trmpc","hortator","frodhes","tenebrae","amino","fizzik","weird-dreams","krautdrums"};
class Core {
 void* library_=nullptr;
 AEffect* e_=nullptr;
 inline static thread_local VstTimeInfo time_{};
 static VstIntPtr host(AEffect*,VstInt32 op,VstInt32,VstIntPtr,void*,float){
  if(op==audioMasterVersion)return 2400;
  if(op==audioMasterGetSampleRate)return 44100;
  if(op==audioMasterGetBlockSize)return 128;
  if(op==audioMasterGetTime)return reinterpret_cast<VstIntPtr>(&time_);
  return 0;
 }
 public:
 Core(const std::string& folder,int source,const std::string& testPath={}){
  if(source<0||source>=int(sizeof(coreIds)/sizeof(*coreIds)))throw std::runtime_error("Invalid bundled engine");
  auto path=testPath.empty()?folder+"/"+coreIds[source]+"/"+componentFile:testPath;
  library_=openLibrary(path);
  if(!library_)throw std::runtime_error(std::string("Engine load: ")+libraryError());
  auto fn=reinterpret_cast<AEffect*(*)(audioMasterCallback)>(librarySymbol(library_,"VSTPluginMain"));
  if(!fn){closeLibrary(library_);library_=nullptr;throw std::runtime_error("Missing engine entry point");}
  e_=fn(host);
  if(!e_||e_->magic!=kEffectMagic||!e_->processReplacing||e_->numOutputs!=2){
   if(e_)e_->dispatcher(e_,effClose,0,0,nullptr,0);closeLibrary(library_);e_=nullptr;library_=nullptr;
   throw std::runtime_error("Invalid source component");
  }
  time_.sampleRate=44100;time_.tempo=120;time_.timeSigNumerator=4;time_.timeSigDenominator=4;
  time_.flags=kVstTempoValid|kVstPpqPosValid|kVstTimeSigValid;
  dispatch(effOpen);dispatch(effSetSampleRate,0,0,nullptr,44100);dispatch(effSetBlockSize,0,128);dispatch(effMainsChanged,0,1);
  dispatch(effSetProgram,0,0);float l[128]{},r[128]{};render(l,r,128);
 }
 ~Core(){if(e_){dispatch(effMainsChanged,0,0);dispatch(effClose);}if(library_)closeLibrary(library_);}
 Core(const Core&)=delete;Core&operator=(const Core&)=delete;
 VstIntPtr dispatch(int op,int index=0,VstIntPtr value=0,void*ptr=nullptr,float opt=0){return e_->dispatcher(e_,op,index,value,ptr,opt);}
 int count()const{return e_->numParams;}
 int latency()const{return e_->initialDelay;}
 void set(int i,float v){if(i>=0&&i<count()&&std::isfinite(v))e_->setParameter(e_,i,std::clamp(v,0.f,1.f));}
 float get(int i){return i>=0&&i<count()?e_->getParameter(e_,i):0;}
 void native(const std::string& key,const std::string& value){
  const char* pair[]={key.c_str(),value.c_str()};
  if(!dispatch(0x47720001,0,0,const_cast<char**>(pair)))throw std::runtime_error("Missing source selector: "+key);
 }
 void apply(const Json& source,const Json& sound){
  if(sound.contains("native_setup"))for(const auto& p:sound["native_setup"])native(p[0].get<std::string>(),p[1].is_string()?p[1].get<std::string>():p[1].dump());
  if(sound.contains("native_key")){const auto& v=sound["native_value"];native(sound["native_key"],v.is_string()?v.get<std::string>():v.dump());}
  else if(sound.contains("program"))dispatch(effSetProgram,0,sound["program"].get<int>());
  else if(sound.contains("values"))for(auto it=sound["values"].begin();it!=sound["values"].end();++it)set(std::stoi(it.key()),it.value().get<float>());
  if(source.contains("native_initial"))for(auto it=source["native_initial"].begin();it!=source["native_initial"].end();++it)native(it.key(),it.value().is_string()?it.value().get<std::string>():it.value().dump());
  if(source.contains("initial_values"))for(auto it=source["initial_values"].begin();it!=source["initial_values"].end();++it)set(std::stoi(it.key()),it.value().get<float>());
  if(source.value("drums",false))for(const auto& p:source["parameters"]){auto key=p["key"].get<std::string>();if(key=="pad"||key=="focused_pad"||key=="ui_current_pad")set(p["index"],std::clamp((sound.value("note",60)-36.f)/std::max(1.f,p["maximum"].get<float>()-p["minimum"].get<float>()),0.f,1.f));}
  float l[128]{},r[128]{};for(int i=0;i<(source.value("id",std::string())=="rhythmer-fm1-frodhes-frodhes"?128:3);i++)render(l,r,128);
 }
 void reference(const Json& source,const Json& reference){
  if(source.value("id",std::string())=="rhythmer-fm1-frodhes-frodhes")return; // Original MPC factory programs own their 8192-frame echo/settle window.
  if(!reference.is_object()||!reference.contains("values"))return;
  // Restore the accepted original program's editable controls explicitly.
  // Constructors can have different init defaults than the released module.
  // Avoid rewriting identical values: original engines may invalidate their
  // factory identity, or keep native resolution beyond the normalized getter.
  for(const auto&p:source["parameters"]){int i=p["index"];if(i<count()&&i<int(reference["values"].size())&&!p.value("is_program_selector",false)&&!p.value("hidden_in_sample",false)&&std::fabs(get(i)-reference["values"][i].get<float>())>1e-6f)set(i,reference["values"][i]);}
  float l[128]{},r[128]{};render(l,r,128);
 }
 void midi(uint8_t s,uint8_t a,uint8_t b){
  VstMidiEvent event{};event.type=kVstMidiType;event.byteSize=sizeof(event);event.midiData[0]=s;event.midiData[1]=a;event.midiData[2]=b;
  VstEvents events{};events.numEvents=1;events.events[0]=reinterpret_cast<VstEvent*>(&event);dispatch(effProcessEvents,0,0,&events);
 }
 void tempo(double bpm,double ppq,bool playing){time_.tempo=bpm;time_.ppqPos=ppq;time_.flags=kVstTempoValid|kVstPpqPosValid|kVstTimeSigValid|(playing?kVstTransportPlaying:0);}
 void render(float*l,float*r,int frames){float* out[]={l,r};e_->processReplacing(e_,nullptr,out,frames);}
 std::string name(int i){char text[1024]{};dispatch(effGetParamName,i,0,text);return text;}
 std::vector<uint8_t> state(){void* data=nullptr;auto n=dispatch(effGetChunk,0,0,&data);if(n<=0||n>32*1024*1024||!data)return {};auto*p=static_cast<uint8_t*>(data);return {p,p+n};}
 void restore(const std::vector<uint8_t>& data){if(!data.empty())dispatch(effSetChunk,0,data.size(),const_cast<uint8_t*>(data.data()));}
};
inline int actualSource(const Json& catalog,int source,int sound){
 const auto&s=catalog["sources"][source]["sounds"][sound];
 if(s.contains("source_engine"))for(const auto& origin:catalog["sources"])if(origin["folder"]==s["source_engine"])return origin["source_index"];
 return source;
}
}
