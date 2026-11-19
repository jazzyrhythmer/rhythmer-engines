#include "generation.h"
#include <iostream>
int main(){re::Generator g;rhythmer::Pattern p;unsigned checks=0;
 for(int genre=0;genre<60;genre++)for(int mode=0;mode<5;mode++){
  re::GenSettings s;s.genre=genre;s.mode=mode==1;s.bars=mode==2?1:4;
  auto out=g.create(p,s,mode==2,mode==3,mode==4,false,1234);
  re::Schedule schedule;schedule.assign(out);
  if(out.length==0)return 2; if(out.events.empty())std::cerr<<"Empty genre="<<genre<<" mode="<<mode<<"\n";
  for(const auto&e:out.events){if(e.key>127||e.tick>=out.length||e.velocity>127||!e.length){std::cerr<<"INVALID genre="<<genre<<" mode="<<mode<<" tick="<<e.tick<<" length="<<e.length<<" pattern="<<out.length<<"\n";return 3;}checks++;}
 }
 std::cout<<"GEN checks "<<checks<<" PASS\n";
}
