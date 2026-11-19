// SPDX-License-Identifier: AGPL-3.0-or-later
#define main old_main
#include "native_instrument.cpp"
#undef main
static void require(bool b,const std::string&m){if(!b)throw std::runtime_error(m);}
static void tick(re::Core&c){float l[128]{},r[128]{};for(int i=0;i<8;i++){c.render(l,r,128);std::this_thread::sleep_for(std::chrono::milliseconds(5));}}
static void open(re::Core&c,int p){c.set(p,1);for(int i=0;i<1000;i++){if(c.get(177)>.5f){tick(c);return;}std::this_thread::sleep_for(std::chrono::milliseconds(5));}throw std::runtime_error("Browser did not open");}
static void close(re::Core&c){c.set(125,1);tick(c);require(c.get(177)<.5f,"Close did not hide browser");}
static void pick(re::Core&c,int index){c.set(124,(index/16)/127.f);tick(c);c.set(126+index%16,1);tick(c);ready(c);require(c.get(177)<.5f,"Selection did not close browser");}
int main(int argc,char**argv){if(argc<3)return 2;try{re::Core c("",0,argv[1]);ready(c);require(c.count()==179,"Browser schema");re::Json catalog;std::ifstream(argv[2])>>catalog;unsigned names=0;
 for(int engine=0;engine<20;engine++){
  open(c,117);c.set(124,(engine/16)/127.f);tick(c);require(text(c,142+engine%16)==catalog["entries"][engine]["name"],"Engine row mismatch");pick(c,engine);require(std::lround(c.get(1)*19)==engine,"Engine selection not applied");
  open(c,118);const auto&items=engine==0?catalog["kits"]:catalog["sources"][int(catalog["entries"][engine]["source_index"])]["sounds"];
  for(int page=0;page<(int(items.size())+15)/16;page++){c.set(124,page/127.f);tick(c);for(int row=0;row<16;row++){int at=page*16+row;std::string shown=text(c,142+row)+text(c,158+row);require(shown==(at<int(items.size())?items[at]["name"].get<std::string>():""),"Preset row mismatch engine="+std::to_string(engine)+" at="+std::to_string(at));if(at<int(items.size()))names++;}}
  pick(c,int(items.size())-1);require(text(c,2)==items.back()["name"],"Selected original preset mismatch");
  std::cout<<"BROWSER_BANK_PASS "<<engine<<" names="<<items.size()<<" all_rows,select_last,engine_link\n";
 }
 c.set(1,0);ready(c);open(c,120);pick(c,10);require(std::lround(c.get(3)*15)==10,"Pad browser not linked to P11");
 open(c,121);pick(c,13);open(c,122);require(text(c,142)!="","Pad sound browser empty");pick(c,0);require(text(c,24)==catalog["sources"][13]["sounds"][0]["name"],"Pad source sound mismatch");
 open(c,123);pick(c,48);require(std::lround(c.get(116)*112)==48,"MIDI root browser mismatch");
 open(c,117);close(c);open(c,118);float before=c.get(6);c.set(6,before<.5f?.8f:.2f);tick(c);close(c);require(std::abs(c.get(6)-before)<.001f,"Hidden browser knob changed sound");
 open(c,118);auto state=c.state();close(c);c.restore(state);ready(c);tick(c);require(c.get(177)<.5f,"Browser unexpectedly reopened after restore");
 std::cout<<"CHOICE_BROWSER_PASS names="<<names<<" engines=20 pad=P11 source,sound,root,close,state,hidden_controls\n";
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}return 0;}
