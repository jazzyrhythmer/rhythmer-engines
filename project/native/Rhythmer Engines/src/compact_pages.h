// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <vector>
#include <string>
#include <algorithm>
namespace re {
// Desktop: pack whole semantic groups. A group larger than a page is divided
// by its original source panels before unavoidable capacity-sized chunks.
// ALL presents sound, modulation and FX in order, packing small whole groups.
// The explicit SOUND/MOD/FX filters still isolate their own controls.
template<class C>std::vector<std::vector<C>>compactControlPages(const std::vector<C>&controls,const std::string&section,size_t capacity=16){
 struct Group{std::string section,name;std::vector<C>items;};std::vector<Group>groups;
 for(const auto&c:controls){std::string family=c.metadata.value("section",std::string("SOUND"));std::string groupName=c.group;std::transform(groupName.begin(),groupName.end(),groupName.begin(),[](unsigned char ch){return char(std::toupper(ch));});if(groupName.find("MODEL")!=std::string::npos)family="SOUND";if(section!="ALL"&&family!=section)continue;auto it=std::find_if(groups.begin(),groups.end(),[&](const Group&g){return g.section==family&&g.name==c.group;});if(it==groups.end()){groups.push_back({family,c.group,{}});it=groups.end()-1;}it->items.push_back(c);}
 std::stable_sort(groups.begin(),groups.end(),[](const Group&a,const Group&b){auto rank=[](const std::string&s){return s=="SOUND"?0:s=="MOD"?1:2;};return rank(a.section)<rank(b.section);});
 std::vector<Group>units;
 for(auto&g:groups){if(g.items.size()<=capacity){units.push_back(g);continue;}std::vector<Group>panels;for(auto&c:g.items){std::string panel=c.metadata.value("source_panel",g.name);auto it=std::find_if(panels.begin(),panels.end(),[&](const Group&p){return p.name==panel;});if(it==panels.end()){panels.push_back({g.section,panel,{}});it=panels.end()-1;}it->items.push_back(c);}for(auto&p:panels)for(size_t at=0;at<p.items.size();at+=capacity)units.push_back({p.section,p.name,{p.items.begin()+at,p.items.begin()+std::min(at+capacity,p.items.size())}});}
 std::vector<std::vector<C>>pages;std::string family;
 for(auto&unit:units){if(pages.empty()||pages.back().size()+unit.items.size()>capacity){pages.emplace_back();family=unit.section;}pages.back().insert(pages.back().end(),unit.items.begin(),unit.items.end());}
 return pages;
}
template<class C>std::string compactPageTitle(const std::vector<C>&page){std::string title;std::vector<std::string>seen;for(auto&c:page)if(std::find(seen.begin(),seen.end(),c.group)==seen.end()){seen.push_back(c.group);if(!title.empty())title+=" / ";title+=c.group;}return title;}
}
