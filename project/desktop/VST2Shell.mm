// SPDX-License-Identifier: AGPL-3.0-or-later
// Hand-written VST2 ABI from the MIT MPC SDK; no Steinberg VST2 SDK dependency.
#include <juce_audio_utils/juce_audio_utils.h>
#include "mpc_vst2_abi.h"
#include <array>
#include <cstring>
extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
namespace {
struct Rect{int16_t top=0,left=0,bottom=820,right=1120;};
class Shell final:public juce::AudioProcessorListener,public juce::AudioPlayHead,private juce::ComponentListener {
public:
 juce::ScopedJuceInitialiser_GUI gui;std::unique_ptr<juce::AudioProcessor>processor;std::unique_ptr<juce::AudioProcessorEditor>editor;AEffect effect{};audioMasterCallback host;double rate=44100;int block=512;juce::MidiBuffer midi;std::vector<uint8_t>chunk;Rect rectangle;
 explicit Shell(audioMasterCallback h):host(h){processor.reset(createPluginFilter());midi.ensureSize(65536);processor->addListener(this);processor->setPlayHead(this);effect.magic=kEffectMagic;effect.object=this;effect.numPrograms=1;effect.numParams=processor->getParameters().size();effect.numOutputs=2;effect.flags=effFlagsHasEditor|effFlagsCanReplacing|effFlagsProgramChunks|effFlagsIsSynth;effect.uniqueID=('J'<<24)|('F'<<16)|('q'<<8)|'7';effect.version=100;effect.dispatcher=dispatch;effect.setParameter=[](AEffect*e,int i,float v){auto&s=*(Shell*)e->object;if(juce::isPositiveAndBelow(i,s.processor->getParameters().size()))s.processor->getParameters()[i]->setValue(v);};effect.getParameter=[](AEffect*e,int i){auto&s=*(Shell*)e->object;return juce::isPositiveAndBelow(i,s.processor->getParameters().size())?s.processor->getParameters()[i]->getValue():0.f;};effect.processReplacing=render;effect.process=render;}
 ~Shell()override{closeEditor();processor->removeListener(this);processor->releaseResources();}
 void closeEditor(){if(editor){editor->removeComponentListener(this);editor->setVisible(false);editor->removeFromDesktop();processor->editorBeingDeleted(editor.get());editor.reset();}}
 static void copy(void*p,const juce::String&s,int bytes){if(p)juce::CharPointer_UTF8((char*)p).writeWithDestByteLimit(s.getCharPointer(),size_t(bytes));}
 static VstIntPtr dispatch(AEffect*e,int op,int index,VstIntPtr value,void*ptr,float opt){auto&s=*(Shell*)e->object;auto&p=*s.processor;switch(op){
 case effOpen:return 1;case effClose:delete &s;return 1;
 case effSetSampleRate:s.rate=opt;return 1;case effSetBlockSize:s.block=int(value);return 1;
 case effMainsChanged:if(value){p.prepareToPlay(s.rate,s.block);e->initialDelay=p.getLatencySamples();}else p.releaseResources();return 1;
 case effGetVstVersion:return 2400;case effGetPlugCategory:return kPlugCategSynth;case effGetVendorVersion:return 100;
 case effGetEffectName:case effGetProductString:copy(ptr,"Rhythmer Engines",32);return 1;case effGetVendorString:copy(ptr,"Jazzy Rhythmer",64);return 1;
 case effGetProgram:return 0;case effSetProgram:return 0;case effGetProgramName:case effGetProgramNameIndexed:copy(ptr,"Default",24);return 1;
 case effGetParamName:if(juce::isPositiveAndBelow(index,p.getParameters().size()))copy(ptr,p.getParameters()[index]->getName(8),8);return 1;
 case effGetParamDisplay:if(juce::isPositiveAndBelow(index,p.getParameters().size()))copy(ptr,p.getParameters()[index]->getCurrentValueAsText(),8);return 1;
 case effGetParamLabel:if(ptr)((char*)ptr)[0]=0;return 1;
 case effCanBeAutomated:return juce::isPositiveAndBelow(index,p.getParameters().size())&&p.getParameters()[index]->isAutomatable();
 case effGetChunk:{juce::MemoryBlock b;p.getStateInformation(b);s.chunk.assign((const uint8_t*)b.getData(),(const uint8_t*)b.getData()+b.getSize());if(ptr)*(void**)ptr=s.chunk.data();return VstIntPtr(s.chunk.size());}
 case effSetChunk:if(ptr&&value>0&&value<16*1024*1024){p.setStateInformation(ptr,int(value));return 1;}return 0;
 case effProcessEvents:if(ptr){auto*ev=(VstEvents*)ptr;for(int i=0;i<std::min(512,ev->numEvents);i++)if(ev->events[i]&&ev->events[i]->type==kVstMidiType){auto*m=(VstMidiEvent*)ev->events[i];s.midi.addEvent(m->midiData,juce::MidiMessage::getMessageLengthFromFirstByte(uint8_t(m->midiData[0])),std::max(0,m->deltaFrames));}}return 1;
 case effEditGetRect:if(ptr)*(Rect**)ptr=&s.rectangle;return 1;
 case effEditOpen:s.closeEditor();s.editor.reset(p.createEditorAndMakeActive());if(!s.editor)return 0;s.editor->setTopLeftPosition(0,0);s.editor->addToDesktop(0,ptr);s.editor->setVisible(true);s.editor->addComponentListener(&s);return 1;
 case effEditClose:s.closeEditor();return 1;
 case effSetSpeakerArrangement:case effStartProcess:case effStopProcess:return 1;
 case effCanDo:if(!ptr)return 0;if(!std::strcmp((const char*)ptr,"hasCockosViewAsConfig")||!std::strcmp((const char*)ptr,"hasCockosExtensions"))return (int32_t)0xbeef0000;if(!std::strcmp((const char*)ptr,"receiveVstEvents")||!std::strcmp((const char*)ptr,"receiveVstMidiEvent")||!std::strcmp((const char*)ptr,"receiveVstTimeInfo"))return 1;return 0;
 case effGetTailSize:return VstIntPtr(p.getTailLengthSeconds()*s.rate);
 default:return 0;}}
 static void render(AEffect*e,float**,float**out,int n){auto&s=*(Shell*)e->object;juce::AudioBuffer<float>b(out,2,n);s.processor->processBlock(b,s.midi);s.midi.clear();}
 juce::Optional<PositionInfo>getPosition()const override{PositionInfo p;if(host){auto*t=(VstTimeInfo*)host((AEffect*)&effect,audioMasterGetTime,0,0x7fff,nullptr,0);if(t){p.setIsPlaying((t->flags&kVstTransportPlaying)!=0);p.setIsRecording((t->flags&kVstTransportRecording)!=0);if(t->flags&kVstTempoValid)p.setBpm(t->tempo);if(t->flags&kVstPpqPosValid)p.setPpqPosition(t->ppqPos);p.setTimeInSamples(int64_t(t->samplePos));p.setTimeSignature(juce::AudioPlayHead::TimeSignature{t->timeSigNumerator,t->timeSigDenominator});}}return p;}
 void audioProcessorParameterChanged(juce::AudioProcessor*,int i,float v)override{if(host)host(&effect,audioMasterAutomate,i,0,nullptr,v);}
 void audioProcessorChanged(juce::AudioProcessor*,const ChangeDetails&)override{effect.initialDelay=processor->getLatencySamples();if(host)host(&effect,audioMasterUpdateDisplay,0,0,nullptr,0);}
 void audioProcessorParameterChangeGestureBegin(juce::AudioProcessor*,int i)override{if(host)host(&effect,audioMasterBeginEdit,i,0,nullptr,0);}void audioProcessorParameterChangeGestureEnd(juce::AudioProcessor*,int i)override{if(host)host(&effect,audioMasterEndEdit,i,0,nullptr,0);}
 void componentMovedOrResized(juce::Component&c,bool,bool resized)override{if(resized){rectangle.right=int16_t(c.getWidth());rectangle.bottom=int16_t(c.getHeight());if(host)host(&effect,audioMasterSizeWindow,c.getWidth(),c.getHeight(),nullptr,0);}}
};
}
extern "C" __attribute__((visibility("default"))) AEffect*VSTPluginMain(audioMasterCallback h){try{if(h&&h(nullptr,audioMasterVersion,0,0,nullptr,0)==0)return nullptr;return &(new Shell(h))->effect;}catch(...){return nullptr;}}
