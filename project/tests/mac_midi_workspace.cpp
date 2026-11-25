#include <juce_audio_utils/juce_audio_utils.h>
#include "component_host.h"
#include "../desktop/MidiWorkspace.h"
#include <iostream>
#include <fstream>
static void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(int argc,char**argv){juce::ScopedJuceInitialiser_GUI gui;try{
 desktop::MidiWorkspace w;int checks=0;re::Json last;
 for(int genre=0;genre<61;genre++)for(int type=0;type<3;type++)for(int bars:{1,4,16})for(int style=0;style<3;style++){
  if(type==2&&!w.supportsDrums(genre))continue;
  auto s=w.defaults();s["genre"]=genre;s["mode"]=type;s["bars"]=bars;s["play"]=style;
  auto d=w.generate(s,type==2,false,1729+checks);check(w.valid(d),"Invalid generated document");if(d["notes"].empty()){std::cerr<<"EMPTY genre="<<genre<<" type="<<type<<" bars="<<bars<<" style="<<style<<"\n";throw std::runtime_error("Empty pattern");}auto file=w.midiFile(d);juce::MemoryOutputStream out;check(file.writeTo(out,0),"MIDI write");juce::MemoryInputStream in(out.getData(),out.getDataSize(),false);juce::MidiFile read;check(read.readFrom(in),"MIDI read");check(read.getTimeFormat()==96,"MIDI clock mismatch");check(read.getLastTimestamp()==bars*384,"MIDI loop length lost");int ons=0,offs=0;std::array<int,128>active{};for(int i=0;i<read.getTrack(0)->getNumEvents();i++){auto m=read.getTrack(0)->getEventPointer(i)->message;check(!m.isTempoMetaEvent(),"GEN overwrites host tempo");if(m.isNoteOn()){check(m.getVelocity()>0,"Lost velocity");check(active[m.getNoteNumber()]++==0,"Overlapping same-key notes");ons++;}if(m.isNoteOff()){check(--active[m.getNoteNumber()]==0,"Unmatched note off");offs++;}}check(ons==int(d["notes"].size())&&offs==ons,"MIDI event loss");last=d;checks++;
 }
 auto map=w.kitKeys(re::Json{{"routes",re::Json::array({{{"mix_role","snare"}},{{"mix_role","kick"}},{{"mix_role","closed_hat"}}})}});check(map[0]==37&&map[1]==36&&map[3]==38,"Kit role mapping mismatch");
 if(argc>=3){re::Json catalog;std::ifstream(argv[2])>>catalog;for(const auto&kit:catalog["kits"]){auto keys=w.kitKeys(kit);if(kit["name"]=="KrautDrums")check(keys[1]==37&&keys[3]==39&&keys[4]==40,"KrautDrums voice role map");for(int genre=0;genre<61;genre++){if(!w.supportsDrums(genre))continue;auto settings=w.defaults();settings["genre"]=genre;settings["mode"]=2;settings["bars"]=4;auto doc=w.generate(settings,true,false,1729+checks,keys);check(w.valid(doc),"Kit GEN invalid");auto midi=w.midiFile(doc);std::array<int,128>held{};for(int n=0;n<midi.getTrack(0)->getNumEvents();n++){auto m=midi.getTrack(0)->getEventPointer(n)->message;if(m.isNoteOn())check(held[m.getNoteNumber()]++==0,"Kit role overlap");if(m.isNoteOff())check(--held[m.getNoteNumber()]==0,"Kit role note off");}for(int n:held)check(!n,"Kit GEN hanging note");checks++;}}}
 desktop::MidiWorkspace restored;restored.restore(last);check(restored.snapshot()==last,"Document restore mismatch");auto malformed=last;malformed["notes"][0][3]=128;check(!w.valid(malformed),"Invalid velocity accepted");
 if(argc>=2){re::Core core("",0,argv[1]);auto b=core.state();check(desktop::extraOffset(b)>0,"Backend chunk mismatch");desktop::writeWorkspace(b,last);check(desktop::readWorkspace(b)==last,"Workspace chunk lost");core.restore(b);check(desktop::extraOffset(core.state())>0,"Backend restore corrupted");}
 std::cout<<"MIDI_WORKSPACE_PASS patterns="<<checks<<" genres=61 types=3 styles=3 lengths=1/4/16 exact_note_pairs velocity no_tempo_meta state_roundtrip\n";
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
