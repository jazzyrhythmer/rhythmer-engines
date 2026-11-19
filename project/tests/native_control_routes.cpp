// SPDX-License-Identifier: AGPL-3.0-or-later
#define main previous_integration_main
#include "native_instrument.cpp"
#undef main
#include <set>
int main(int argc,char**argv){if(argc<3)return 2;try{re::Core c("",0,argv[1]);ready(c);c.set(1,0);ready(c);c.set(3,10.f/15);c.set(4,0);unsigned pagesChecked=0,bindingsChecked=0,edits=0;
for(int source=0;source<26;source++){
 c.set(23,source/25.f);ready(c);c.set(5,0);auto first=extra(c);int pages=first["detail_pages"];if(pages<1||pages>512)throw std::runtime_error("Bad detail page count");
 for(int page=0;page<pages;page++){
  c.set(5,page/511.f);auto state=extra(c);bool any=false;
  for(int slot=0;slot<8;slot++){
   uint64_t token=state["bindings"][8+slot];if(!token)continue;int cell=(token>>16)&0xffff,index=int(token&0xffff)-4;any=true;bindingsChecked++;
   if(index<0)continue;
   if(cell!=10||index>=int(state["graph"]["cells"][cell]["values"].size()))throw std::runtime_error("Wrong selected-pad binding");
   if(c.name(14+slot).rfind("P11 ",0)!=0)throw std::runtime_error("Wrong selected-pad label");
  }
  if(!any)throw std::runtime_error("Empty detail page");pagesChecked++;
 }
 // A real edit on this source must leave the other fifteen pads unchanged.
 bool edited=false;
 for(int editPage=0;editPage<pages&&!edited;editPage++){
 c.set(5,editPage/511.f);auto before=extra(c);
 for(int slot=0;slot<8&&!edited;slot++){
  uint64_t token=before["bindings"][8+slot];int index=int(token&0xffff)-4;if(!token||index<0)continue;
  float old=c.get(14+slot);c.set(14+slot,old<.5f?.75f:.25f);play(c,46);ready(c);auto after=extra(c);
  for(int pad=0;pad<16;pad++)if(pad!=10&&before["graph"]["cells"][pad]!=after["graph"]["cells"][pad])throw std::runtime_error("Edit changed another pad");
  if(before["graph"]["cells"][10]==after["graph"]["cells"][10])throw std::runtime_error("Source edit did not reach sound state");edited=true;edits++;
 }
 }
 if(!edited)throw std::runtime_error("No editable source control");
 std::cout<<"SOURCE "<<source<<" detail_pages="<<pages<<" P11 routes=PASS"<<std::endl;
}
std::cout<<"CONTROL_ROUTES_PASS pages="<<pagesChecked<<" bindings="<<bindingsChecked<<" source_edits="<<edits<<" other_pads_unchanged=15"<<std::endl;
}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}return 0;}
