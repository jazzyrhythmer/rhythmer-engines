// SPDX-License-Identifier: GPL-3.0-only
// Adapted from Rhythmer gen_break_midi and break_sequence.c (see third_party/jiant-gen).
// Native sequencer adaptation: jazzy rhythmer. No sample audio is included.
#include "rhythmer/model/gen8.h"
#include "rhythmer/model/gen8_tables.h"
#include <cstring>

namespace rhythmer {
namespace {
struct Dice {
    uint32_t q;
    explicit Dice(unsigned seed):q((seed+1u)*2654435761u ^ 0x6D2B79F5u) {if(!q)q=1;}
    unsigned get(unsigned n) {q^=q<<13;q^=q>>17;q^=q<<5;return n?q%n:0;}
};
unsigned priority(char c) {return c=='X'?255:c>='0'&&c<='9'?unsigned(c-'0')*28:0;}
int slice_of(uint8_t key,const BreakMap& map) {
    for(unsigned i=0;i<16;i++)if(key==map.slices[i])return int(i);
    return -1;
}
}
Pattern break_generate(const Pattern& source,BreakAction action,const Gen8Settings& s,
                       const BreakMap& map,unsigned bpm) {
    Pattern out=source;
    unsigned amount=std::min<unsigned>(s.amount,100), complexity=std::min<unsigned>(s.complexity,100);
    if(!source.length || (!amount && action!=BreakAction::Straight))return out;
    Dice rng(s.seed);
    unsigned steps=std::min((source.length+23u)/24u,2048u);
    if(action==BreakAction::Mutate) {
        // bs_vary: retain quarter-note anchors and all non-audio data. Never randomise an empty phrase.
        size_t fallback=out.events.size(); bool changed=false;
        for(size_t i=0;i<out.events.size();i++) {
            auto& e=out.events[i]; int old=slice_of(e.key,map);
            if(old<0 || e.tick>=source.length || e.tick%96==0)continue;
            if(fallback==out.events.size())fallback=i;
            if(rng.get(200)>=amount)continue;
            e.key=map.slices[(unsigned(old)+1+rng.get(15))&15];changed=true;
        }
        if(!changed&&fallback<out.events.size()) {
            auto& e=out.events[fallback];e.key=map.slices[(unsigned(slice_of(e.key,map))+1)&15];
        }
        return out;
    }
    if(action==BreakAction::Fill) {
        // Original BS_FILL: last four/eight steps; a real slice precedes every roll.
        if(std::none_of(out.events.begin(),out.events.end(),[&](const Event& e){return e.tick<out.length&&slice_of(e.key,map)>=0;}))return out;
        unsigned count=amount>75?8:4,start=steps>count?steps-count:0;
        uint32_t from=start*24;
        out.events.erase(std::remove_if(out.events.begin(),out.events.end(),[&](const Event& e){return e.tick>=from&&e.tick<out.length&&slice_of(e.key,map)>=0;}),out.events.end());
        for(unsigned i=start;i<steps;i++) {
            Event e;e.tick=i*24;e.length=uint16_t(std::min(24u,out.length-e.tick));
            e.key=map.slices[(start/2)&15];e.velocity=uint8_t(80+(i-start)*5);
            e.ratchet=uint8_t(i==start?1:2+rng.get(2)*2);out.add(e);
        }
        out.sort();return out;
    }
    out.events.clear();out.euclids.clear();
    const auto& g=gen8_genres[s.genre%gen8_genre_count];
    bool half=action==BreakAction::Genre&&g.hip&&bpm>=120;
    const unsigned stride=half?2:1;
    int previous=-1;
    for(unsigned i=0;i<steps;i++) {
        if(i%stride)continue;
        unsigned cell=i/stride,pos=cell&15,bar=(cell/16)%std::clamp(map.bars,1u,4u),slice=0,hit=0;
        unsigned ratchet=1;int velocity=100;
        if(action==BreakAction::Straight) {
            unsigned width=std::clamp(map.bars,1u,4u);
            if(i%width)continue;
            slice=(i/width)&15;
        } else if(action==BreakAction::Random) {
            // bs_vary BS_RANDOM: first downbeat fixed, amount controls sixteenth retriggers.
            if((i&1)&&rng.get(100)>=amount)continue;
            slice=i?rng.get(16):0;
        } else {
            int density=std::clamp(40+int(amount)*2+(int(amount)-50)/2,20,255);
            for(unsigned l=0;l<8;l++) {
                const char* d=g.drums[l];unsigned n=unsigned(std::strlen(d));
                if(!n)continue;
                unsigned lv=priority(d[pos%n]);
                if(lv>unsigned(255-std::clamp(density+(l==0?40:l<3?20:l>=5?-20:0),0,255)))hit|=1u<<l;
            }
            // Rhythmer's coherent feel: downbeat/backbeats, half-time Future/Nocturnal, Jungle fourth-bar reply.
            if(g.feel && amount>20) {
                if(pos==0)hit|=1;
                bool halftime=g.feel==GF_FUTURE||g.feel==GF_NOCT;
                if(halftime&&(pos==4||pos==12))hit&=~6u;
                if(pos==(halftime?8u:4u)||(!halftime&&pos==12))hit|=2;
            }
            if(g.fast&&complexity>40&&pos>=14)hit=2;
            if(!hit)continue;
            if(g.hip&&!(hit&3)&&(pos&3)&&rng.get(100)<((pos&1)?80-complexity/3:40))continue;
            slice=bar*8+((hit&1)?(pos>=8?4:0):(hit&6)?(pos>=8?6:2):((pos/2)|1));
            if(g.fast&&(hit&6)&&pos!=4&&pos!=12&&rng.get(100)<complexity)slice^=4;
            slice=std::min(15u,slice*2/std::clamp(map.bars,1u,4u));
            if(!(hit&7))velocity=g.hip?65:84;
            // Original command bank 24..31 becomes a ratchet on this slice, with no hidden prior-note state.
            if(g.fast&&complexity>40&&pos>=14)ratchet=complexity>75?4:2;
        }
        Event e;e.tick=i*24;e.key=map.slices[slice&15];e.velocity=uint8_t(velocity);e.ratchet=uint8_t(ratchet);
        e.length=uint16_t(std::min(24u*stride,out.length-e.tick));
        // ST_TIE/continuous chops become a bounded native note length.
        if(previous>=0)out.events[size_t(previous)].length=uint16_t(std::min<uint32_t>(65535,e.tick-out.events[size_t(previous)].tick));
        out.add(e);previous=int(out.events.size())-1;
    }
    if(previous>=0) {
        auto& e=out.events[size_t(previous)];e.length=uint16_t(std::min<uint32_t>(65535,out.length-e.tick));
    }
    // Ratchets occupy their own grid cell; a later rest must not stretch a roll across the phrase.
    for(auto& e:out.events)if(e.ratchet>1)e.length=uint16_t(std::min<unsigned>(e.length,24*stride));
    return out;
}
} // namespace rhythmer
