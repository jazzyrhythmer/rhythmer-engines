#include "third_party/json.hpp"
#include "compact_pages.h"
#include <set>
#include <iostream>
struct C{int index;std::string group;nlohmann::json metadata;};
static void check(bool v,const char*s){if(!v)throw std::runtime_error(s);}
int main(){using namespace re;std::vector<C>cs;int index=0;auto group=[&](std::string name,int n,std::string section="SOUND",std::string panel=""){for(int i=0;i<n;i++)cs.push_back({index++,name,{{"section",section},{"source_panel",panel.empty()?name:panel}}});};
group("FILTER",3);group("MIX",2);group("OSC",6);group("VOICE",4);group("ENV",4,"MOD");group("LFO",4,"MOD");group("DELAY",4,"FX");group("REVERB",4,"FX");
auto pages=compactControlPages(cs,"ALL");check(pages.size()==2,"Small related groups did not pack");std::set<int>found;for(auto&p:pages){check(p.size()<=16,"Page overflow");std::string family=p[0].metadata["section"];for(auto&c:p){check(found.insert(c.index).second,"Duplicate control");}}check(found.size()==cs.size(),"Lost parameter");check(pages[0].size()==15,"Whole source groups split unnecessarily");auto mod=compactControlPages(cs,"MOD");for(auto&p:mod)for(auto&c:p)check(c.metadata["section"]=="MOD","Section filter leaked sound or FX");
cs.clear();index=0;group("OSC",12,"SOUND","OSC 1");group("OSC",12,"SOUND","OSC 2");pages=compactControlPages(cs,"ALL");check(pages.size()==2&&pages[0].size()==12&&pages[1].size()==12,"Original oscillator panels split");for(auto&p:pages)for(auto&c:p)check(c.metadata["source_panel"]==p[0].metadata["source_panel"],"Source panel torn apart");
cs.clear();index=0;group("LARGE",40);pages=compactControlPages(cs,"ALL");check(pages.size()==3&&pages[0].size()==16,"Oversized group not bounded");check(compactControlPages(cs,"FX").empty(),"Empty FX fell back to sound controls");std::cout<<"COMPACT_PAGES_PASS group_integrity source_panels capacity coverage section_filters\n";}
