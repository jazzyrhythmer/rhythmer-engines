// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "rhythmer/model/chord_gen.h"
#include "rhythmer/model/acid_gen.h"
#include "rhythmer/model/scales.h"
#include "rhythmer/model/gen_menu_order.h"
#include <array>
#include <chrono>
#include <stdexcept>
namespace re {
// Settings belong to this instrument, never to the MPC's project tempo/key.
struct GenSettings {
 int mode=0,genre=41,bars=2,complexity=50,energy=65,root=0,scale=1;
 int chords=4,color=3,voicing=0,play=0,progression=0,rate=1,gate=80;
 int density=70,accent=40,slide=25,octaves=2,octave=1;
 int breakAction=0;
};
class Generator {
 std::array<uint32_t,8>recent_{};unsigned cursor_=0;uint32_t entropy_=0x4a467137;
 public:
 uint32_t nextSeed(){
  uint32_t x=entropy_^uint32_t(std::chrono::steady_clock::now().time_since_epoch().count());
  do{x^=x<<13;x^=x>>17;x^=x<<5;if(!x)x=0x9e3779b9;}while(std::find(recent_.begin(),recent_.end(),x)!=recent_.end());
  recent_[cursor_++%8]=x;entropy_=x;return x;
 }
 rhythmer::Pattern create(const rhythmer::Pattern& previous,const GenSettings&s,bool kit,bool acid,bool brk,bool mutate=false,uint32_t replay=0){
  using namespace rhythmer;
  Pattern base=previous;base.length=unsigned(std::clamp(s.bars,1,16))*384;
  Gen8Settings settings;settings.seed=uint16_t(replay?replay:nextSeed());settings.genre=s.genre<2?settings.seed%gen8_genre_count:gen_fm1_menu[(s.genre-2)%gen8_genre_count];settings.complexity=s.complexity;settings.amount=s.energy;
  Gen8Voice voice;voice.drums=kit;voice.drum_keys={{36,37,38,39,40,41,42,43}};voice.acid=acid;voice.mono=acid;
  const auto& scale=scales()[std::clamp(s.scale,0,int(scales().size()-1))];
  if(acid){AcidGenSettings a;a.density=s.density;a.accent=s.accent;a.slide=s.slide;a.octaves=s.octaves;a.octave=s.octave;a.seed=replay?replay:nextSeed();return acid_generate_pattern(base,a,mutate,s.root,scale.mask);}
  if(brk){BreakMap map;map.bars=s.bars;for(unsigned i=0;i<16;i++)map.slices[i]=36+i;return break_generate(base,BreakAction(std::clamp(s.breakAction,0,4)),settings,map,120);}
  if(kit){
   Pattern out=base;out.events.clear();out.euclids.clear();
   for(auto role:{Gen8Role::Kick,Gen8Role::Snare,Gen8Role::Hat,Gen8Role::Percussion}){
    auto part=mutate?mutate8(base,role,settings,voice,s.root,s.scale):generate8(base,role,settings,voice,s.root,s.scale);
    for(const auto&e:part.events)out.add(e);
   }
   out.sort();return out;
  }
  if(s.mode==1){ChordGenSettings c;c.genre=s.genre;c.bars=s.bars;c.chords=s.chords;c.rhythm=s.complexity;c.energy=s.energy;c.color=s.color;c.voicing=s.voicing;c.play=s.play;c.progression=s.progression;c.rate=s.rate;c.gate=s.gate;c.octave=s.octave;c.local=1;c.root=s.root;c.scale=s.scale;c.seed=replay?replay:nextSeed();return generate_chords(base,c,voice,s.root,s.scale,384,mutate);}
  return mutate?mutate8(base,Gen8Role::Melody1,settings,voice,s.root,s.scale):generate8(base,Gen8Role::Melody1,settings,voice,s.root,s.scale);
 }
};
// A fixed immutable schedule, built away from process(). Internal audio only.
struct ScheduledNote {double tick=0;uint8_t status=0,key=0,velocity=0;};
struct Schedule {
 static constexpr size_t maxEvents=32768;
 std::array<ScheduledNote,maxEvents>events{};size_t count=0;double length=384;
 void assign(const rhythmer::Pattern&p){
  count=0;length=std::max(1u,p.length);
  for(const auto&e:p.events){
   if(e.flags&rhythmer::step_mute)continue;
   unsigned ratchets=std::clamp<unsigned>(e.ratchet,1,8);
   for(unsigned r=0;r<ratchets;r++){
    if(count+2>maxEvents)throw std::runtime_error("GEN event capacity exceeded");
    double tick=e.tick+double(e.length)*r/ratchets;double end=std::min(tick+std::max(1.,double(e.length)/ratchets*(ratchets>1?.95:1.)),length-1e-6);
    events[count++]={tick,uint8_t(0x90),e.key,e.velocity};events[count++]={end,uint8_t(0x80),e.key,0};
   }
  }
  std::stable_sort(events.begin(),events.begin()+count,[](auto&a,auto&b){return a.tick==b.tick?a.status<b.status:a.tick<b.tick;});
 }
};
}
