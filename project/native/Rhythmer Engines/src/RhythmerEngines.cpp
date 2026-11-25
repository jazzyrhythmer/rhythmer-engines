// SPDX-License-Identifier: AGPL-3.0-or-later
// Original source core -> optional single-drum pitch bridge -> kit/output filter
// -> master. A service thread loads programs and prepares immutable graphs.
// Audio exclusively owns live source instances. UI edits use a bounded queue.
#include "framework/mpc_plugin.h"
#include "component_host.h"
#include "compact_pages.h"
#include "realtime_queue.h"

#include "rhythmer/engine/kit_output_stage.h"
#include "rhythmer/plugin/rate_bridge.h"
#include <dsp/svf.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <chrono>
namespace re {
namespace fs=std::filesystem;
class Cell {
 public:
 int origin=0,source=0,preset=0,note=60;std::atomic<bool>drum{false};bool pressure=false,silenced=false;int nativePitch=-1;std::vector<int>persist,voiceOverrides;bool coupledVoice=false;
 std::unique_ptr<Core>core;std::unique_ptr<std::atomic<float>[]>values;int count=0;
 std::vector<std::string>names;rhythmer::PluginRateBridge pitch;double pitchRatio=1;
 std::array<bool,128>held{};
 Cell(const Json&cat,int origin,int program,const std::string&folder):origin(origin),source(actualSource(cat,origin,program)),preset(program){
  auto&s=cat["sources"][source];auto&sound=cat["sources"][origin]["sounds"][program];
  coupledVoice=s.value("name","")=="Prophet";
  core=std::make_unique<Core>(folder,source);core->apply(s,sound);core->reference(s,cat["sources"][origin]["program_states"][program]);count=core->count();values=std::make_unique<std::atomic<float>[]>(count);
  for(int i=0;i<count;i++){values[i].store(core->get(i));names.push_back(core->name(i));}
  note=sound.value("note",60);drum=s.value("drums",false);pressure=s.value("velocity_pressure",false);
  for(const auto&p:s["parameters"]){auto key=p["key"].get<std::string>();if(!p.value("is_program_selector",false)&&!p.value("hidden_in_sample",false)&&key!="pad"&&key!="focused_pad"&&key!="ui_current_pad"&&key!="ui_focus_level"&&!(s.value("name","")=="TR-MPC"&&(key.rfind("e_",0)==0||key.rfind("edit_",0)==0||key.rfind("rnd_",0)==0)))persist.push_back(p["index"]);}
  if(coupledVoice)for(const auto&p:s["parameters"]){auto n=p.value("name","");if(n=="VCE"||n=="PRIO"||n=="ALLOC"||n=="DTUNE")voiceOverrides.push_back(p["index"]);}
  nativePitch=s.value("native_pitch_parameter",-1);int pitchControl=nativePitch;if(pitchControl>=0&&pitchControl<count)drum=values[pitchControl].load()<.5;
  pitch.reset(44100,44100,true,false);
 }
 void refreshModel(){float l[128]{},r[128]{};core->render(l,r,128);for(int i=0;i<count;i++){values[i]=core->get(i);names[i]=core->name(i);}}
 void set(int p,float v){if(p>=0&&p<count){core->set(p,v);float actual=core->get(p);values[p].store(actual,std::memory_order_relaxed);if(p==nativePitch)drum=actual<.5f;}}
 void syncValues(){for(int i:persist)if(i<count)values[i].store(core->get(i),std::memory_order_relaxed);}
 void copySound(const Cell&other){for(int index:other.persist)if(std::fabs(core->get(index)-other.values[index].load())>1e-6f)set(index,other.values[index].load());for(int index:voiceOverrides)if(std::fabs(core->get(index)-other.values[index].load())>1e-6f)set(index,other.values[index].load());}
 void midi(uint8_t status,uint8_t key,uint8_t vel,bool kit){
  auto hi=status&0xf0;if(hi==0x90&&vel)silenced=false;if(hi==0xb0&&key==120)silenced=true;
  if(hi==0xb0&&(key==120||key==123))held.fill(false);
  if(drum&&!kit&&(hi==0x90||hi==0x80)){
   bool on=hi==0x90&&vel;held[key]=on;
   if(on){double ratio=std::pow(2.,std::clamp(int(key)-60,-36,36)/12.);if(std::fabs(ratio-pitchRatio)>1e-7){pitchRatio=ratio;pitch.reset(unsigned(std::lround(44100*ratio)),44100,true,false,true);}}
   else if(std::any_of(held.begin(),held.end(),[](bool x){return x;}))return;
  }
  if((hi==0x90||hi==0x80)&&(drum||kit))key=note;
  if(pressure&&hi==0x90&&vel)core->midi(0xd0,vel,0);
  core->midi(status,key,vel);
 }
 void render(float*l,float*r,unsigned n,bool kit){
  if(drum&&!kit&&pitchRatio!=1)pitch.render(l,r,n,[&](float*a,float*b,unsigned k){core->render(a,b,k);});
  else core->render(l,r,n);
  if(silenced){std::fill(l,l+n,0);std::fill(r,r+n,0);}
 }
};
struct Graph {
 unsigned generation=1;int entry=0,program=0,kit=0;float kitGain=1;std::string kitName;
 bool isKit=false,filter=false;std::string warning;
 std::vector<std::shared_ptr<Cell>>cells;
 std::array<mpc::dsp::SVF,2>filters;rhythmer::KitOutputStage kitOutput;
 std::array<std::atomic<float>,3>common{};
 Graph(){common[0]=1;common[1]=0;common[2]=0;for(auto&f:filters)f.setSampleRate(44100);}
 void midi(uint8_t status,uint8_t key,uint8_t vel,int root=36){
  if(isKit){if((status&0xf0)==0x90||(status&0xf0)==0x80){int p=int(key)-root;if(p<0||p>=int(cells.size()))p=int(key)-36;if(p>=0&&p<int(cells.size()))cells[p]->midi(status,key,vel,true);}else for(auto&c:cells)c->midi(status,key,vel,true);}
  else if(!cells.empty())cells[0]->midi(status,key,vel,false);
 }
 void render(float*l,float*r,unsigned n,double bpm,double ppq,bool playing){
  std::fill(l,l+n,0);std::fill(r,r+n,0);
  {float a[128]{},b[128]{};for(auto&c:cells){std::fill(a,a+n,0);std::fill(b,b+n,0);c->core->tempo(bpm,ppq,playing);c->render(a,b,n,isKit);for(unsigned i=0;i<n;i++){l[i]+=a[i];r[i]+=b[i];}}}
  float cutoff=common[0].load(),res=common[1].load();int type=std::lround(common[2].load()*2);
  if(filter&&(cutoff<.9999f||res>.0001f||type!=0)){float hz=20*std::pow(1000.f,cutoff);for(auto&f:filters)f.set(hz,std::min(res,.95f));for(unsigned i=0;i<n;i++){l[i]=filters[0].process(l[i],type);r[i]=filters[1].process(r[i],type);}}
  if(isKit){if(kitGain!=1)for(unsigned i=0;i<n;i++){l[i]*=kitGain;r[i]*=kitGain;}kitOutput.process(l,r,n);}
 }
 Json snapshot(){Json x={{"kit_name",kitName},{"kit_gain",kitGain},{"entry",entry},{"program",program},{"kit",kit},{"common",{common[0].load(),common[1].load(),common[2].load()}},{"cells",Json::array()}};for(auto&c:cells){Json values=Json::array();for(int i=0;i<c->count;i++)values.push_back(nullptr);for(int i:c->persist)if(i<c->count&&!c->names[i].empty()&&c->names[i].rfind("Unused",0)!=0&&c->names[i].rfind("Inactive Parameter",0)!=0)values[i]=c->values[i].load();x["cells"].push_back({{"source",c->source},{"origin",c->origin},{"preset",c->preset},{"values",values},{"native_state",c->core->state()}});}return x;}
};
struct Visible {int cell=0,index=-1;Json metadata;std::string name,group;};
constexpr int ControlSlots=40,NewControls=89,NewPages=113,KitRoot=116,OpenBrowser=117,BrowserPage=124,BrowserClose=125,BrowserRow=126,BrowserText=142,BrowserTail=158,BrowserPrev=174,BrowserNext=175,BrowserTitle=176,BrowserVisible=177,BrowserInfo=178,TotalParameters=185;
inline int controlParameter(int slot){return slot<16?6+slot:NewControls+slot-16;}
inline int controlSlot(int parameter){return parameter>=6&&parameter<=21?parameter-6:parameter>=NewControls&&parameter<NewPages?parameter-NewControls+16:-1;}
struct View {std::array<std::string,ControlSlots>names{},text{};std::array<int,3>sectionPages{1,1,1},sectionPage{};std::array<std::string,3>sectionGroup{};std::vector<std::string>presetNames;std::vector<std::string>padSoundNames;std::array<std::string,16>padLabels{};std::string program,group,status="Loading";int pages=1,page=0;};
class RhythmerEngines:public mpc::Plugin {
#include "desktop_rate.inc"
 int padLabelsBase_=73;
 enum P{Master,Engine,Program,Pad,Family,Page,Main0,Main1,Main2,Main3,Main4,Main5,Main6,Main7,Detail0,Detail1,Detail2,Detail3,Detail4,Detail5,Detail6,Detail7,KitPreset,PadSource,PadSound,Visual,Status,Panic,Count};
 struct FactoryProgram { int entry,preset;std::string name; };
 std::vector<FactoryProgram>factoryPrograms_;std::array<std::atomic<bool>,TotalParameters>hostDirty_{};
 struct BrowserView {std::array<std::string,16>rows{};std::string title;int page=0,pages=1,count=0;};BrowserView browserView_;std::mutex browserMutex_;std::atomic<int>browserKind_{0},browserPick_{-1},browserStep_{0};std::atomic<unsigned>browserRevision_{1};unsigned browserShown_=0;
 Json catalog_;std::vector<int>presetParams_;std::vector<int>padSoundParams_;std::vector<int>sourcePicker_;std::string coresFolder_;
 std::atomic<Graph*>active_{nullptr},hazard_{nullptr};std::vector<std::unique_ptr<Graph>>graphs_;
 std::atomic<float>motionPhase_{0};std::atomic<bool>quit_{false};std::thread worker_;std::mutex viewMutex_,restoreMutex_,graphsMutex_;View view_;Json restore_;bool restorePending_=false;
 std::atomic<int>requestedSource_{-1},requestedSound_{-1},requestedPad_{0};std::atomic<unsigned>contextRev_{1},layoutRev_{1};unsigned generation_=0;
 std::atomic<uint64_t>reloadBinding_{0};std::atomic<float>reloadValue_{0};std::array<std::atomic<bool>,ControlSlots>reloadControl_{};std::array<std::atomic<uint64_t>,ControlSlots>bindings_{};std::array<std::atomic<unsigned>,ControlSlots>editSequence_{},appliedSequence_{};std::array<std::atomic<float>,ControlSlots>editValue_{};std::atomic<unsigned>editClock_{0},completedRevision_{0};Edits edits_;inline static thread_local bool internal_=false;
 bool transportPlaying_=false;double freePpq_=0;std::array<uint8_t,128>heldVelocity_{};float envelope_=0,masterGain_=.75f,motionPose_=0;int lastNote_=60,lastVelocity_=0;std::atomic<bool>resetRequested_{false},panicRequested_{false},loadingRestore_{false};std::atomic<unsigned>completedLayout_{0};std::atomic<bool>restoringParameters_{false};
 int sourcePickerIndex(int actual)const{auto i=std::find(sourcePicker_.begin(),sourcePicker_.end(),actual);return i==sourcePicker_.end()?0:int(i-sourcePicker_.begin());}
 static mpc::PluginInfo info(){mpc::PluginInfo i;i.name="Rhythmer Engines";i.vendor="Jazzy Rhythmer";i.uniqueId=MPC_FOURCC('J','F','q','7');i.version=300;i.isInstrument=true;i.numInputs=0;i.wantsMidi=true;return i;}
 void addUI(mpc::Param p){p.automatable=false;addParam(p);}
 void quietSet(int p,float v){float before=getNormalised(p);bool old=internal_;internal_=true;set(p,v);internal_=old;if(before!=getNormalised(p)&&p<int(hostDirty_.size()))hostDirty_[p]=true;}
 void quietNorm(int p,float v){float before=getNormalised(p);bool old=internal_;internal_=true;setNormalised(p,v);internal_=old;if(before!=getNormalised(p)&&p<int(hostDirty_.size()))hostDirty_[p]=true;}
 int bounded(int p,int n){return n?((getInt(p)%n)+n)%n:0;}
 std::shared_ptr<Cell>cell(int source,int preset){const auto&s=catalog_["sources"][source];preset=(preset%int(s["sounds"].size())+int(s["sounds"].size()))%int(s["sounds"].size());return std::make_shared<Cell>(catalog_,source,preset,coresFolder_);}
 std::unique_ptr<Graph>build(const Json&restored=Json()){
  auto g=std::make_unique<Graph>();g->generation=++generation_;g->entry=bounded(Engine,catalog_["entries"].size());const auto&e=catalog_["entries"][g->entry];
  g->isKit=e["kind"]=="kit";
  if(g->isKit){g->kit=bounded(KitPreset,catalog_["kits"].size());g->kitName=catalog_["kits"][g->kit]["name"].get<std::string>();g->kitGain=catalog_["kits"][g->kit].value("mix_gain",1.f);auto*old=active_.load();
   if(auto token=reloadBinding_.load();token&&old&&old->isKit&&old->kit==g->kit&&unsigned(token>>32)==old->generation){g->cells=old->cells;int c=(token>>16)&0xffff,p=int(token&0xffff)-4;auto previous=g->cells[c];auto fresh=cell(previous->origin,previous->preset);fresh->copySound(*previous);fresh->set(p,reloadValue_.load());fresh->refreshModel();g->cells[c]=fresh;for(int i=0;i<3;i++)g->common[i]=old->common[i].load();}
   else if(restored.is_object()&&restored.value("entry",-1)==g->entry&&restored.contains("cells")&&restored["cells"].size()==16){for(const auto&c:restored["cells"])g->cells.push_back(cell(c.value("origin",c.value("source",0)),c.value("preset",0)));}
   else if(old&&old->isKit&&old->kit==g->kit&&(requestedSource_.load()>=0||requestedSound_.load()>=0)){
    g->cells=old->cells;int p=std::clamp(requestedPad_.load(),0,15);int source=requestedSource_.load()>=0?requestedSource_.load():g->cells[p]->origin;int sound=requestedSound_.load()>=0?requestedSound_.load():0;g->cells[p]=cell(source,sound);for(int i=0;i<3;i++)g->common[i]=old->common[i].load();
   }else for(const auto&r:catalog_["kits"][g->kit]["routes"]){auto c=cell(r["engine"],r["sound"]);if(r.contains("mix_values"))for(auto it=r["mix_values"].begin();it!=r["mix_values"].end();++it)c->set(std::stoi(it.key()),it.value().get<float>());g->cells.push_back(c);}if(restored.is_object())g->kitGain=restored.value("kit_gain",1.f);else if(old&&old->isKit&&old->kit==g->kit&&(requestedSource_.load()>=0||requestedSound_.load()>=0||reloadBinding_.load()))g->kitGain=old->kitGain;g->filter=true;if(!std::isfinite(g->kitGain)||g->kitGain<=0||g->kitGain>128)throw std::runtime_error("Invalid kit gain");}
  else if(auto token=reloadBinding_.load();token&&active_.load()&&unsigned(token>>32)==active_.load()->generation&&active_.load()->entry==g->entry){
   auto*old=active_.load();g->cells=old->cells;g->program=old->program;g->filter=old->filter;int c=(token>>16)&0xffff,p=int(token&0xffff)-4;if(c<0||c>=int(g->cells.size()))throw std::runtime_error("Invalid model binding");auto previous=g->cells[c];auto fresh=cell(previous->origin,previous->preset);
   fresh->copySound(*previous);fresh->set(p,reloadValue_.load());fresh->refreshModel();g->cells[c]=fresh;for(int i=0;i<3;i++)g->common[i]=old->common[i].load();
  }
  else{int source=e["source_index"];g->program=bounded(Program,catalog_["sources"][source]["sounds"].size());g->cells.push_back(cell(source,g->program));g->filter=catalog_["sources"][g->cells[0]->source].value("output_filter",false);}
  return g;
 }
 std::string label(const Graph&g,const Cell&c,const Json&p){std::string n=p.value("label",p.value("name",p.value("key","")));int index=p["index"];
  if(c.drum&&index<int(c.names.size())&&!c.names[index].empty())n=c.names[index];
  if(p.contains("dynamic_labels")){int sel=p["dynamic_selector"];if(sel>=0&&sel<c.count){const auto&defs=catalog_["sources"][c.source]["parameters"];auto it=std::find_if(defs.begin(),defs.end(),[&](auto&x){return x["index"]==sel;});if(it!=defs.end()){int k=std::lround((*it)["minimum"].template get<float>()+c.values[sel].load()*((*it)["maximum"].template get<float>()-(*it)["minimum"].template get<float>()));if(k>=0&&k<int(p["dynamic_labels"].size()))n=p["dynamic_labels"][k];}}}
  if(c.drum){for(const char*prefix:{"pad","voice","p","v"}){size_t size=std::strlen(prefix);if(n.size()<=size)continue;bool match=true;for(size_t i=0;i<size;i++)if(std::tolower(static_cast<unsigned char>(n[i]))!=prefix[i])match=false;if(!match)continue;size_t at=size;while(at<n.size()&&n[at]==' ')at++;size_t digits=at;while(at<n.size()&&std::isdigit(static_cast<unsigned char>(n[at])))at++;if(at==digits)continue;size_t separator=at;while(at<n.size()&&(n[at]==' '||n[at]=='_'||n[at]=='-'||n[at]==':'||n[at]=='/'))at++;if(at>separator&&at<n.size())n=n.substr(at);break;}}
  return n;
 }
 std::vector<Visible>visible(Graph&g){
  std::vector<Visible>v;int focus=g.isKit?std::clamp(getInt(Pad)-1,0,15):0;
  if(g.filter)for(int i=0;i<3;i++)v.push_back({focus,-1-i,Json{{"minimum",0},{"maximum",i==2?2:100},{"options",i==2?Json::array({"LP","BP","HP"}):Json::array()}},std::string(g.isKit?"Kit ":"Output ")+std::array<std::string,3>{"Cutoff","Resonance","Type"}[i],"FILTER"});
  auto&c=*g.cells[focus];const auto&s=catalog_["sources"][c.source];const Json*sound=nullptr;
  if(g.isKit)sound=&s["sounds"][std::min(c.preset,int(s["sounds"].size())-1)];else{int origin=catalog_["entries"][g.entry]["source_index"];sound=&catalog_["sources"][origin]["sounds"][g.program];}
  for(const auto&p:s["parameters"]){
   auto key=p["key"].get<std::string>();int index=p["index"];auto n=label(g,c,p);
   if(p.value("hidden_in_sample",false)||p.value("is_program_selector",false)||n.empty()||std::all_of(n.begin(),n.end(),[](unsigned char ch){return ch=='-'||std::isspace(ch);})||n.find("Unused")==0||n.find("Inactive Parameter")==0)continue;
   if(key=="pad"||key=="focused_pad"||key=="ui_current_pad"||key=="ui_focus_level")continue;
   if(sound->contains("native_key")&&(*sound)["native_key"]==key)continue;
   Json allowed=sound->value("parameter_indices",Json());if(allowed.is_null()&&s.contains("voice_parameters"))allowed=s["voice_parameters"].value(std::to_string(c.note),Json());if(allowed.is_array()&&std::find(allowed.begin(),allowed.end(),index)==allowed.end())continue;
   char prefix[8];std::snprintf(prefix,sizeof(prefix),"p%02d_",c.note-35);if(c.drum&&key.size()>3&&key[0]=='p'&&std::isdigit(key[1])&&std::isdigit(key[2])&&key[3]=='_'&&key.rfind(prefix,0)!=0)continue;
   if(p.contains("active_models")){std::string selector=s.value("model_control_prefix","")+std::to_string(c.note-35);for(const auto&m:s["parameters"])if(m["key"]==selector){int mi=m["index"];int value=std::lround(m["minimum"].get<float>()+c.values[mi].load()*(m["maximum"].get<float>()-m["minimum"].get<float>()));if(std::find(p["active_models"].begin(),p["active_models"].end(),value)==p["active_models"].end())n.clear();}}
   if(n.empty())continue;
   if(g.isKit&&(n=="Volume"||n=="Master Volume"))n="Output Level";
   v.push_back({focus,index,p,(g.isKit?"P"+std::to_string(focus+1)+" ":"")+n,p.value("group","SOUND")});
  }
  std::stable_sort(v.begin(),v.end(),[](auto&a,auto&b){return a.metadata.value("native_ui_order",a.index<0?-100:1000)<b.metadata.value("native_ui_order",b.index<0?-100:1000);});return v;
 }
 void layout(Graph&g){
  auto controls=visible(g);std::vector<std::vector<Visible>>pages;std::string group;std::vector<Visible>*current=nullptr;
  const char*section=getInt(Family)==1?"SOUND":getInt(Family)==2?"MOD":getInt(Family)==3?"FX":"ALL";
#if defined(__APPLE__) || defined(_WIN32)
  pages=compactControlPages(controls,section);
#else
  for(auto&p:controls){if(std::string(section)!="ALL"&&p.metadata.value("section","SOUND")!=section)continue;if(!current||group!=p.group||current->size()==8){pages.emplace_back();current=&pages.back();group=p.group;}current->push_back(p);}
#endif
  int page=bounded(Page,pages.size());quietSet(Page,page);
  std::vector<Visible>main;auto add=[&](const Visible&p){if(main.size()<8&&std::none_of(main.begin(),main.end(),[&](auto&x){return x.index==p.index;}))main.push_back(p);};
  for(auto&p:controls)if(p.index<0&&p.index>=-2)add(p);
  if(!g.cells.empty()){
   const auto&s=catalog_["sources"][g.cells[g.isKit?std::clamp(getInt(Pad)-1,0,15):0]->source];
   if(g.isKit)for(auto role:{"volume","tune","decay","pan"})for(auto&p:controls)if(p.metadata.value("kit_mix_role","")==role)add(p);
   if(s.contains("primary_candidates"))for(const auto&key:s["primary_candidates"])for(auto&p:controls)if(p.metadata.value("key","")==key.get<std::string>())add(p);
  }
  for(auto&p:controls)add(p);
  std::array<std::vector<std::vector<Visible>>,3>sectionLists;std::array<std::string,3>lastGroup;
  for(const auto&control:controls){std::string group=control.group;std::transform(group.begin(),group.end(),group.begin(),[](unsigned char c){return char(std::toupper(c));});auto section=control.metadata.value("section",std::string("SOUND"));int bank=group.find("MODEL")!=std::string::npos?0:section=="FX"?2:(section=="MOD"||group.find("ENV")!=std::string::npos||group.find("ADSR")!=std::string::npos||group.find("LFO")!=std::string::npos)?1:0;
   auto&bucket=sectionLists[bank];if(bucket.empty()||lastGroup[bank]!=control.group||bucket.back().size()==8){bucket.emplace_back();lastGroup[bank]=control.group;}bucket.back().push_back(control);}
  View next;next.page=page;
  for(int bank=0;bank<3;bank++){auto&bucket=sectionLists[bank];next.sectionPages[bank]=std::max(1,int(bucket.size()));next.sectionPage[bank]=bounded(NewPages+bank,next.sectionPages[bank]);quietSet(NewPages+bank,next.sectionPage[bank]);next.sectionGroup[bank]=bucket.empty()?std::array<std::string,3>{"No sound controls","No modulation controls","No FX controls"}[bank]:bucket[next.sectionPage[bank]][0].group;}
if(g.isKit){for(const auto&k:catalog_["kits"])next.presetNames.push_back(k["name"]);}else{int origin=catalog_["entries"][g.entry]["source_index"];for(const auto&p:catalog_["sources"][origin]["sounds"])next.presetNames.push_back(p["name"]);}
  if(g.isKit)for(int i=0;i<16;i++){auto&c=*g.cells[i];next.padLabels[i]=catalog_["sources"][c.origin]["sounds"][c.preset]["name"].get<std::string>();}
  next.pages=std::max(1,int(pages.size()));next.group=pages.empty()?std::string("No ")+section+" controls":compactPageTitle(pages[page]);next.status=g.warning.empty()?"READY":g.warning;
  if(g.isKit)next.program=catalog_["kits"][g.kit]["name"];else{int origin=catalog_["entries"][g.entry]["source_index"];next.program=catalog_["sources"][origin]["sounds"][g.program]["name"];}
  for(int slot=0;slot<ControlSlots;slot++){
   const std::vector<Visible>empty;int bank=(slot-16)/8;const auto&list=slot<8?main:slot<16?(pages.empty()?empty:pages[page]):(sectionLists[bank].empty()?empty:sectionLists[bank][next.sectionPage[bank]]);int at=slot%8;
#if defined(__APPLE__) || defined(_WIN32)
   const auto&desktopList=(slot>=16&&slot<24)?(pages.empty()?empty:pages[page]):list;
   if(slot>=16&&slot<24)at+=8;
#else
   const auto&desktopList=list;
#endif
   if(at<int(desktopList.size())){const auto&p=desktopList[at];uint64_t token=(uint64_t(g.generation)<<32)|(uint64_t(p.cell)<<16)|uint16_t(p.index+4);if(bindings_[slot].load()!=token)editSequence_[slot]=0;bindings_[slot].store(token,std::memory_order_release);
    bool rebuild=false;if(p.index>=0){auto key=p.metadata.value("key","");rebuild=key.find("model")!=std::string::npos||p.name.find("Model")!=std::string::npos||p.name=="Machine"||p.name.find(" Machine")!=std::string::npos;for(const auto&other:catalog_["sources"][g.cells[p.cell]->source]["parameters"])if(other.value("dynamic_selector",-1)==p.index)rebuild=true;}reloadControl_[slot]=rebuild;
    next.names[slot]=p.name;
    float val=p.index<0?g.common[-p.index-1].load():g.cells[p.cell]->values[p.index].load();quietNorm(controlParameter(slot),val);
    float lo=p.metadata.value("minimum",0.f),hi=p.metadata.value("maximum",1.f);float plain=lo+val*(hi-lo);if(std::abs(plain)<.00001f)plain=0;if(p.metadata.value("type",std::string())=="int")plain=std::round(plain);if(p.metadata.contains("options")&&!p.metadata["options"].empty()){int opt=std::clamp(int(std::lround(plain-lo)),0,int(p.metadata["options"].size())-1);next.text[slot]=p.metadata["options"][opt].is_string()?p.metadata["options"][opt].get<std::string>():p.metadata["options"][opt].dump();}else{char t[64];if(p.metadata.value("display_native_percent",false))std::snprintf(t,sizeof(t),"%.0f %%",plain*100);else if(p.metadata.value("display_percentage",false))std::snprintf(t,sizeof(t),"%.0f %%",val*100);else std::snprintf(t,sizeof(t),"%.3g%s%s",plain,p.metadata.value("unit","").empty()?"":" ",p.metadata.value("unit","").c_str());next.text[slot]=t;}
   }else{bindings_[slot]=0;next.names[slot]="";next.text[slot]="";quietNorm(controlParameter(slot),0);}
  }
if(presetParams_[g.entry]>=0)quietSet(presetParams_[g.entry],g.program);
  if(g.isKit){auto&c=*g.cells[std::clamp(getInt(Pad)-1,0,15)];quietSet(PadSource,sourcePickerIndex(c.origin));quietSet(PadSound,c.preset);quietSet(padSoundParams_[c.origin],c.preset);for(const auto&sound:catalog_["sources"][c.origin]["sounds"])next.padSoundNames.push_back(sound["name"]);}
  bool namesChanged=false;{std::lock_guard<std::mutex>lock(viewMutex_);namesChanged=view_.sectionPage!=next.sectionPage||view_.sectionGroup!=next.sectionGroup||view_.page!=next.page||view_.names!=next.names||view_.program!=next.program||view_.group!=next.group||view_.status!=next.status;
   for(int i=0;i<ControlSlots;i++)if(view_.text[i]!=next.text[i]||view_.names[i]!=next.names[i])hostDirty_[controlParameter(i)]=true;for(int i=0;i<16;i++)if(view_.padLabels[i]!=next.padLabels[i])hostDirty_[padLabelsBase_+i]=true;for(int b=0;b<3;b++)if(view_.sectionPage[b]!=next.sectionPage[b]||view_.sectionPages[b]!=next.sectionPages[b]||view_.sectionGroup[b]!=next.sectionGroup[b])hostDirty_[NewPages+b]=true;
   if(view_.program!=next.program)hostDirty_[Program]=true;if(view_.page!=next.page||view_.group!=next.group||view_.pages!=next.pages)hostDirty_[Page]=true;view_=std::move(next);}
  quietSet(Status,g.warning.empty()?0:1);if(namesChanged)requestDisplayUpdate();
 }
 void restoreGraph(Graph&g,const Json&state){if(state.value("entry",-1)!=g.entry)return;
  if(state.contains("common"))for(int i=0;i<3;i++)g.common[i]=state["common"][i].get<float>();
  if(state.contains("cells"))for(size_t k=0;k<g.cells.size()&&k<state["cells"].size();k++){auto&c=*g.cells[k];const auto&s=state["cells"][k];if(s.value("source",-1)!=c.source)continue;
   // Native state carries layered/model-specific data that the currently visible
   // normalized control bank cannot represent (for example all Amino layers).
   if(s.contains("native_state")&&s["native_state"].is_array()&&s["native_state"].size()<=32*1024*1024)c.core->restore(s["native_state"].get<std::vector<uint8_t>>());
   // Persist only canonical sound controls, never focus-dependent helper aliases.
   for(const auto&p:catalog_["sources"][c.source]["parameters"]){int i=p["index"];if(std::find(c.persist.begin(),c.persist.end(),i)!=c.persist.end()&&i<c.count&&i<int(s["values"].size())&&!s["values"][i].is_null()&&std::fabs(c.core->get(i)-s["values"][i].get<float>())>1e-6f)c.set(i,s["values"][i]);}
   // Original Prophet native patch edits accept their allocator defaults. Apply
   // saved track overrides last, after all native patch controls are restored.
   for(int i:c.voiceOverrides)if(i<int(s["values"].size())&&!s["values"][i].is_null()&&std::fabs(c.core->get(i)-s["values"][i].get<float>())>1e-6f)c.set(i,s["values"][i]);c.refreshModel();}

 }
 void closeBrowser(){browserKind_=0;quietSet(BrowserVisible,0);for(int i=0;i<7;i++)quietSet(OpenBrowser+i,0);browserRevision_++;layoutRev_++;}
 std::vector<std::string>browserChoices(int kind){std::vector<std::string>rows;int engine=bounded(Engine,catalog_["entries"].size());
  if(kind==1)for(const auto&e:catalog_["entries"])rows.push_back(e["name"]);
  if(kind==2||kind==3){if(kind==3||engine==0)for(const auto&k:catalog_["kits"])rows.push_back(k["name"]);else{int source=catalog_["entries"][engine]["source_index"];for(const auto&p:catalog_["sources"][source]["sounds"])rows.push_back(p["name"]);}}
  if(kind==4){std::lock_guard<std::mutex>l(viewMutex_);for(int i=0;i<16;i++){char n[8];std::snprintf(n,sizeof(n),"P%02d",i+1);rows.push_back(std::string(n)+" / "+view_.padLabels[i]);}}
  if(kind==5)for(int source:sourcePicker_)rows.push_back(catalog_["sources"][source]["name"].get<std::string>()+(source==8?" / DRUMS":""));
  if(kind==6){int source=sourcePicker_[bounded(PadSource,sourcePicker_.size())];for(const auto&p:catalog_["sources"][source]["sounds"])rows.push_back(p["name"]);}
  if(kind==7)for(int i=0;i<=112;i++)rows.push_back(std::to_string(i)+" / "+std::array<std::string,12>{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}[i%12]);return rows;
 }
 void serviceBrowser(){int kind=browserKind_.load();if(!kind)return;if(browserShown_==browserRevision_.load()&&browserStep_.load()==0&&browserPick_.load()<0)return;auto rows=browserChoices(kind);int pages=std::max(1,(int(rows.size())+15)/16);int step=browserStep_.exchange(0);if(step){quietSet(BrowserPage,std::clamp(getInt(BrowserPage)+step,0,pages-1));browserRevision_++;}
  int pick=browserPick_.exchange(-1);if(pick>=0){if(pick<int(rows.size())){int engine=bounded(Engine,catalog_["entries"].size());if(kind==1)setProgram(pick);else if(kind==2)set(engine==0?KitPreset:presetParams_[engine],pick);else if(kind==3)set(KitPreset,pick);else if(kind==4)set(Pad,pick+1);else if(kind==5)set(PadSource,pick);else if(kind==6)set(padSoundParams_[sourcePicker_[bounded(PadSource,sourcePicker_.size())]],pick);else if(kind==7)set(KitRoot,pick);}closeBrowser();return;}
  auto revision=browserRevision_.load();if(revision==browserShown_)return;int page=std::clamp(getInt(BrowserPage),0,pages-1);quietSet(BrowserPage,page);BrowserView next;next.page=page;next.pages=pages;next.count=int(rows.size());next.title=std::array<std::string,7>{"ENGINES","SOUNDS / KITS","KITS","PADS","PAD ENGINES","PAD SOUNDS","MIDI ROOT"}[kind-1];if(kind==2)next.title=catalog_["entries"][bounded(Engine,catalog_["entries"].size())]["name"].get<std::string>()+" / SOUNDS";
  for(int i=0;i<16;i++){int at=page*16+i;if(at<int(rows.size()))next.rows[i]=rows[at];}
  {std::lock_guard<std::mutex>l(browserMutex_);browserView_=std::move(next);}quietSet(BrowserVisible,1);hostDirty_[BrowserTitle]=true;hostDirty_[BrowserInfo]=true;for(int i=0;i<16;i++){hostDirty_[BrowserRow+i]=true;hostDirty_[BrowserText+i]=true;hostDirty_[BrowserTail+i]=true;}browserShown_=revision;requestDisplayUpdate();
 }
 void service(){unsigned context=completedRevision_.load(),layoutVersion=0;auto nextPublish=std::chrono::steady_clock::now();int motionFrame=-1;
  while(!quit_.load()){
   try{serviceBrowser();auto revision=contextRev_.load();if(revision!=context){Json restored;{std::lock_guard<std::mutex>l(restoreMutex_);if(restorePending_)restored=restore_;}auto g=build(restored);if(restored.is_object()){restoreGraph(*g,restored);std::lock_guard<std::mutex>l(restoreMutex_);if(revision==contextRev_.load())restorePending_=false;}
    if(revision==contextRev_.load()){auto*ptr=g.get();{std::lock_guard<std::mutex>lock(graphsMutex_);graphs_.push_back(std::move(g));active_.store(ptr,std::memory_order_release);}context=revision;reloadBinding_=0;requestedSource_=-1;requestedSound_=-1;layoutVersion=0;resetRequested_=true;}}
    auto*g=active_.load();auto lv=layoutRev_.load();if(g&&(lv!=layoutVersion||!layoutVersion)){layout(*g);layoutVersion=lv?lv:1;completedRevision_=context;completedLayout_=lv;loadingRestore_=false;}
    std::lock_guard<std::mutex>lock(graphsMutex_);auto*live=active_.load();auto*busy=hazard_.load();graphs_.erase(std::remove_if(graphs_.begin(),graphs_.end(),[&](auto&g){return g.get()!=live&&g.get()!=busy;}),graphs_.end());
   }catch(const std::exception&e){std::lock_guard<std::mutex>l(viewMutex_);view_.status=e.what();quietSet(Status,1);context=contextRev_.load();completedRevision_=context;loadingRestore_=false;}
   // MPC caches displayed parameter values. Publish dirty values off the audio
   // thread; animation updates must not rebuild the entire tab via UpdateDisplay.
   auto now=std::chrono::steady_clock::now();if(now>=nextPublish){nextPublish=now+std::chrono::milliseconds(40);
    for(int i=0;i<numParams();i++)if(hostDirty_[i].exchange(false)){if(i==Visual){int frame=std::clamp(int(getNormalised(i)*23),0,23);if(frame==motionFrame)continue;motionFrame=frame;}notifyHostParameterChanged(i);}
   }
   std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
 }
 public:
 RhythmerEngines():Plugin(info()){
  std::string catalogText;if(!mpc::readTextFile(mpc::contentPath("catalog.json"),catalogText))throw std::runtime_error("Missing Rhythmer catalog");catalog_=Json::parse(catalogText);
#if defined(__APPLE__) || defined(_WIN32)
  coresFolder_=mpc::contentPath(
#ifdef _WIN32
   "Engines/windows"
#else
   "Engines/mac"
#endif
  );
#else
  coresFolder_=mpc::contentPath(sizeof(void*)==4?"Engines/arm32":"Engines/arm64");
#endif
  std::vector<std::string>engines,kits,sources;for(auto&e:catalog_["entries"])engines.push_back(e["name"]);for(auto&k:catalog_["kits"])kits.push_back(k["name"]);for(int i=0;i<int(catalog_["sources"].size());i++)if(catalog_["sources"][i]["name"]!="Acid Engine"){sourcePicker_.push_back(i);sources.push_back(catalog_["sources"][i]["name"]);}
  addParam(mpc::floatParam("master","Level",0,1,.75));addParam(mpc::choiceParam("engine","Engine",engines,0));addParam(mpc::intParam("program","Preset",0,2047,0));addParam(mpc::intParam("pad","Pad",1,16,1));addParam(mpc::choiceParam("family","Section",{"ALL","SOUND","MOD","FX"}));addParam(mpc::intParam("page","Detail Page",0,511,0));
  for(int i=0;i<16;i++){auto p=mpc::floatParam(("control_"+std::to_string(i)).c_str(),("Control "+std::to_string(i+1)).c_str(),0,1,0);p.automatable=false;addParam(p);}
  addParam(mpc::choiceParam("kit","Kit Preset",kits));addParam(mpc::choiceParam("pad_source","Pad Engine",sources));addParam(mpc::intParam("pad_sound","Pad Sound",0,2047,0));auto visual=mpc::floatParam("visual","Motion",0,1,0);visual.automatable=false;addParam(visual);auto status=mpc::intParam("status","Status",0,1,0);status.automatable=false;addParam(status);auto panic=mpc::boolParam("panic","STOP SOUND",false);panic.automatable=false;addParam(panic);
  if(numParams()!=Count)throw std::runtime_error("Parameter schema mismatch");
  presetParams_.assign(catalog_["entries"].size(),-1);padSoundParams_.assign(catalog_["sources"].size(),-1);
  for(int i=0;i<std::min(20,int(catalog_["entries"].size()));i++)if(catalog_["entries"][i]["kind"]=="source"){int source=catalog_["entries"][i]["source_index"];std::vector<std::string>names;for(const auto&p:catalog_["sources"][source]["sounds"])names.push_back(p["name"]);auto id="preset_entry_"+std::to_string(i),label="Preset / "+catalog_["entries"][i]["name"].get<std::string>();auto p=mpc::choiceParam(id.c_str(),label.c_str(),names);p.automatable=false;presetParams_[i]=addParam(p);}
  for(int i:sourcePicker_)if(i<27){std::vector<std::string>names;for(const auto&p:catalog_["sources"][i]["sounds"])names.push_back(p["name"]);auto id="pad_sound_source_"+std::to_string(i),label="Pad Sound / "+catalog_["sources"][i]["name"].get<std::string>();auto p=mpc::choiceParam(id.c_str(),label.c_str(),names);p.automatable=false;padSoundParams_[i]=addParam(p);}
  padLabelsBase_=numParams();for(int i=0;i<16;i++){auto p=mpc::floatParam(("pad_label_"+std::to_string(i)).c_str(),("Pad "+std::to_string(i+1)+" Sound").c_str(),0,1,0);p.automatable=false;addParam(p);}
  for(int bank=0;bank<3;bank++)for(int slot=0;slot<8;slot++){std::string id=std::array<std::string,3>{"edit_sound_","edit_mod_","edit_fx_"}[bank]+std::to_string(slot),name=std::array<std::string,3>{"Sound Control ","Mod Control ","FX Control "}[bank]+std::to_string(slot+1);auto p=mpc::floatParam(id.c_str(),name.c_str(),0,1,0);p.automatable=false;addParam(p);}
  for(int bank=0;bank<3;bank++){auto id=std::array<const char*,3>{"sound_page","mod_page","fx_page"}[bank],name=std::array<const char*,3>{"Sound Page","Mod Page","FX Page"}[bank];auto p=mpc::intParam(id,name,0,511,0);p.automatable=false;addParam(p);}addUI(mpc::intParam("kit_midi_root","Kit MIDI Root",0,112,60));for(int i=0;i<7;i++){auto id="browser_open_"+std::to_string(i);auto p=mpc::boolParam(id.c_str(),"Open browser",false);p.automatable=false;addParam(p);}addUI(mpc::intParam("browser_page","Browser Page",0,127,0));addUI(mpc::boolParam("browser_close","Close browser",false));
  for(int i=0;i<16;i++){auto id="browser_pick_"+std::to_string(i);auto p=mpc::boolParam(id.c_str(),"Choose sound",false);p.automatable=false;addParam(p);}
  for(int part=0;part<2;part++)for(int i=0;i<16;i++){auto id="browser_label_"+std::to_string(part)+"_"+std::to_string(i);auto p=mpc::floatParam(id.c_str(),"Browser sound",0,1,0);p.automatable=false;addParam(p);}
  addUI(mpc::boolParam("browser_prev","Previous page",false));addUI(mpc::boolParam("browser_next","Next page",false));addUI(mpc::floatParam("browser_title","Browser title",0,1,0));addUI(mpc::boolParam("browser_visible","Browser visible",false));addUI(mpc::floatParam("browser_info","Browser information",0,1,0));
  // Keep all legacy parameter IDs (0..178). Append expansion selectors.
  for(int i=20;i<int(catalog_["entries"].size());++i){int source=catalog_["entries"][i]["source_index"];std::vector<std::string>names;for(const auto& sound:catalog_["sources"][source]["sounds"])names.push_back(sound["name"]);auto id="preset_entry_"+std::to_string(i),label="Preset / "+catalog_["entries"][i]["name"].get<std::string>();auto p=mpc::choiceParam(id.c_str(),label.c_str(),names);p.automatable=false;presetParams_[i]=addParam(p);}
  for(int i:sourcePicker_)if(i>=27){std::vector<std::string>names;for(const auto& sound:catalog_["sources"][i]["sounds"])names.push_back(sound["name"]);auto id="pad_sound_source_"+std::to_string(i),label="Pad Sound / "+catalog_["sources"][i]["name"].get<std::string>();auto p=mpc::choiceParam(id.c_str(),label.c_str(),names);p.automatable=false;padSoundParams_[i]=addParam(p);}
  if(numParams()!=TotalParameters)throw std::runtime_error("Extended parameter schema");
  // MPC's native PRESET selector chooses the instrument engine. Source sound
  // banks remain inside the skin; program count and names stay fixed at load.
  for(int entry=0;entry<int(catalog_["entries"].size());entry++)factoryPrograms_.push_back({entry,0,catalog_["entries"][entry]["name"].get<std::string>()});
  auto first=build();auto*initial=first.get();graphs_.push_back(std::move(first));active_.store(initial);completedRevision_=contextRev_.load();layout(*initial);completedLayout_=layoutRev_.load();worker_=std::thread([this]{service();});
 }
 ~RhythmerEngines()override{quit_=true;if(worker_.joinable())worker_.join();active_=nullptr;graphs_.clear();}
 void parameterChanged(int i,float v)override{if(internal_||restoringParameters_.load())return;
  if(i>=OpenBrowser&&i<OpenBrowser+7){if(v>=.5f){browserKind_=i-OpenBrowser+1;browserPick_=-1;quietSet(BrowserPage,0);browserRevision_++;}quietSet(i,0);return;}
  if(i==BrowserClose){if(v>=.5f)closeBrowser();quietSet(i,0);return;}if(i==BrowserPage){browserRevision_++;return;}
  if(i==BrowserPrev||i==BrowserNext){if(v>=.5f)browserStep_+=(i==BrowserPrev?-1:1);quietSet(i,0);return;}
  if(i>=BrowserRow&&i<BrowserRow+16){if(v>=.5f)browserPick_=getInt(BrowserPage)*16+i-BrowserRow;quietSet(i,0);return;}
  if(browserKind_.load()&&controlSlot(i)>=0)return;
  if(i==KitRoot){panicRequested_=true;return;}
  if(i==Panic){if(v>=.5f)panicRequested_=true;quietSet(Panic,0);return;}
  for(int entry=0;entry<int(presetParams_.size());entry++)if(i==presetParams_[entry]&&entry==getInt(Engine)){quietSet(Program,v);contextRev_++;return;}
  int selected=sourcePicker_[bounded(PadSource,sourcePicker_.size())];
  for(int source:sourcePicker_)if(i==padSoundParams_[source]&&source==selected&&getInt(Engine)==0){quietSet(PadSound,v);requestedPad_=std::clamp(getInt(Pad)-1,0,15);requestedSound_=int(std::lround(v));contextRev_++;return;}
  if(int slot=controlSlot(i);slot>=0){auto binding=bindings_[slot].load(std::memory_order_acquire);if(binding){editValue_[slot]=v;unsigned sequence=++editClock_;editSequence_[slot]=sequence;if(reloadControl_[slot].load()){reloadValue_=v;reloadBinding_=binding;contextRev_++;}else edits_.push({binding,v,sequence});}return;}
  if(i==PadSource||i==PadSound){requestedPad_=std::clamp(getInt(Pad)-1,0,15);if(i==PadSource){requestedSource_=selected;requestedSound_=0;}else requestedSound_=int(std::lround(v));contextRev_++;}
  if(i==Engine){closeBrowser();quietSet(Program,0);quietSet(Page,0);}
  if(i==Engine||i==Program||i==KitPreset){reloadBinding_=0;requestedSource_=-1;requestedSound_=-1;contextRev_++;}
  if(i==Pad||i==Family||i==Page||(i>=NewPages&&i<NewPages+3))layoutRev_++;
 }
 std::string parameterName(int i)override{if(i>=BrowserRow&&i<BrowserRow+16){std::lock_guard<std::mutex>l(browserMutex_);return std::to_string(browserView_.page*16+i-BrowserRow+1);}if(i==Page){std::lock_guard<std::mutex>l(viewMutex_);return std::to_string(getInt(Page)%std::max(1,view_.pages)+1)+"/"+std::to_string(view_.pages)+" "+view_.group;}if(i>=NewPages&&i<NewPages+3){std::lock_guard<std::mutex>l(viewMutex_);int b=i-NewPages;return std::to_string(view_.sectionPage[b]+1)+"/"+std::to_string(view_.sectionPages[b])+" "+view_.sectionGroup[b];}if(int slot=controlSlot(i);slot>=0){std::lock_guard<std::mutex>l(viewMutex_);return view_.names[slot].empty()?"Control (unused)":view_.names[slot];}return param(i).name;}
 bool formatValue(int i,float v,std::string&out)override{if((i>=BrowserText&&i<BrowserTail+16)||i==BrowserTitle||i==BrowserInfo){std::lock_guard<std::mutex>l(browserMutex_);if(i==BrowserTitle)out=browserView_.title;else if(i==BrowserInfo)out="PAGE "+std::to_string(browserView_.page+1)+" / "+std::to_string(browserView_.pages)+"   "+std::to_string(browserView_.count)+" ITEMS";else{int row=(i-BrowserText)%16,offset=i>=BrowserTail?32:0;out=browserView_.rows[row].size()>unsigned(offset)?browserView_.rows[row].substr(offset,32):"";}return true;}std::lock_guard<std::mutex>l(viewMutex_);if(i>=padLabelsBase_&&i<padLabelsBase_+16){out=view_.padLabels[i-padLabelsBase_];return true;}if(int slot=controlSlot(i);slot>=0){out=view_.text[slot];return true;}if(i>=NewPages&&i<NewPages+3){int b=i-NewPages;out=std::to_string(view_.sectionPage[b]+1)+"/"+std::to_string(view_.sectionPages[b])+" "+view_.sectionGroup[b];return true;}if(i==Program){out=view_.program;return true;}if(i==PadSound){out=view_.padSoundNames.empty()?"—":view_.padSoundNames[size_t(int(v))%view_.padSoundNames.size()];return true;}if(i==Page){out=std::to_string(int(v)%std::max(1,view_.pages)+1)+"/"+std::to_string(view_.pages)+" "+view_.group;return true;}if(i==Status){out=completedRevision_.load()!=contextRev_.load()?"LOADING":view_.status;return true;}return false;}
 int numPrograms()const override{return int(factoryPrograms_.size());}
 int currentProgram()const override{return std::clamp(getInt(Engine),0,numPrograms()-1);}
 const char*programName(int index)const override{return index>=0&&index<numPrograms()?factoryPrograms_[index].name.c_str():"";}
 void setProgram(int index)override{if(index<0||index>=numPrograms())return;closeBrowser();const auto&p=factoryPrograms_[index];quietSet(Engine,p.entry);quietSet(Program,p.entry==0?0:p.preset);if(p.entry==0)quietSet(KitPreset,0);quietSet(Page,0);reloadBinding_=0;requestedSource_=-1;requestedSound_=-1;contextRev_++;}
 void reset()override{resetRequested_=true;}
 void beginParameterRestore()override{restoringParameters_=true;}
 void endParameterRestore()override{restoringParameters_=false;closeBrowser();}
 void beginStateSave()override{for(int i=0;i<3000;i++){if(browserPick_.load()<0&&completedRevision_.load()==contextRev_.load()&&completedLayout_.load()==layoutRev_.load())break;std::this_thread::sleep_for(std::chrono::milliseconds(2));}}
 void saveExtraState(std::vector<uint8_t>&out)override{std::lock_guard<std::mutex>guard(graphsMutex_);auto*g=active_.load();Json s={{"schema",2}};if(g){s["graph"]=g->snapshot();std::array<int,ControlSlots>ordered{};for(int i=0;i<ControlSlots;i++)ordered[i]=i;std::sort(ordered.begin(),ordered.end(),[&](int a,int b){return editSequence_[a].load()<editSequence_[b].load();});
   for(int slot:ordered){if(editSequence_[slot].load()<=appliedSequence_[slot].load())continue;auto binding=bindings_[slot].load();if(unsigned(binding>>32)!=g->generation)continue;int c=(binding>>16)&0xffff,p=int(binding&0xffff)-4;float v=editValue_[slot].load();if(p<0&&p>=-3)s["graph"]["common"][-p-1]=v;else if(c<int(g->cells.size())&&p>=0&&p<g->cells[c]->count){
     // A host may save before the next audio block applies a knob edit.
     // Persist the source control's canonical step, rather than an arbitrary
     // normalised slider position that cannot survive an integer/enum restore.
     for(const auto&def:catalog_["sources"][g->cells[c]->source]["parameters"])if(def["index"]==p){auto type=def.value("type","");if(type=="int"||type=="enum"||type=="bool"){float lo=def.value("minimum",0.f),hi=def.value("maximum",1.f);if(hi>lo)v=std::clamp((std::round(lo+v*(hi-lo))-lo)/(hi-lo),0.f,1.f);}break;}
     s["graph"]["cells"][c]["values"][p]=v;
    }}
  }
  s["bindings"]=Json::array();for(auto&b:bindings_)s["bindings"].push_back(b.load());{std::lock_guard<std::mutex>l(viewMutex_);s["detail_pages"]=view_.pages;}auto text=s.dump();out.assign(text.begin(),text.end());
 }
 void loadExtraState(const uint8_t*d,size_t n)override{if(n>16*1024*1024)return;auto s=Json::parse(d,d+n,nullptr,false);if(!s.is_object()||(s.value("schema",0)!=1&&s.value("schema",0)!=2))return;std::lock_guard<std::mutex>l(restoreMutex_);reloadBinding_=0;requestedSource_=-1;requestedSound_=-1;restore_=s.value("graph",Json::object());if(s.value("schema",0)==1&&restore_.is_object()){int old=restore_.value("entry",0);if(old==1||old==2)restore_=Json{{"entry",0},{"kit",0},{"kit_name","K-WASUB / 909"}};else restore_["entry"]=old>=3?old-2:old;}
  if(restore_.is_object()&&restore_.contains("entry")){quietSet(Engine,restore_.value("entry",0));quietSet(Program,restore_.value("program",0));if(restore_.value("entry",-1)==0){int kit=restore_.value("kit",0);std::string name=restore_.value("kit_name",std::string());if(name.empty()&&catalog_.contains("kit_order_legacy")&&kit>=0&&kit<int(catalog_["kit_order_legacy"].size()))name=catalog_["kit_order_legacy"][kit].get<std::string>();if(!name.empty())for(int i=0;i<int(catalog_["kits"].size());i++)if(catalog_["kits"][i]["name"]==name){kit=i;break;}quietSet(KitPreset,kit);restore_["kit"]=kit;}}
  restorePending_=true;loadingRestore_=true;contextRev_++;
 }
 void prepare(double rate,int /*maxBlock*/)override{
#if defined(__APPLE__) || defined(_WIN32)
  desktopRate_.reset(44100,unsigned(std::lround(rate)),true);desktopMidiCount_=0;setLatency(int(desktopRate_.latency()));
#else
  (void)rate;
#endif
 }
 void process(mpc::ProcessContext&ctx)override{
#if defined(__APPLE__) || defined(_WIN32)
  if(desktopRate_.converting()){
   int midi=0;
   for(int frame=0;frame<ctx.numFrames;){
    while(midi<ctx.numMidiEvents&&ctx.midi[midi].sampleOffset<=frame){if(desktopMidiCount_<int(desktopMidi_.size())){desktopMidi_[desktopMidiCount_]=ctx.midi[midi];desktopMidi_[desktopMidiCount_++].sampleOffset=0;}midi++;}
    int n=std::min(128,ctx.numFrames-frame);if(midi<ctx.numMidiEvents)n=std::min(n,std::max(1,ctx.midi[midi].sampleOffset-frame));
    float* outputs[]={ctx.outputs[0]+frame,ctx.outputs[1]+frame};unsigned pulled=0;
    desktopRate_.render(outputs[0],outputs[1],unsigned(n),[&](float*l,float*r,unsigned count){
     auto inner=ctx;float* innerOut[]={l,r};inner.outputs=innerOut;inner.sampleRate=44100;inner.numFrames=int(count);inner.midi=desktopMidi_.data();inner.numMidiEvents=desktopMidiCount_;desktopMidiCount_=0;
     if(inner.transport.ppqValid)inner.transport.ppqPos+=double(frame)*inner.transport.bpm/(60*ctx.sampleRate)+double(pulled)*inner.transport.bpm/(60*44100.);pulled+=count;
     processEngine(inner);
    });frame+=n;
   }return;
  }
#endif
  processEngine(ctx);
 }
 void processEngine(mpc::ProcessContext&ctx){
  if(loadingRestore_.load()){for(int i=0;i<ctx.numMidiEvents;i++){const auto&e=ctx.midi[i];if(e.isNoteOn()){heldVelocity_[e.note()]=e.velocity();lastNote_=e.note();lastVelocity_=e.velocity();}if(e.isNoteOff())heldVelocity_[e.note()]=0;if(e.isAllNotesOff())heldVelocity_.fill(0);}return;}
  Graph*g=nullptr;do{g=active_.load(std::memory_order_acquire);hazard_.store(g,std::memory_order_seq_cst);}while(g!=active_.load(std::memory_order_acquire));if(!g){hazard_=nullptr;return;}
  Edit edit;bool changed=false;std::array<bool,16>changedCells{};while(edits_.pop(edit)){if(unsigned(edit.binding>>32)!=g->generation)continue;int cell=(edit.binding>>16)&0xffff,index=int(edit.binding&0xffff)-4;
   if(index<0&&index>=-3)g->common[-index-1]=edit.value;else if(cell<int(g->cells.size())){g->cells[cell]->set(index,edit.value);changedCells[cell]=true;}
   changed=true;for(int slot=0;slot<ControlSlots;slot++)if(editSequence_[slot].load()==edit.sequence)appliedSequence_[slot]=edit.sequence;
   for(int slot=0;slot<ControlSlots;slot++)if(bindings_[slot].load()==edit.binding)quietNorm(controlParameter(slot),edit.value);
  }if(changed){for(size_t cell=0;cell<g->cells.size();cell++)if(changedCells[cell])g->cells[cell]->syncValues();layoutRev_++;}
  if(resetRequested_.exchange(false)){g->midi(0xb0,123,0);freePpq_=0;for(int key=0;key<128;key++)if(heldVelocity_[key])g->midi(0x90,key,heldVelocity_[key],getInt(KitRoot));}
  double bpm=ctx.transport.tempoValid?ctx.transport.bpm:120;double start=ctx.transport.ppqValid?ctx.transport.ppqPos:freePpq_;double increment=std::clamp(bpm,20.,400.)*96/(60*ctx.sampleRate);
  if(panicRequested_.exchange(false)||(transportPlaying_&&!ctx.transport.playing)){g->midi(0xb0,64,0);g->midi(0xb0,120,0);g->midi(0xb0,123,0);for(auto&f:g->filters)f.reset();heldVelocity_.fill(0);}transportPlaying_=ctx.transport.playing;
  int midi=0;float a[128],b[128];float gain=get(Master);double tick=start*96;
  for(int f=0;f<ctx.numFrames;){int n=std::min(128,ctx.numFrames-f);
   // Split exactly at input MIDI and generated note boundaries.
   while(midi<ctx.numMidiEvents&&ctx.midi[midi].sampleOffset<=f){auto&e=ctx.midi[midi++];if(e.isNoteOn()){heldVelocity_[e.note()]=e.velocity();lastNote_=e.note();lastVelocity_=e.velocity();}if(e.isNoteOff())heldVelocity_[e.note()]=0;if(e.isAllNotesOff())heldVelocity_.fill(0);g->midi(e.data[0],e.data[1],e.data[2],getInt(KitRoot));}
   if(midi<ctx.numMidiEvents)n=std::min(n,ctx.midi[midi].sampleOffset-f);
   n=std::max(n,1);g->render(a,b,n,bpm,tick/96,ctx.transport.playing);for(int i=0;i<n;i++){masterGain_+=(gain-masterGain_)*.002f;float l=std::isfinite(a[i])?a[i]*masterGain_:0,r=std::isfinite(b[i])?b[i]*masterGain_:0;ctx.outputs[0][f+i]=l;ctx.outputs[1][f+i]=r;envelope_=std::max(std::max(std::fabs(l),std::fabs(r)),envelope_*.9995f);}f+=n;tick+=n*increment;
  }
  freePpq_=tick/96;bool held=std::any_of(heldVelocity_.begin(),heldVelocity_.end(),[](uint8_t v){return v!=0;});float activity=std::clamp(envelope_*4+(held?.45f:0.f),0.f,1.f);float phase=motionPhase_.load();if(activity>.001f)phase=std::fmod(phase+ctx.numFrames/float(ctx.sampleRate)*(.2f+lastNote_/180.f),1.f);motionPhase_=phase;
  float pitch=std::clamp(lastNote_/127.f,0.f,1.f),velocity=lastVelocity_/127.f;float target=activity*(.12f+.5f*pitch+.18f*velocity+.16f*std::sin(phase*6.2831853f));float smoothing=std::min(1.f,ctx.numFrames/float(ctx.sampleRate)*18.f);motionPose_+=(target-motionPose_)*smoothing;if(activity<.001f&&motionPose_<.0001f)motionPose_=0;quietSet(Visual,std::clamp(motionPose_,0.f,1.f));hazard_=nullptr;
 }
};
}
MPC_PLUGIN_ENTRY(re::RhythmerEngines)
