// SPDX-License-Identifier: GPL-3.0-only
// Derived musical rules: JIANT, Copyright (C) 2026 Juan Ignacio Antonucci.
// Rhythmer genre/rhythmer adaptation and native event integration: jazzy rhythmer.
#include "rhythmer/model/gen8.h"
#include "rhythmer/model/gen8_tables.h"
#include "rhythmer/model/scales.h"
#include <cstring>

namespace rhythmer {
namespace {
struct Rng {
    uint32_t q;
    Rng(unsigned seed, unsigned part) : q((seed+1u)*2654435761u ^ (part+1u)*0x9E3779B9u ^ 0x6D2B79F5u) {
        if (!q) q=1;
        next(); next(); next();
    }
    uint32_t next() { q^=q<<13; q^=q>>17; q^=q<<5; return q; }
    bool chance(unsigned p) { return next()%100 < std::min(p,100u); }
};
int fit(int note, const Gen8Voice& v) {
    int lo=std::min<unsigned>(v.low,127), hi=std::clamp<int>(v.high,lo,127);
    while (note<lo) note+=12;
    while (note>hi && note-12>=lo) note-=12;
    return std::clamp(note,lo,hi);
}
// Priority maps from gen_tables.c: X anchors survive; ghosts disappear first.
unsigned level(char c) { return c=='X'?255:c>='0'&&c<='9'?unsigned(c-'0')*28:0; }
}

const char* gen8_role_name(Gen8Role r) {
    static const char* names[] {"BASS","CHORDS","MELODY 1","MELODY 2","KICK","SNARE","HI-HAT","PERCUSSION"};
    return names[unsigned(r)%8];
}

Pattern generate8(const Pattern& source, Gen8Role role, const Gen8Settings& s,
                  const Gen8Voice& voice, unsigned root, unsigned scale) {
    Pattern out=source; out.events.clear(); out.euclids.clear();
    if (!source.length || !s.amount) return out;
    const auto& g=gen8_genres[s.genre%gen8_genre_count];
    unsigned r=unsigned(role)%8, amount=std::min<unsigned>(s.amount,100);
    Rng rng(s.seed,r), harmony(s.seed,20);
    const auto& prog=g.progressions[harmony.next()%3];
    const char* bass=g.bass[harmony.next()%3];
    std::array<uint8_t,12> degrees {};
    unsigned nd=scale_degrees(scale,degrees);
    auto tone=[&](int degree) { int oct=degree>=0?degree/int(nd):(degree-int(nd)+1)/int(nd);
        return int(root%12)+12*oct+degrees[unsigned(degree-oct*int(nd))]; };
    // The original QSTACK retains sevenths, ninths, rootless and suspended colours.
    static const int stack[][4] {{0,2,4,127},{0,2,4,6},{0,2,6,8},{0,2,4,8},{0,1,4,127},
        {0,3,4,127},{0,2,4,5},{0,3,6,9},{0,4,7,127},{0,2,6,10},{0,2,6,127},
        {2,4,6,8},{2,6,8,12},{0,2,5,8},{0,3,6,8}};
    auto chord_tone=[&](unsigned bar,unsigned index) {
        unsigned c=bar%prog.count,q=prog.quality[c], quality=q&15;
        int degree=prog.degrees[c], off=stack[quality<15?quality:0][index%4];
        if(off==127) off=0;
        int n=tone(degree+off);
        if((q&0xf0)==F_MAJ||(q&0xf0)==F_DOM) {
            static const int major[4] {0,4,7,10};
            n=tone(degree)+major[index%4];
        } else if((q&0xf0)==F_FLAT) {
            static const int major[4] {0,4,7,10};
            n=int(root%12)+(degree==2?3:degree==5?8:degree==6?10:tone(degree)-int(root%12))+major[index%4];
        } else if((q&0xf0)==F_PAR) {
            n=tone(degree)+((index%4)==1?3:(index%4)==2?7:(index%4)==3?10:0);
        }
        return n;
    };
    auto hit=[&](unsigned step,int note,int vel,unsigned length=18,unsigned ratchet=1) {
        Event e; e.tick=step*sixteenth_ticks;
        if(e.tick>=out.length) return;
        e.key=uint8_t(note); e.velocity=uint8_t(std::clamp(vel,1,127));
        if(vel>=100)e.flags|=step_accent;
        e.length=uint16_t(std::min<unsigned>(length,out.length-e.tick)); e.ratchet=uint8_t(ratchet);
        // Reuse Rhythmer's repeating timing cells, never move the opening downbeat backwards.
        static const int lazy[16] {0,1,-1,3,7,2,0,4,0,2,-1,3,8,2,0,4};
        int micro=r==4?0:r==0?(step%4?1:0):lazy[step%16];
        int offset=micro*int(g.human)*int(sixteenth_ticks)/(100*64);
        e.tick=uint32_t(std::clamp<int64_t>(int64_t(e.tick)+offset,0,out.length-1));
        e.length=uint16_t(std::min<unsigned>(e.length,out.length-e.tick));
        out.add(e);
    };
    unsigned steps=(out.length+sixteenth_ticks-1)/sixteenth_ticks;
    // Bound generated data even on 128-bar projects.
    steps=std::min(steps,2048u);
    int motif[8]; for(auto& n:motif)n=int(rng.next()%4);
    if(voice.acid) {
        // Rhythmer gen_acid: weighted root/octave/b7/fifth/b3/second,
        // repeating sixteen-step motif, accents and adjacent-note slides.
        static const unsigned weights[]={55,15,10,10,5,5};
        static const int intervals[]={0,12,10,7,3,2};
        unsigned pitch[16];bool on[16];
        for(unsigned i=0;i<16;i++){on[i]=i%4==0||rng.chance(45+s.complexity/3);unsigned n=rng.next()%100,k=0;while(k<5&&n>=weights[k]){n-=weights[k];++k;}pitch[i]=k;}
        for(unsigned bar=0;bar<(steps+15)/16;bar++){
            bool acc[16]{},slides[16]{};unsigned target=3+rng.next()%3,k=0;
            for(unsigned tries=0;tries<32&&k<target;tries++){unsigned p=rng.next()%16;if(on[p]&&!acc[p]){acc[p]=true;k++;}}
            target=2+rng.next()%2;k=0;
            for(unsigned tries=0;tries<32&&k<target;tries++){unsigned p=rng.next()%15;if(on[p]&&on[p+1]&&!slides[p]){slides[p]=true;k++;if(pitch[p+1]==0&&rng.chance(50))pitch[p+1]=1;}}
            for(unsigned p=0;p<16;p++){unsigned i=bar*16+p;if(i>=steps||!on[p])continue;
                int n=36+tone(prog.degrees[bar%prog.count])+intervals[pitch[p]];
                if(p==0)n=36+tone(prog.degrees[bar%prog.count]);
                hit(i,fit(n,voice),acc[p]?112:90,slides[p]?30:18);
                if(slides[p]&&!out.events.empty())out.events.back().flags|=step_slide;
            }
        }
        out.sort();return out;
    }
    for(unsigned i=0;i<steps;i++) {
        unsigned p=i%16,bar=i/16,c=bar%prog.count;
        if(r>=4) {
            unsigned begin=r==4?0:r==5?1:r==6?3:5, end=r==5?3:r==6?5:r==7?8:1;
            for(unsigned l=begin;l<end;l++) {
                const char* map=g.drums[l]; unsigned n=unsigned(std::strlen(map));
                if(!n)continue;
                unsigned lv=level(map[p%n]), density=std::clamp(40+int(amount)*2+(int(amount)-50)/2+(l==0?40:0),20,255);
                if(lv<=255-density)continue;
                unsigned rat=(r==6&&g.fast&&s.complexity>65&&p>=14)?2:1;
                if(voice.drum_keys[l]!=255)hit(i,voice.drum_keys[l],lv>=230?110:50+int(lv)*50/255,24,rat);
            }
        } else if(r==0) {
            unsigned n=unsigned(std::strlen(bass)); char b=n?bass[i%n]:'.';
            if(!n) { // BA_KICK / BA_DOUBLE are written against the shared kick cell.
                unsigned kn=unsigned(std::strlen(g.drums[0]));
                if(kn&&level(g.drums[0][p%kn])>100)b='R';
            }
            if(b=='=') {
                if(!out.events.empty()) {
                    auto& last=out.events.back();
                    last.length=uint16_t(std::min<uint32_t>(65535,std::min(out.length,(i+1)*24)-last.tick));
                }
                continue;
            }
            if(b=='.'||b=='-'||b=='_'||b==' ')continue;
            if(p%4 && !rng.chance(amount))continue;
            int d=b=='3'?2:b=='5'?4:b=='7'?6:b=='2'?1:b=='6'?5:0;
            int note=36+tone(prog.degrees[c]+d)+(b=='8'?12:b=='L'?-12:0);
            if(b=='N'||b=='A')note=36+tone(prog.degrees[(c+1)%prog.count])-(b=='A'?1:0);
            hit(i,fit(note,voice),b=='x'?60:p%4?86:104,20);
        } else if(r==1) {
            if(p!=0 && (p!=10 || !rng.chance(amount/2)))continue;
            if(voice.mono) {
                for(unsigned k=0;k<4;k++)if(i+k<steps)hit(i+k,fit(48+chord_tone(bar,k),voice),82,20);
            } else for(unsigned k=0;k<4;k++) {
                if(stack[(prog.quality[c]&15)<15?prog.quality[c]&15:0][k]==127)continue;
                hit(i,fit(48+chord_tone(bar,k),voice),82,180);
            }
        } else {
            // Two related voices with contrasting registers and call/answer space.
            bool answer=r==3;
            if((bar%2==0)==answer && p<8)continue;
            if(p%(answer?4:2) || !rng.chance(amount))continue;
            unsigned m=unsigned(motif[(p/2)%8]);
            hit(i,fit((answer?60:72)+chord_tone(bar,m),voice),p%4?76:96,answer?36:18);
        }
    }
    out.sort(); return out;
}

Pattern mutate8(const Pattern& source, Gen8Role role, const Gen8Settings& s,
                const Gen8Voice& voice, unsigned root, unsigned scale) {
    Pattern out=source;
    if(!s.amount || source.events.empty())return out;
    Rng rng(s.seed,40+unsigned(role));
    Pattern candidates=generate8(source,role,s,voice,root,scale);
    for(auto& e:out.events) {
        if(e.tick>=out.length || e.tick%ticks_per_quarter==0 || (e.flags&step_accent) || !rng.chance(std::min<unsigned>(s.amount,100)/2))continue;
        // Preserve rests/anchors and all motion. Drum keys never become pitches.
        if(unsigned(role)<4&&!voice.drums) {
            auto at=std::find_if(candidates.events.begin(),candidates.events.end(),[&](const Event& x){return x.tick/24==e.tick/24;});
            if(at!=candidates.events.end())e.key=at->key;
        }
        e.velocity=uint8_t(std::clamp(int(e.velocity)+int(rng.next()%21)-10,25,127));
    }
    return out;
}
} // namespace rhythmer
