// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "rhythmer/model/chord_gen.h"
#include "rhythmer/model/gen_menu_order.h"
#include "rhythmer/model/scales.h"
#include <mutex>
namespace desktop {
// Offline composition only: the DAW owns playback, tempo and note lifetimes.
// No generated events ever enter processBlock or the live MIDI input path.
struct MidiWorkspace {
 using Json=re::Json;
 mutable std::mutex mutex;
 Json document=Json::object();
 static Json defaults(){return {{"mode",0},{"genre",41},{"bars",4},{"root",0},{"scale",1},{"complexity",40},{"energy",65},{"chords",4},{"color",3},{"play",0},{"gate",80},{"octave",4}};}
 static int setting(const Json&s,const char*key,int low,int high){return std::clamp(s.value(key,defaults().value(key,0)),low,high);}
 Json snapshot()const{std::lock_guard<std::mutex>lock(mutex);return document;}
 static bool valid(const Json&d){try{
  if(!d.is_object()||d.value("schema",0)!=1||!d.contains("settings")||!d["settings"].is_object()||!d.contains("notes")||!d["notes"].is_array()||d["notes"].size()>8192)return false;
  int length=d.value("length",0);if(length<384||length>6144)return false;
  for(const auto&n:d["notes"]){if(!n.is_array()||n.size()!=4)return false;for(auto&v:n)if(!v.is_number_integer())return false;if(n[0].get<int>()<0||n[0].get<int>()>=length||n[1].get<int>()<1||n[0].get<int>()+n[1].get<int>()>length||n[2].get<int>()<0||n[2].get<int>()>127||n[3].get<int>()<1||n[3].get<int>()>127)return false;}
  for(auto it=d["settings"].begin();it!=d["settings"].end();++it)if(!it.value().is_number_integer())return false;
  return true;
 }catch(...){return false;}}
 void restore(const Json&d){std::lock_guard<std::mutex>lock(mutex);document=valid(d)?d:Json::object();}
 static bool supportsDrums(int genre){if(genre<2)return true;const auto&g=rhythmer::gen8_genres[rhythmer::gen_fm1_menu[(genre-2)%rhythmer::gen8_genre_count]];for(auto*cell:g.drums)if(cell&&*cell)return true;return false;}
 static std::array<uint8_t,8> kitKeys(const Json&kit){
  std::array<uint8_t,8>keys{{36,37,38,39,40,41,42,43}};const char*roles[]={"kick","snare","clap","closed_hat","open_hat","tom","rim","percussion"};
  if(!kit.contains("routes"))return keys;std::array<bool,8>found{};int cymbal=-1,secondCymbal=-1;
  for(int p=0;p<int(kit["routes"].size())&&p<16;p++){auto role=kit["routes"][p].value("mix_role",std::string());if(role=="cymbal"){if(cymbal<0)cymbal=p;else if(secondCymbal<0)secondCymbal=p;}for(int r=0;r<8;r++)if(!found[r]&&role==roles[r]){keys[r]=36+p;found[r]=true;}}
  if(!kit.value("role_fallbacks",false))return keys;
  if(!found[2]&&found[1])keys[2]=keys[1];if(!found[3]&&cymbal>=0)keys[3]=36+cymbal;if(!found[4]&&cymbal>=0)keys[4]=36+(secondCymbal>=0?secondCymbal:cymbal);if(!found[6]&&found[1])keys[6]=keys[1];return keys;
 }
 Json generate(Json s,bool kit,bool mutate,uint32_t seed,std::array<uint8_t,8>drumKeys={{36,37,38,39,40,41,42,43}}){
  using namespace rhythmer;
  int mode=kit?2:setting(s,"mode",0,1),bars=setting(s,"bars",1,16),root=setting(s,"root",0,11),scale=setting(s,"scale",0,12),genre=setting(s,"genre",0,60);
  Pattern base;base.length=bars*384;
  auto old=snapshot();if(mutate&&valid(old))for(const auto&n:old["notes"]){Event e;e.tick=n[0];e.length=n[1];e.key=n[2];e.velocity=n[3];if(e.tick<base.length)base.add(e);}
  if(kit&&!supportsDrums(genre))throw std::runtime_error("This genre has no drum pattern");
  Gen8Settings gs;gs.seed=uint16_t(seed);gs.genre=genre<2?seed%gen8_genre_count:gen_fm1_menu[(genre-2)%gen8_genre_count];gs.complexity=setting(s,"complexity",0,100);gs.amount=setting(s,"energy",0,100);
  if(kit&&genre<2){std::vector<unsigned>choices;for(unsigned i=0;i<gen8_genre_count;i++)if(supportsDrums(int(i)+2))choices.push_back(gen_fm1_menu[i]);gs.genre=choices[seed%choices.size()];}
  Gen8Voice voice;voice.drums=kit;voice.drum_keys=drumKeys;Pattern out=base;
  if(kit){out.events.clear();for(auto role:{Gen8Role::Kick,Gen8Role::Snare,Gen8Role::Hat,Gen8Role::Percussion}){auto part=mutate?mutate8(base,role,gs,voice,root,scale):generate8(base,role,gs,voice,root,scale);for(auto&e:part.events)out.add(e);}}
  else if(mode==1){ChordGenSettings c;c.genre=genre;c.bars=bars;c.chords=setting(s,"chords",1,16);c.rhythm=gs.complexity;c.energy=gs.amount;c.color=setting(s,"color",0,5);c.play=setting(s,"play",0,2);c.gate=setting(s,"gate",10,100);c.octave=setting(s,"octave",1,6);c.root=root;c.scale=scale;c.local=1;c.seed=seed;out=generate_chords(base,c,voice,root,scale,384,mutate);}
  else out=mutate?mutate8(base,Gen8Role::Melody1,gs,voice,root,scale):generate8(base,Gen8Role::Melody1,gs,voice,root,scale);
  // Ratchets become explicit note pairs. Clamp at the exact loop boundary and
  // merge same-pitch overlaps so a note-off cannot cut another overlapping hit.
  struct N{int tick,length,key,velocity;};std::vector<N>notes;
  for(auto&e:out.events){if(e.flags&step_mute)continue;for(unsigned r=0;r<e.hits();r++){int tick=e.tick+int(double(e.length)*r/e.hits());if(tick>=int(base.length))continue;int length=std::clamp(int(double(e.length)/e.hits()),1,int(base.length)-tick);notes.push_back({tick,length,int(e.key),e.hit_velocity(e.velocity,r)});}}
  std::stable_sort(notes.begin(),notes.end(),[](auto&a,auto&b){return a.key==b.key?a.tick<b.tick:a.key<b.key;});
  std::vector<N>merged;for(auto n:notes){if(!merged.empty()&&merged.back().key==n.key&&merged.back().tick+merged.back().length>n.tick){auto&last=merged.back();last.length=std::max(last.tick+last.length,n.tick+n.length)-last.tick;}else merged.push_back(n);}
  Json result={{"schema",1},{"settings",s},{"seed",seed},{"kind",kit?"drums":mode==1?"chords":"melody"},{"length",base.length},{"notes",Json::array()}};
  for(auto&n:merged)result["notes"].push_back({n.tick,n.length,n.key,n.velocity});restore(result);return result;
 }
 static juce::MidiFile midiFile(const Json&d){
  juce::MidiFile file;file.setTicksPerQuarterNote(96);juce::MidiMessageSequence sequence;
  auto name=juce::MidiMessage::textMetaEvent(3,"RHYTHMER "+juce::String(d.value("kind",std::string("pattern"))));sequence.addEvent(name,0);sequence.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4,4),0);
  // Deliberately no tempo event: importing this clip must not change host BPM.
  if(valid(d)){for(const auto&n:d["notes"]){int tick=n[0],length=n[1],key=n[2],velocity=n[3];sequence.addEvent(juce::MidiMessage::noteOn(1,key,juce::uint8(velocity)),tick);sequence.addEvent(juce::MidiMessage::noteOff(1,key),tick+length);}
   sequence.addEvent(juce::MidiMessage::endOfTrack(),d["length"].get<int>());}
  sequence.updateMatchedPairs();file.addTrack(sequence);return file;
 }
 static bool write(const juce::File&path,const Json&d){if(!valid(d))return false;juce::MemoryOutputStream stream;if(!midiFile(d).writeTo(stream,0))return false;return path.replaceWithData(stream.getData(),stream.getDataSize());}
};
// Store editor composition beside the existing backend JSON, retaining MPCP
// parameter layout and legacy session compatibility. Bounds checked before use.
static size_t extraOffset(const std::vector<uint8_t>&b){
 if(b.size()<12||std::memcmp(b.data(),"MPCP",4))return 0;auto u32=[&](size_t p){uint32_t n;std::memcpy(&n,b.data()+p,4);return n;};if(u32(4)!=2)return 0;size_t p=12;uint32_t count=u32(8);if(count>4096)return 0;
 for(uint32_t i=0;i<count;i++){if(p>=b.size())return 0;size_t n=b[p++];if(n+8>b.size()-p)return 0;p+=n+8;}
 if(p+4>b.size()||u32(p)!=b.size()-p-4)return 0;return p;
}
static MidiWorkspace::Json readWorkspace(const std::vector<uint8_t>&b){size_t p=extraOffset(b);if(!p)return {};auto j=re::Json::parse(b.begin()+p+4,b.end(),nullptr,false);return j.is_object()?j.value("desktop_midi",re::Json::object()):re::Json::object();}
static void writeWorkspace(std::vector<uint8_t>&b,const re::Json&d){size_t p=extraOffset(b);if(!p||!MidiWorkspace::valid(d))return;auto j=re::Json::parse(b.begin()+p+4,b.end(),nullptr,false);if(!j.is_object())return;j["desktop_midi"]=d;auto text=j.dump();uint32_t n=uint32_t(text.size());b.resize(p+4);std::memcpy(b.data()+p,&n,4);b.insert(b.end(),text.begin(),text.end());}
}
