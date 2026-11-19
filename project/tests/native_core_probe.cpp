// SPDX-License-Identifier: AGPL-3.0-or-later
#include "component_host.h"
#include <fstream>
#include <iostream>
#include <iomanip>
int main(int argc,char**argv){
 if(argc<4)return 2;
 re::Json cat;std::ifstream(argv[1])>>cat;
 int source=std::stoi(argv[3]);int begin=argc>4?std::stoi(argv[4]):0;int end=argc>5?std::stoi(argv[5]):int(cat["sources"][source]["sounds"].size());
 re::Json results=re::Json::array();
 try{
 for(int p=begin;p<end;p++){
  int actual=re::actualSource(cat,source,p);const auto&s=cat["sources"][actual];const auto&sound=cat["sources"][source]["sounds"][p];
  re::Core core(argv[2],actual);core.apply(s,sound);core.reference(s,cat["sources"][source]["program_states"][p]);
  // Validate actual values against the accepted source-state references.
  double stateError=0;const auto&ref=cat["sources"][source]["program_states"][p];
  for(int i=0;i<int(ref["values"].size());i++)stateError=std::max(stateError,std::fabs(double(core.get(i))-ref["values"][i].get<double>()));
  int note=s.value("drums",false)?sound.value("note",60):60;
  core.midi(0x90,note,110);
  double sum=0,peak=0;int invalid=0;long frames=0;float l[512],r[512];
  const int blocks[]={32,64,127,128,256,512};
  for(int b=0;frames<22050;b++){
   int n=blocks[b%6];std::fill(l,l+n,0);std::fill(r,r+n,0);core.render(l,r,n);
   for(int i=0;i<n;i++){if(!std::isfinite(l[i])||!std::isfinite(r[i]))invalid++;else{sum+=l[i]*l[i]+r[i]*r[i];peak=std::max(peak,double(std::max(std::fabs(l[i]),std::fabs(r[i]))));}}
   frames+=n;
  }
  core.midi(0x80,note,0);
  auto state=core.state();std::vector<float>before;for(int i=0;i<core.count();i++)before.push_back(core.get(i));
  if(core.count()>1)core.set(1,.731);core.restore(state);
  double restoreError=0;for(int i=0;i<core.count();i++)restoreError=std::max(restoreError,double(std::fabs(core.get(i)-before[i])));
  re::Json diff=re::Json::array();for(int i=0;i<int(ref["values"].size());i++)if(std::fabs(double(core.get(i))-ref["values"][i].get<double>())>1e-4)diff.push_back({i,core.get(i),ref["values"][i]});
  re::Json row={{"differences",diff},{"source",source},{"program",p},{"name",sound["name"]},{"nativeParameters",core.count()},{"rms",std::sqrt(sum/(frames*2))},{"peak",peak},{"nonfinite",invalid},{"sourceStateError",stateError},{"restoreError",restoreError},{"chunkBytes",state.size()}};
  results.push_back(row);std::cout<<row.dump()<<std::endl;
 }
 }catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}
 return 0;
}
