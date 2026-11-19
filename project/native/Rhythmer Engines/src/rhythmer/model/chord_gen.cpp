// SPDX-License-Identifier: GPL-3.0-only
// Original harmonic/voice-leading adaptation by jazzy rhythmer.
// Genre progression vocabulary: existing JIANT / FM-1 Rhythmer tables.
#include "rhythmer/model/chord_gen.h"
#include "rhythmer/model/gen8_tables.h"
#include "rhythmer/model/gen_menu_order.h"
#include "rhythmer/model/scales.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
namespace rhythmer {
namespace {
struct Random{uint32_t s;explicit Random(uint32_t v):s(v?v:1){}uint32_t next(){s^=s<<13;s^=s>>17;s^=s<<5;return s;}bool chance(unsigned p){return next()%100<std::min(p,100u);}};
int mod(int n,int d){return (n%d+d)%d;}
std::vector<int> degrees(unsigned mask){std::vector<int> d;for(int i=0;i<12;i++)if(mask&(1u<<i))d.push_back(i);if(d.empty())d={0,2,4,5,7,9,11};return d;}
int tone(const std::vector<int>& d,int degree){int k=mod(degree,int(d.size()));return d[k]+12*((degree-k)/int(d.size()));}
int in_range(int n,const Gen8Voice& v){int low=std::min(127,int(v.low)),high=std::clamp(int(v.high),low,127);while(n<low)n+=12;while(n>high&&n-12>=low)n-=12;return std::clamp(n,low,high);}
}
const char* chord_progression_name(unsigned n){static const char* a[]={"AUTO","GENRE","II-V-I","CYCLE","MODAL","BORROWED","CHROMATIC"};return a[n%7];}
const char* chord_color_name(unsigned n){static const char* a[]={"TRIADS","7THS","9THS","ADVANCED"};return a[n%4];}
const char* chord_voicing_name(unsigned n){static const char* a[]={"AUTO","CLOSE","OPEN","ROOTLESS","QUARTAL"};return a[n%5];}
const char* chord_play_name(unsigned n){static const char* a[]={"BLOCK","ARP UP","ARP DOWN","ARP UP-DN","STRUM"};return a[n%5];}
int chord_gen_value(const ChordGenSettings& s,unsigned i,unsigned root,unsigned scale){
 const int v[]={s.genre,s.progression,s.chords,s.bars,s.rhythm,s.color,s.energy,s.play,s.gate,s.voicing,s.octave,s.rate,s.local,s.local?s.root:int(root%12),s.local?s.scale:int(scale%scales().size())};return v[i%15];
}
std::array<int,2> chord_gen_bounds(unsigned i){static const std::array<int,2> b[]={{0,60},{0,6},{1,8},{1,16},{0,100},{0,3},{0,100},{0,4},{10,100},{0,4},{2,6},{0,3},{0,1},{0,11},{0,12}};return b[i%15];}
void chord_gen_set(ChordGenSettings& s,unsigned i,int v,unsigned root,unsigned scale){
 auto b=chord_gen_bounds(i);v=std::clamp(v,b[0],b[1]);
 if((i==13||i==14||(i==12&&v))&&!s.local){s.root=root%12;s.scale=scale%scales().size();s.local=1;}
 switch(i){case 0:s.genre=v;break;case 1:s.progression=v;break;case 2:s.chords=v;break;case 3:s.bars=v;break;case 4:s.rhythm=v;break;case 5:s.color=v;break;case 6:s.energy=v;break;case 7:s.play=v;break;case 8:s.gate=v;break;case 9:s.voicing=v;break;case 10:s.octave=v;break;case 11:s.rate=v;break;case 12:s.local=v;break;case 13:s.root=v;break;case 14:s.scale=v;break;}
}
const char* chord_gen_param_name(unsigned i){static const char* a[]={"GENRE","PROGRESSION","CHORDS","BARS","PATTERN","CHORD COLOR","ENERGY","PLAY STYLE","GATE","VOICING","REGISTER","PLAY RATE","KEY SOURCE","KEY","SCALE"};return a[i%15];}
std::string chord_gen_param_text(const ChordGenSettings& s,unsigned i,unsigned root,unsigned scale){
 int v=chord_gen_value(s,i,root,scale);
 switch(i){case 0:return gen_menu_name(v);case 1:return chord_progression_name(v);case 5:return chord_color_name(v);case 7:return chord_play_name(v);case 9:return chord_voicing_name(v);case 10:return "OCT "+std::to_string(v);case 11:{static const char* arp[]={"1/4","1/8","1/16","1/32"};static const char* strum[]={"TIGHT","NATURAL","LOOSE","WIDE"};return s.play==4?strum[v%4]:arp[v%4];}case 12:return v?"LOCAL":"PROJECT";case 13:{static const char* keys[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};return keys[v%12];}case 14:return scales()[v%scales().size()].name;default:return std::to_string(v)+(i==4||i==6||i==8?" %":"");}
}
int gen_follow_note(int note,unsigned fr,unsigned fm,unsigned tr,unsigned tm,const Gen8Voice& voice){
 auto from=degrees(fm),to=degrees(tm);int relative=note-int(fr%12),pc=mod(relative,12),oct=(relative-pc)/12,which=0,best=99;
 for(unsigned i=0;i<from.size();i++){int d=std::abs(pc-from[i]);d=std::min(d,12-d);if(d<best){best=d;which=int(i);}}
 unsigned target=std::min<unsigned>(to.size()-1,unsigned(which)*to.size()/from.size());
 return in_range(int(tr%12)+12*oct+to[target],voice);
}
Pattern generate_chords(const Pattern& source,const ChordGenSettings& s,const Gen8Voice& voice,unsigned root,unsigned scale,unsigned bar_ticks,bool mutate){
 Pattern out=source;out.events.clear();out.euclids.clear();out.division=Division::Sixteenth;
 bar_ticks=std::clamp(bar_ticks,24u,3072u);unsigned bars=std::clamp<unsigned>(s.bars,1,16),count=std::clamp<unsigned>(s.chords,1,8),rhythm=std::min<unsigned>(s.rhythm,100),color=std::min<unsigned>(s.color,3);
 out.length=bar_ticks*bars;Random rng(s.seed^(mutate?0x39a8d2e1u:0x61491cfbu));
 Random harmony(s.harmony);
 unsigned genre=s.genre<2?harmony.next()%gen8_genre_count:gen_fm1_menu[(s.genre-2)%gen8_genre_count];const auto& g=gen8_genres[genre];
 bool jazz=g.feel==GF_JAZZY||genre==40||genre==41,neo=g.feel==GF_SOUL||g.feel==GF_RNB;
 bool spacious=g.feel==GF_AMB||g.feel==GF_ATMOS||g.feel==GF_LIQUID||genre==12||genre>=51;
 bool lazy=g.hip||neo||g.feel==GF_DRUNK||g.feel==GF_TRIP;
 auto d=degrees(scales()[scale%scales().size()].mask);if(d.size()==12)d={0,2,3,5,7,8,10};bool minor=d.size()==7&&d[2]==3,major=d.size()==7&&d[2]==4;bool tonal=minor||major;
 const auto& base=g.progressions[harmony.next()%3];unsigned family=s.progression%7;
 if(!family)family=neo?5:jazz?2:spacious?4:1;
 unsigned variant=harmony.next()%3;
 std::array<int,8> roots{};std::array<unsigned,8> qualities{};
 static constexpr int ii[3][8]={{1,4,0,5,1,4,0,0},{2,5,1,4,0,3,1,4},{5,1,4,0,3,2,1,4}},cycle[]={0,5,1,4,2,5,1,4},modal[]={0,3,0,5,3,0,6,0},borrowed[]={3,2,5,1,4,0,3,4};
 for(unsigned i=0;i<count;i++){
  roots[i]=family==1?base.degrees[i%base.count]:family==2?ii[variant][i]:family==3?cycle[i]:family==4?modal[i]:family==5?borrowed[i]:cycle[i];
  if(family==5&&major&&variant==1){static const int soul[]={3,3,2,5,1,4,0,0};roots[i]=soul[i];}
  qualities[i]=family==1?base.quality[i%base.count]:unsigned(Q_7);
 }
 // A loop has an audible home; a one-chord setting is a true tonic/modal vamp.
 if(count==1)roots[0]=0;
 std::vector<int> previous;int center=std::clamp<int>((int(s.octave)+1)*12+int(root%12),36,84);
 for(unsigned c=0;c<count;c++){
  uint32_t begin=uint64_t(out.length)*c/count,end=uint64_t(out.length)*(c+1)/count,span=end-begin;if(!span)continue;
  int degree=roots[c],chord_root=int(root%12)+tone(d,degree);bool dominant=tonal&&(((qualities[c]&0xf0)==F_DOM)||(mod(degree,7)==4&&(family==2||family==3||family==5||family==6)));
  std::vector<int> pcs;unsigned voices=color==0?3:color==1?4:color==2?5:6;
  // Diatonic thirds are the safe foundation, including non-heptatonic scales.
  for(unsigned k=0;k<voices;k++){int extension=2*int(k);if(k==5&&tonal&&mod(tone(d,degree+2)-tone(d,degree),12)==4)extension=12;pcs.push_back(mod(int(root%12)+tone(d,degree+extension),12));}
  if(family==5&&major&&variant==1&&c==1){pcs.clear();for(int n:{0,3,7,10,14,17}){if(pcs.size()>=voices)break;pcs.push_back(mod(chord_root+n,12));}}
  if(tonal&&family==1){unsigned flags=qualities[c]&0xf0;
   if(flags==F_FLAT){chord_root=int(root%12)+(degree==2?3:degree==5?8:degree==6?10:tone(d,degree));pcs.clear();for(int n:{0,4,7,10,14,21}){if(pcs.size()>=voices)break;pcs.push_back(mod(chord_root+n,12));}}
   else if(flags==F_MAJ){pcs.clear();for(int n:{0,4,7,11,14,21}){if(pcs.size()>=voices)break;pcs.push_back(mod(chord_root+n,12));}}
   else if(flags==F_PAR){pcs.clear();for(int n:{0,3,7,10,14,17}){if(pcs.size()>=voices)break;pcs.push_back(mod(chord_root+n,12));}}
  }
  if(dominant&&(!minor||color==3)){pcs.clear();std::vector<int> intervals={0,4,7,10,14,21};
   // Altered colour is used only on a resolving dominant, not on every chord.
   if(color==3&&(jazz||neo||family==6))intervals={0,4,10,13,20,22};
   for(unsigned k=0;k<voices;k++)pcs.push_back(mod(chord_root+intervals[k],12));
  }
  if(tonal&&family==6&&color>=2&&c+1<count&&harmony.chance(45)){
   // Tritone-substitute dominant approaching the next written root.
   int next=int(root%12)+tone(d,roots[c+1]);chord_root=next+1;pcs.clear();for(int n:{0,4,10,14,21})pcs.push_back(mod(chord_root+n,12));
  }
  unsigned voicing=s.voicing%5;if(!voicing)voicing=(neo||jazz)&&color>=2?3:spacious?2:1;
  if(voicing==3&&pcs.size()>3){pcs.erase(pcs.begin());if(pcs.size()>4)pcs.erase(pcs.begin()+1);}
  if(voicing==4){pcs.clear();for(unsigned k=0;k<std::min(voices,5u);k++)pcs.push_back(mod(int(root%12)+tone(d,degree+3*int(k)),12));}
  // Remove pitch-class duplicates before searching inversions/registers.
  std::vector<int> unique;for(int pc:pcs)if(std::find(unique.begin(),unique.end(),pc)==unique.end())unique.push_back(pc);pcs=std::move(unique);
  std::vector<int> chosen;double best=std::numeric_limits<double>::max();
  for(unsigned inv=0;inv<pcs.size();inv++)for(int shift=-12;shift<=12;shift+=12){
   std::vector<int> candidate;int last=center-13+shift;
   for(unsigned k=0;k<pcs.size();k++){int n=pcs[(inv+k)%pcs.size()];while(n<=last)n+=12;candidate.push_back(n);last=n;}
   if(voicing==2&&candidate.size()>=4){candidate[1]+=12;std::sort(candidate.begin(),candidate.end());}
   if(candidate.front()<int(voice.low)||candidate.back()>int(voice.high)||candidate.back()>108)continue;
   double cost=std::abs(candidate[candidate.size()/2]-center)*1.5;
   for(unsigned i=0;i<candidate.size();i++){
    if(i&&candidate[i]-candidate[i-1]<=1&&candidate[i]<60)cost+=12; // prevent low-register mud
    if(!previous.empty())cost+=std::abs(candidate[i]-previous[std::min<unsigned>(i,previous.size()-1)]);
   }
   if(!previous.empty())cost+=std::abs(candidate.back()-previous.back())*1.7;
   if(cost<best){best=cost;chosen=std::move(candidate);}
  }
  if(chosen.empty()){for(int pc:pcs)chosen.push_back(in_range(center+mod(pc-center,12),voice));std::sort(chosen.begin(),chosen.end());chosen.erase(std::unique(chosen.begin(),chosen.end()),chosen.end());}
  previous=chosen;
  std::vector<uint32_t> strikes{begin};
  // More rhythm adds re-articulations; it never silently adds harmonic changes.
  for(unsigned bar=begin/bar_ticks;bar<=(end-1)/bar_ticks;bar++){
   uint32_t anchor=bar*bar_ticks;
   std::array<unsigned,4> positions=lazy?std::array<unsigned,4>{6,10,14,15}:std::array<unsigned,4>{4,8,10,14};
   unsigned extras=rhythm>=85?4:rhythm>=60?3:rhythm>=30?2:rhythm>=10?1:0;
   for(unsigned k=0;k<extras;k++)if(rng.chance(35+rhythm/2)){
    uint32_t tick=anchor+uint64_t(positions[k])*bar_ticks/16;
    if(tick>begin&&tick<end&&tick-begin>=12&&end-tick>=12)strikes.push_back(tick);
   }
  }
  std::sort(strikes.begin(),strikes.end());strikes.erase(std::unique(strikes.begin(),strikes.end()),strikes.end());
  for(unsigned h=0;h<strikes.size();h++){
   uint32_t tick=strikes[h],next=h+1<strikes.size()?strikes[h+1]:end;
   unsigned delay=lazy?std::min<unsigned>(5,(next-tick)/8):0;tick=std::min(tick+delay,next-1);
   unsigned duration=std::max(1u,(next-tick)*std::clamp<unsigned>(s.gate,10,100)/100);
   unsigned play=s.play%5;if(voice.mono&&(play==0||play==4))play=1;
   const unsigned arp_steps[]={96,48,24,12},strum_steps[]={1,2,4,8};
   auto emit=[&](uint32_t at,unsigned index,unsigned length){if(at>=next)return;Event e;e.tick=at;e.key=uint8_t(chosen[index]);
    e.velocity=uint8_t(std::clamp(48+int(s.energy)*55/100-int(index)+int(rng.next()%9)-4-(h?4:0),30,120));
    e.length=uint16_t(std::max(1u,std::min<unsigned>(length,next-at)));out.add(e);};
   if(play>=1&&play<=3){
    unsigned step=std::max(1u,std::min<unsigned>(arp_steps[s.rate%4],next-tick)),k=0;
    for(uint32_t at=tick;at<next;at+=step,++k){unsigned n=chosen.size(),index=play==2?n-1-k%n:play==3&&n>1?unsigned((k%(2*n-2)<n)?k%(2*n-2):2*n-2-k%(2*n-2)):k%n;
     emit(at,index,std::max(1u,step*std::clamp<unsigned>(s.gate,10,100)/100));}
   }else{
    unsigned spread=play==4?std::min<unsigned>(strum_steps[s.rate%4],std::max(1u,(next-tick-1)/std::max<unsigned>(chosen.size(),1))):0;
    for(unsigned i=0;i<chosen.size();i++)emit(tick+i*spread,i,duration>i*spread?duration-i*spread:1);
   }
  }
 }
 out.sort();return out;
}
}
