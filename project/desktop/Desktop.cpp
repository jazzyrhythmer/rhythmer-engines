// SPDX-License-Identifier: AGPL-3.0-or-later
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "component_host.h"
#include "LiveArt.h"
#include "MidiWorkspace.h"
#include "MidiView.h"

#include <fstream>
namespace desktop {
static constexpr int desktopParameterCount=103;
static int nativeParameter(int p){return p<97?p:p+82;}
static void anchor(){}
#if defined(_WIN32)
static constexpr const char* backendExtension=".dll";
#else
static constexpr const char* backendExtension=".dylib";
#endif
static juce::File resources(){auto path=re::containingLibrary(reinterpret_cast<const void*>(&anchor));if(path.empty())return {};return juce::File(juce::String::fromUTF8(path.c_str())).getParentDirectory().getParentDirectory().getChildFile("Resources");}
static juce::Typeface::Ptr loadFace(const char* filename,float weight){juce::MemoryBlock bytes;resources().getChildFile(juce::String("Fonts/")+filename).loadFileAsData(bytes);if(!bytes.getSize())return {};auto face=juce::Typeface::createSystemTypefaceFor(bytes.getData(),bytes.getSize());if(face){const juce::FontVariableSetting setting{juce::FontFeatureTag("wght"),weight};if(auto weighted=face->cloneWithVariableSettings({&setting,1}))return weighted;}return face;}
class Processor;
class Parameter final:public juce::AudioProcessorParameterWithID {
 Processor& owner;int index;float initial;
public:
 Parameter(Processor&,int);
 float getValue()const override;void setValue(float)override;float getDefaultValue()const override{return initial;}
 juce::String getName(int max)const override;juce::String getLabel()const override{return {};}
 juce::String getText(float,int)const override;float getValueForText(const juce::String&s)const override{return s.getFloatValue();}
 bool isAutomatable()const override;
 bool isMetaParameter()const override;Category getCategory()const override;int getNumSteps()const override;bool isDiscrete()const override;bool isBoolean()const override;
};
class Editor;
class Processor final:public juce::AudioProcessor,private juce::Timer {
public:
 MidiWorkspace midiWorkspace;
 std::unique_ptr<re::Core> core;re::Json catalog;juce::String error;juce::MidiKeyboardState keyboard;
 std::atomic<int> visualNote{60},visualHeld{0};std::atomic<float> visualLevel{0};std::atomic<uint64_t> visualBlock{0},visualStrike{0};
 juce::MidiBuffer routedMidi;bool wasHostPlaying=false;
 std::array<float,desktopParameterCount> publishedValues{};std::array<std::string,24> publishedNames{};
 std::array<bool,128> visualKeys{};bool visualSustain=false;double visualSampleRate=44100;float visualEnvelope=0;
 Processor():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)){
  try{auto r=resources();auto text=r.getChildFile("Content/catalog.json").loadFileAsString();if(text.isEmpty())throw std::runtime_error("Missing sound catalog");catalog=re::Json::parse(text.toStdString());core=std::make_unique<re::Core>("",0,r.getChildFile(juce::String("Backend/rhythmer-engines")+backendExtension).getFullPathName().toStdString());}
  catch(const std::exception&e){error=e.what();}
  routedMidi.ensureSize(65536);
  for(int i=0;i<desktopParameterCount;i++){addParameter(new Parameter(*this,i));publishedValues[i]=value(i);}
  startTimerHz(20);
 }
 ~Processor()override{stopTimer();}
 void timerCallback()override{
  if(!core)return;bool namesChanged=false;
  for(int k=0;k<24;k++){auto name=core->name(k<16?6+k:89+k-16);if(name!=publishedNames[k]){publishedNames[k]=std::move(name);namesChanged=true;}}
  for(int i=0;i<desktopParameterCount;i++){float current=value(i);if(std::abs(current-publishedValues[i])>1e-7f){publishedValues[i]=current;if(getParameters()[i]->getCategory()==juce::AudioProcessorParameter::genericParameter)getParameters()[i]->sendValueChangedMessageToListeners(current);}}
  // Metadata and mirrored values are published on the message thread. Never
  // write them back through setValue(), which would retrigger sound edits.
  if(namesChanged)updateHostDisplay(juce::AudioProcessorListener::ChangeDetails().withParameterInfoChanged(true));
 }
 const juce::String getName()const override{return "Rhythmer Engines";}
 void prepareToPlay(double rate,int block)override{visualSampleRate=rate;visualKeys.fill(false);visualSustain=false;visualHeld=0;visualEnvelope=0;if(core){core->dispatch(effMainsChanged,0,0);core->dispatch(effSetSampleRate,0,0,nullptr,float(rate));core->dispatch(effSetBlockSize,0,block);core->dispatch(effMainsChanged,0,1);setLatencySamples(core->latency());}}
 void releaseResources()override{visualKeys.fill(false);visualHeld=0;visualSustain=false;visualLevel=0;if(core)core->dispatch(effMainsChanged,0,0);}
 bool isBusesLayoutSupported(const BusesLayout&l)const override{return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
 void processBlock(juce::AudioBuffer<float>&audio,juce::MidiBuffer&midi)override{
  juce::ScopedNoDenormals guard;audio.clear();if(!core)return;
  double bpm=120,ppq=0;bool playing=false;if(auto*h=getPlayHead())if(auto p=h->getPosition()){bpm=p->getBpm().orFallback(120.);ppq=p->getPpqPosition().orFallback(0.);playing=p->getIsPlaying();}core->tempo(bpm,ppq,playing);
  // Clear the displayed keyboard as well as the sound before new live MIDI.
  // Preallocated in the constructor; short messages and a bounded input copy.
  routedMidi.clear();if(wasHostPlaying&&!playing)for(int channel=1;channel<=16;channel++){routedMidi.addEvent(juce::MidiMessage::controllerEvent(channel,64,0),0);routedMidi.addEvent(juce::MidiMessage::allSoundOff(channel),0);routedMidi.addEvent(juce::MidiMessage::allNotesOff(channel),0);}wasHostPlaying=playing;
  int copied=0;for(auto event:midi)if(event.getMessage().getRawDataSize()<=3&&copied++<512)routedMidi.addEvent(event.getMessage(),event.samplePosition);
  keyboard.processNextMidiBuffer(routedMidi,0,audio.getNumSamples(),true);
  std::array<VstMidiEvent,512>events{};struct Packet{VstInt32 numEvents=0;VstIntPtr reserved=0;VstEvent* events[512]{};}packet;
  for(auto x:routedMidi){auto m=x.getMessage();if(m.isNoteOn()){visualKeys[m.getNoteNumber()]=true;visualNote=m.getNoteNumber();visualStrike.fetch_add(1,std::memory_order_relaxed);}else if(m.isNoteOff())visualKeys[m.getNoteNumber()]=false;else if(m.isController()){if(m.getControllerNumber()==64)visualSustain=m.getControllerValue()>=64;else if(m.getControllerNumber()==120||m.getControllerNumber()==123){visualKeys.fill(false);visualSustain=false;}}
if(m.getRawDataSize()>3||packet.numEvents==512)continue;int i=packet.numEvents++;auto&e=events[i];e.type=kVstMidiType;e.byteSize=sizeof(e);e.deltaFrames=x.samplePosition;std::memcpy(e.midiData,m.getRawData(),size_t(m.getRawDataSize()));packet.events[i]=(VstEvent*)&e;}
  if(packet.numEvents)core->dispatch(effProcessEvents,0,0,&packet);core->render(audio.getWritePointer(0),audio.getWritePointer(1),audio.getNumSamples());
  float peak=0;for(int c=0;c<2;c++)for(int i=0;i<audio.getNumSamples();i++)peak=std::max(peak,std::fabs(audio.getSample(c,i)));visualEnvelope=std::max(peak,visualEnvelope*float(std::exp(-audio.getNumSamples()/(.15*visualSampleRate))));visualLevel.store(std::min(visualEnvelope,1.f),std::memory_order_relaxed);bool held=std::any_of(visualKeys.begin(),visualKeys.end(),[](bool v){return v;});visualHeld.store(held||(visualSustain&&visualEnvelope>.001f),std::memory_order_relaxed);visualBlock.fetch_add(1,std::memory_order_relaxed);midi.clear();
 }
 bool acceptsMidi()const override{return true;}bool producesMidi()const override{return false;}double getTailLengthSeconds()const override{return 10;}
 int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return "Default";}void changeProgramName(int,const juce::String&)override{}
 void getStateInformation(juce::MemoryBlock&out)override{const juce::ScopedLock lock(getCallbackLock());if(core){auto b=core->state();writeWorkspace(b,midiWorkspace.snapshot());out.replaceAll(b.data(),b.size());}}
 void setStateInformation(const void*d,int n)override{const juce::ScopedLock lock(getCallbackLock());if(core&&n>0&&n<16*1024*1024){auto*b=(const uint8_t*)d;std::vector<uint8_t> bytes(b,b+n);core->restore(bytes);midiWorkspace.restore(readWorkspace(bytes));}}
 bool hasEditor()const override{return true;}juce::AudioProcessorEditor*createEditor()override;
 float value(int p)const{return core?core->get(nativeParameter(p)):0;}juce::String name(int p)const{return core?juce::String(core->name(nativeParameter(p))):"Unavailable";}
 juce::String display(int p)const{if(!core)return error;char text[1024]{};core->dispatch(effGetParamDisplay,nativeParameter(p),0,text);return juce::String(text);}
 void edit(int p,float value){if(p<0||p>=getParameters().size())return;auto*q=getParameters()[p];q->beginChangeGesture();q->setValueNotifyingHost(value);q->endChangeGesture();}
};
Parameter::Parameter(Processor&o,int i):AudioProcessorParameterWithID(juce::ParameterID("rhythmer_"+juce::String(i),1),"Parameter "+juce::String(i)),owner(o),index(i),initial(o.value(i)){}
float Parameter::getValue()const{return owner.value(index);}void Parameter::setValue(float v){if(owner.core)owner.core->set(nativeParameter(index),v);}
juce::String Parameter::getName(int max)const{auto text=owner.name(index);return max>0?text.substring(0,max):text;}juce::String Parameter::getText(float v,int max)const{
 juce::String text;auto item=[&](const re::Json&list,float scale){int i=int(std::lround(std::clamp(v,0.f,1.f)*scale));return i>=0&&i<int(list.size())?juce::String(list[i]["name"].get<std::string>()):juce::String("--");};
 if(!owner.catalog.is_null()&&index==1)text=item(owner.catalog["entries"],float(owner.catalog["entries"].size()-1));
 else if(!owner.catalog.is_null()&&index==22)text=item(owner.catalog["kits"],float(owner.catalog["kits"].size()-1));
 else if(index==3)text="PAD "+juce::String(1+int(std::lround(v*15)));
 else if(index==4)text=juce::StringArray{"ALL","SOUND","MOD","FX"}[std::clamp(int(std::lround(v*3)),0,3)];
 else if(!owner.catalog.is_null()&&(index==23||(((index>=47&&index<73)||(index>=99&&index<103))))){std::vector<int>sources;for(int i=0;i<int(owner.catalog["sources"].size());i++)if(owner.catalog["sources"][i]["name"]!="Acid Engine")sources.push_back(i);int pick=index==23?std::clamp(int(std::lround(v*(sources.size()-1))),0,int(sources.size()-1)):index>=99?index-73:index-47;const auto&source=owner.catalog["sources"][sources[pick]];text=index==23?juce::String(source["name"].get<std::string>()):item(source["sounds"],float(source["sounds"].size()-1));}
 else if(((index>=28&&index<47)||(index>=97&&index<99))&&!owner.catalog.is_null()){int source=owner.catalog["entries"][(index>=97?index-77:index-27)]["source_index"];const auto&list=owner.catalog["sources"][source]["sounds"];text=item(list,float(list.size()-1));}
 else text=std::abs(v-getValue())<.0001f?owner.display(index):juce::String(v,3);
 return max>0?text.substring(0,max):text;
}bool Parameter::isAutomatable()const{return owner.core&&owner.core->dispatch(effCanBeAutomated,nativeParameter(index))!=0;}
bool Parameter::isMetaParameter()const{return index!=0&&getCategory()==genericParameter;}
juce::AudioProcessorParameter::Category Parameter::getCategory()const{return index==25||index==26||(index>=73&&index<89)?otherMeter:genericParameter;}
int Parameter::getNumSteps()const{
 switch(index){case 1:return owner.catalog.contains("entries")?int(owner.catalog["entries"].size()):22;case 3:return 16;case 4:return 4;case 22:return owner.catalog.contains("kits")?int(owner.catalog["kits"].size()):36;case 23:return owner.catalog.contains("sources")?int(owner.catalog["sources"].size())-1:26;case 27:return 2;default:break;}
 if(((index>=28&&index<47)||(index>=97&&index<99))&&!owner.catalog.is_null()){int source=owner.catalog["entries"][(index>=97?index-77:index-27)]["source_index"];return int(owner.catalog["sources"][source]["sounds"].size());}
 if(((index>=47&&index<73)||(index>=99&&index<103))&&!owner.catalog.is_null()){int selected=index>=99?index-73:index-47;for(const auto&source:owner.catalog["sources"])if(source["name"]!="Acid Engine"&&selected--==0)return int(source["sounds"].size());}
 return getDefaultNumParameterSteps();
}
bool Parameter::isDiscrete()const{return getNumSteps()!=getDefaultNumParameterSteps();}
bool Parameter::isBoolean()const{return index==27;}
class PosterLook final:public juce::LookAndFeel_V4 {
public:
 juce::Typeface::Ptr body,heading;
 juce::Typeface::Ptr getTypefaceForFont(const juce::Font& f)override{return body?body:juce::LookAndFeel_V4::getTypefaceForFont(f);}
 juce::Font getComboBoxFont(juce::ComboBox& b)override{return juce::Font(b.getComponentID()=="engine"&&heading?heading:body).withHeight(b.getComponentID()=="engine"?30.f:19.f);}
 juce::Font getPopupMenuFont()override{return juce::Font(body).withHeight(20);}
 void drawButtonText(juce::Graphics&g,juce::TextButton&b,bool over,bool down)override{g.setColour((b.getToggleState()||over||down?juce::Colours::black:juce::Colours::white).withAlpha(b.isEnabled()?1.f:.3f));g.setFont(juce::Font(heading?heading:body).withHeight(b.getComponentID().startsWith("perf_")?14:18));g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(5),juce::Justification::centred,1);}

 void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down)override{g.setColour(b.getToggleState()||over||down?juce::Colours::white:juce::Colours::black);g.fillRect(b.getLocalBounds());g.setColour(juce::Colours::white.withAlpha(b.isEnabled()?1.f:.3f));g.drawRect(b.getLocalBounds(),b.isEnabled()?3:1);}
 void drawRotarySlider(juce::Graphics&g,int x,int y,int w,int h,float position,float start,float end,juce::Slider&)override{auto bounds=juce::Rectangle<float>(float(x),float(y),float(w),float(h)).reduced(9);float radius=std::min(bounds.getWidth(),bounds.getHeight())*.5f;auto centre=bounds.getCentre();juce::Path base;base.addCentredArc(centre.x,centre.y,radius,radius,0,start,end,true);g.setColour(juce::Colours::white);g.strokePath(base,juce::PathStrokeType(1.5f));juce::Path value;float angle=start+position*(end-start);value.addCentredArc(centre.x,centre.y,radius,radius,0,start,angle,true);g.strokePath(value,juce::PathStrokeType(5));auto dot=centre+juce::Point<float>(std::sin(angle)*radius,-std::cos(angle)*radius);g.fillEllipse(dot.x-4,dot.y-4,8,8);}
 void drawComboBox(juce::Graphics&g,int w,int h,bool,int,int,int,int,juce::ComboBox&)override{g.fillAll(juce::Colours::black);g.setColour(juce::Colours::white);g.drawRect(0,0,w,h,3);juce::Path arrow;arrow.addTriangle(float(w-22),h*.43f,float(w-10),h*.43f,float(w-16),h*.62f);g.fillPath(arrow);}
};
class FullKeyboard final:public juce::MidiKeyboardComponent {
public:
 explicit FullKeyboard(juce::MidiKeyboardState&s):MidiKeyboardComponent(s,horizontalKeyboard){}
 void drawWhiteNote(int note,juce::Graphics&g,juce::Rectangle<float>r,bool down,bool over,juce::Colour,juce::Colour)override{g.setColour(down?juce::Colours::black:juce::Colours::white);g.fillRect(r);g.setColour(down?juce::Colours::white:juce::Colours::black);g.drawRect(r,over?3.f:1.5f);if(note%12==0){g.setFont(12);g.drawText(juce::MidiMessage::getMidiNoteName(note,true,true,3),r.removeFromBottom(19),juce::Justification::centred);}}
 void drawBlackNote(int,juce::Graphics&g,juce::Rectangle<float>r,bool down,bool over,juce::Colour)override{g.setColour(down?juce::Colours::white:juce::Colours::black);g.fillRect(r);g.setColour(down?juce::Colours::black:juce::Colours::white);g.drawRect(r.reduced(.7f),over?2.5f:1.4f);}
};
class AboutView final:public juce::Component {
 juce::TextButton close{"BACK"};juce::Label title,bio;juce::TextEditor credits;
 std::array<juce::HyperlinkButton,9>links;
public:
 explicit AboutView(std::function<void()>back){
  title.setText("JAZZY RHYTHMER",juce::dontSendNotification);title.setFont(juce::Font(30,juce::Font::bold));addAndMakeVisible(title);
  bio.setText("In search of feel-good rhythms. Music, instruments & open-source firmware. Building JR Audio Software.",juce::dontSendNotification);bio.setFont(juce::Font(18,juce::Font::bold));addAndMakeVisible(bio);
  const char*names[]={"WEBSITE","INSTAGRAM","YOUTUBE","SOUNDCLOUD","BANDCAMP","SUPPORT / KO-FI","PATREON","GITHUB","CONTACT"};
  const char*urls[]={"https://jazzyrhythmer.github.io/","https://www.instagram.com/jazzyrhythmer/","https://www.youtube.com/@jazzyrhythmer","https://soundcloud.com/jazzyrhythmer","https://jazzyrhythmer.bandcamp.com/","https://ko-fi.com/jazzyrhythmer","https://www.patreon.com/JazzyRhythmer/","https://github.com/jazzyrhythmer","mailto:jazzyrhythmer@gmail.com"};
  for(int i=0;i<9;i++){links[i].setButtonText(names[i]);links[i].setURL(juce::URL(urls[i]));links[i].setColour(juce::HyperlinkButton::textColourId,juce::Colours::white);links[i].setFont(juce::Font(18,juce::Font::bold),true,juce::Justification::centred);addAndMakeVisible(links[i]);}
  credits.setMultiLine(true);credits.setReadOnly(true);credits.setScrollbarsShown(true);credits.setCaretVisible(false);credits.setFont(juce::Font(16,juce::Font::bold));credits.setColour(juce::TextEditor::backgroundColourId,juce::Colours::black);credits.setColour(juce::TextEditor::textColourId,juce::Colours::white);credits.setColour(juce::TextEditor::outlineColourId,juce::Colours::white);credits.setColour(juce::TextEditor::focusedOutlineColourId,juce::Colours::white);
  auto text=resources().getChildFile("CREDITS.md").loadFileAsString();if(text.isEmpty())text="Port, integration and interface by Jazzy Rhythmer. Source foundation: Melodee / Ellic Studio. Original engine names, notices and licenses retained in the package.";
  text=text.replace("# Credits","CREDITS").replace("Rhythmer Engines for MPC 3","Rhythmer Engines").replace("already approved ","");auto privateNote=text.indexOf("This is a private instrument test build.");if(privateNote>=0)text=text.substring(0,privateNote);
  text+="\n\nMac interface: JUCE contributors. Full original and transitive notices are retained in Licenses/.\n";credits.setText(text);for(auto*c:credits.getChildren())if(auto*v=dynamic_cast<juce::Viewport*>(c)){v->getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId,juce::Colours::white);v->getHorizontalScrollBar().setColour(juce::ScrollBar::thumbColourId,juce::Colours::white);}addAndMakeVisible(credits);close.onClick=std::move(back);addAndMakeVisible(close);
 }
 void paint(juce::Graphics&g)override{g.fillAll(juce::Colours::black);g.setColour(juce::Colours::white);g.drawRect(getLocalBounds().reduced(12),3);g.fillRect(28,126,getWidth()-56,3);}
 void resized()override{int w=getWidth();title.setBounds(28,24,w-210,48);close.setBounds(w-165,28,130,40);bio.setBounds(28,78,w-56,42);for(int i=0;i<9;i++){int col=i%3,row=i/3;links[i].setBounds(28+(w-56)*col/3,142+row*44,(w-56)/3,38);}credits.setBounds(28,286,w-56,getHeight()-316);}
};
static juce::Rectangle<float> glyphInk(const juce::Font&font,const juce::String&text){juce::GlyphArrangement glyphs;glyphs.addLineOfText(font,text,0,0);juce::Path path;glyphs.createPath(path);return path.getBounds();}
class BrandLink final:public juce::HyperlinkButton {
public:
 using HyperlinkButton::HyperlinkButton;juce::Typeface::Ptr typeface;
 void paintButton(juce::Graphics&g,bool over,bool down)override{auto font=juce::Font(typeface).withHeight(16);auto ink=glyphInk(font,getButtonText());g.setColour(juce::Colours::white);g.setFont(font);g.drawSingleLineText(getButtonText(),0,int(std::lround(getHeight()-3-ink.getBottom())));if(over||down)g.drawHorizontalLine(getHeight()-1,0,float(getWidth()));}
};
class KitPad final:public juce::TextButton {
public:
 juce::String number,sound;bool selected=false;juce::Typeface::Ptr font;
 void paintButton(juce::Graphics&g,bool over,bool down)override{g.fillAll(selected||down?juce::Colours::white:juce::Colours::black);g.setColour(selected||down?juce::Colours::black:juce::Colours::white);g.drawRect(getLocalBounds(),over?4:2);auto r=getLocalBounds().reduced(9);g.setFont(juce::Font(font).withHeight(23));g.drawText(number,r.removeFromTop(27),juce::Justification::left);g.setFont(juce::Font(font).withHeight(14));g.drawFittedText(sound,r,juce::Justification::bottomLeft,2);}
};
class Editor final:public juce::AudioProcessorEditor,private juce::Timer {
 Processor&p;PosterLook mono;juce::TextButton gen{"GEN"},about{"ABOUT"};std::unique_ptr<MidiView>midiView;BrandLink by{"by Jazzy Rhythmer",juce::URL("https://jazzyrhythmer.github.io/")};std::unique_ptr<AboutView>aboutView;juce::ComboBox engine,preset,pad,section,padEngine,padSound;juce::Slider output;juce::Label outputLabel;std::array<juce::Slider,16>knobs;std::array<juce::Label,16>names,values;
 std::array<KitPad,16>kitPads;std::array<bool,16>padDown{};int gridKit=-1;std::array<juce::TextButton,3>tabs;juce::TextButton prev{"<"},next{">"},panic{"STOP SOUND"};std::vector<int>sourcePicker;FullKeyboard keys;int tab=0,sub=0,pageCount=1,presetControl=2,presetRange=2048,currentEngine=-1,currentSource=-1,currentArt=18;std::array<int,16>targets{};juce::Typeface::Ptr face;std::unique_ptr<rhyvisual::Context,void(*)(rhyvisual::Context*)> visual{rhyvisual::create(),rhyvisual::destroy};
 juce::Rectangle<int> brandBounds,artBounds;float brandHeight=48;std::array<juce::Rectangle<int>,16> cards;double tickAt=0,visualAt=0,audioAt=0;float smoothLevel=0,gestureEnergy=0;uint64_t lastBlock=0,lastStrike=0;unsigned controlTicks=0;bool audioFresh=false;juce::String pageTitle;bool updating=false;
 int contextualPage(int engine)const{return engine==0?2:-1;}
 bool available(int page,int engine)const{return page<2||(page==2&&engine==0);}
 void choices(juce::ComboBox&box,const std::vector<juce::String>&text){box.clear(juce::dontSendNotification);for(size_t i=0;i<text.size();i++)box.addItem(text[i],int(i)+1);}
 void choose(juce::ComboBox&box,int control,int count){box.onChange=[this,&box,control,count]{if(!updating)p.edit(control,float(box.getSelectedId()-1)/std::max(1,count-1));};}
 void setTab(int i){tab=i;sub=0;for(int k=0;k<3;k++)tabs[k].setToggleState(k==i,juce::dontSendNotification);refresh();resized();}
 void refresh(){updating=true;int e=int(std::lround(p.value(1)*float(p.catalog["entries"].size()-1)));if(!available(tab,e)){tab=(tab==2||tab==4||tab==5)?contextualPage(e):0;if(tab<0)tab=0;sub=0;}for(int k=0;k<3;k++){tabs[k].setVisible(k==0||k==1||k==contextualPage(e));tabs[k].setEnabled(available(k,e));tabs[k].setToggleState(k==tab,juce::dontSendNotification);}engine.setSelectedId(e+1,juce::dontSendNotification);pad.setSelectedId(int(std::lround(p.value(3)*15))+1,juce::dontSendNotification);
  if(e!=currentEngine){currentEngine=e;static const char* artNames[]={"Sophie","Aphex","Elixir","FM-106","FMini-1","CS-1","OberVave","K-WASUB","Hera","Braids","Plaits","Hortator","MONO2K","Prophet","S1Vave","Dexed","Guru-600","Acid Engine","Rhythmer Kit","Break Engine","Frodhes","Tenebrae","Amino","Fizzik"};currentArt=0;auto artName=p.catalog["entries"][e]["name"].get<std::string>();for(int i=0;i<int(std::size(artNames));i++)if(artName==artNames[i]){currentArt=i;break;}std::vector<juce::String>s;int control=2,count=2048;
   if(!p.catalog.is_null()&&p.catalog.contains("entries")){
    if(e==0){for(auto&k:p.catalog["kits"])s.emplace_back(k["name"].get<std::string>());control=22;count=int(s.size());}
    else{int source=p.catalog["entries"][e]["source_index"];for(auto&v:p.catalog["sources"][source]["sounds"])s.emplace_back(v["name"].get<std::string>());}
   }choices(preset,s);choose(preset,control,count);presetControl=control;presetRange=count;resized();
  }preset.setSelectedId(int(std::lround(p.value(presetControl)*std::max(1,presetRange-1)))%std::max(1,preset.getNumItems())+1,juce::dontSendNotification);section.setSelectedId(int(std::lround(p.value(4)*3))+1,juce::dontSendNotification);
  int selected=int(std::lround(p.value(23)*float(sourcePicker.size()-1)));int source=sourcePicker[std::clamp(selected,0,int(sourcePicker.size()-1))];padEngine.setSelectedId(selected+1,juce::dontSendNotification);if(source!=currentSource&&!p.catalog.is_null()){currentSource=source;std::vector<juce::String>s;for(auto&v:p.catalog["sources"][source]["sounds"])s.emplace_back(v["name"].get<std::string>());choices(padSound,s);padSound.onChange=[this]{if(!updating)p.edit(24,float(padSound.getSelectedId()-1)/2047.f);};}padSound.setSelectedId(int(std::lround(p.value(24)*2047))%std::max(1,padSound.getNumItems())+1,juce::dontSendNotification);
  for(int i=0;i<16;i++){kitPads[i].setVisible(tab==2&&e==0);kitPads[i].setEnabled(p.display(26)=="READY");kitPads[i].selected=i==int(std::lround(p.value(3)*15));kitPads[i].sound=p.display(73+i);kitPads[i].repaint();}
  if(tab==0||tab==2){for(int k=0;k<8;k++)targets[k]=6+k;pageTitle=tab==2?"SELECTED PAD SOUND":"PLAY";}
  if(tab==1){for(int k=0;k<16;k++)targets[k]=k<8?14+k:89+k-8;auto pageText=p.display(5);pageCount=std::max(1,pageText.fromFirstOccurrenceOf("/",false,false).getIntValue());pageTitle="PAGE "+pageText.upToFirstOccurrenceOf(" ",false,false).replace("/"," / ");}
  for(int k=0;k<16;k++){int id=k<8||tab==1?targets[k]:-1;auto n=id>=0?p.name(id):juce::String();bool visible=id>=0&&n.isNotEmpty()&&!n.contains("unused");knobs[k].setVisible(visible);knobs[k].setEnabled(p.display(26)=="READY");names[k].setVisible(visible);values[k].setVisible(visible);if(visible){names[k].setText(n,juce::dontSendNotification);values[k].setText(p.display(id),juce::dontSendNotification);if(!knobs[k].isMouseButtonDown())knobs[k].setValue(p.value(id),juce::dontSendNotification);}}
  bool loaded=p.display(26)=="READY";
  for(auto*c:{&engine,&preset,&pad,&section,&padEngine,&padSound})c->setEnabled(loaded);
  panic.setEnabled(loaded);prev.setEnabled(loaded&&pageCount>1);next.setEnabled(loaded&&pageCount>1);
  if(!output.isMouseButtonDown())output.setValue(p.value(0),juce::dontSendNotification);
updating=false;if(tab==1)resized();repaint();
 }
 void timerCallback()override{if(!isShowing())return;if(++controlTicks%6==0)refresh();else repaint(artBounds.expanded(3));}

public:
 explicit Editor(Processor&proc):AudioProcessorEditor(proc),p(proc),keys(proc.keyboard){
  engine.setComponentID("engine");mono.body=loadFace("Rubik.ttf",800);face=loadFace("InterTight.ttf",900);mono.heading=face;
  mono.setColour(juce::TextButton::buttonColourId,juce::Colours::black);mono.setColour(juce::TextButton::buttonOnColourId,juce::Colours::white);mono.setColour(juce::TextButton::textColourOffId,juce::Colours::white);mono.setColour(juce::TextButton::textColourOnId,juce::Colours::black);mono.setColour(juce::PopupMenu::backgroundColourId,juce::Colours::black);mono.setColour(juce::PopupMenu::textColourId,juce::Colours::white);mono.setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colours::white);mono.setColour(juce::PopupMenu::highlightedTextColourId,juce::Colours::black);setLookAndFeel(&mono);
  setSize(1120,820);setResizable(true,false);setResizeLimits(1000,760,1600,1200);
  auto style=[](juce::ComboBox&c){c.setColour(juce::ComboBox::backgroundColourId,juce::Colours::black);c.setColour(juce::ComboBox::textColourId,juce::Colours::white);c.setColour(juce::ComboBox::outlineColourId,juce::Colours::white);};
  for(auto*c:{&engine,&preset,&pad,&section,&padEngine,&padSound}){addAndMakeVisible(c);style(*c);}
  std::vector<juce::String>text;if(!p.catalog.is_null()){for(auto&e:p.catalog["entries"])text.emplace_back(e["name"].get<std::string>());choices(engine,text);text.clear();for(int i=0;i<int(p.catalog["sources"].size());i++)if(p.catalog["sources"][i]["name"]!="Acid Engine"){sourcePicker.push_back(i);text.emplace_back(p.catalog["sources"][i]["name"].get<std::string>());}choices(padEngine,text);}choose(engine,1,int(p.catalog["entries"].size()));choose(pad,3,16);choose(section,4,4);choose(padEngine,23,int(sourcePicker.size()));
  for(int i=1;i<=16;i++)pad.addItem("PAD "+juce::String(i),i);section.addItemList({"ALL","SOUND","MOD","FX"},1);
  const char*tabNames[]={"PLAY","EDIT","KIT"};for(int i=0;i<3;i++){tabs[i].setButtonText(tabNames[i]);tabs[i].onClick=[this,i]{setTab(i);};addAndMakeVisible(tabs[i]);}
  for(int i=0;i<16;i++){auto&b=kitPads[i];b.number="P"+juce::String(i+1).paddedLeft('0',2);b.font=face;b.setComponentID("kit_pad_"+juce::String(i+1));b.setButtonText(b.number);b.onStateChange=[this,i]{bool down=kitPads[i].isDown();if(down==padDown[i])return;padDown[i]=down;if(down){p.edit(3,float(i)/15);p.keyboard.noteOn(1,36+i,1.f);refresh();}else p.keyboard.noteOff(1,36+i,0.f);};addChildComponent(b);}
  for(auto*b:{&prev,&next,&panic})addAndMakeVisible(b);
  auto turnPage=[this](int direction){int total=p.display(5).fromFirstOccurrenceOf("/",false,false).getIntValue();int n=int(std::lround(p.value(5)*511));p.edit(5,float((n+std::max(1,total)+direction)%std::max(1,total))/511);refresh();resized();};prev.onClick=[turnPage]{turnPage(-1);};next.onClick=[turnPage]{turnPage(1);};panic.onClick=[this]{p.keyboard.allNotesOff(0);p.edit(27,1);};
  for(int k=0;k<16;k++){auto&v=knobs[k];v.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);v.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);v.setRange(0,1,.00001);v.setColour(juce::Slider::rotarySliderFillColourId,juce::Colours::white);v.setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colours::black);v.setColour(juce::Slider::thumbColourId,juce::Colours::white);v.onValueChange=[this,k]{if(!updating&&targets[k]>=0){gestureEnergy=.22f;p.edit(targets[k],float(knobs[k].getValue()));}};addAndMakeVisible(v);addAndMakeVisible(names[k]);addAndMakeVisible(values[k]);for(auto*l:{&names[k],&values[k]}){l->setJustificationType(juce::Justification::centred);l->setColour(juce::Label::textColourId,juce::Colours::white);l->setFont(juce::Font(17,juce::Font::bold));}}
  addAndMakeVisible(output);addAndMakeVisible(outputLabel);outputLabel.setText("OUTPUT",juce::dontSendNotification);outputLabel.setFont(juce::Font(15,juce::Font::bold));outputLabel.setColour(juce::Label::textColourId,juce::Colours::white);output.setSliderStyle(juce::Slider::LinearHorizontal);output.setRange(0,1,.001);output.setTextBoxStyle(juce::Slider::TextBoxRight,false,48,24);output.setColour(juce::Slider::trackColourId,juce::Colours::white);output.setColour(juce::Slider::thumbColourId,juce::Colours::white);output.setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);output.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::black);output.textFromValueFunction=[](double v){return juce::String(int(std::lround(v*100)))+"%";};output.valueFromTextFunction=[](const juce::String&s){return s.getDoubleValue()/100.;};output.onValueChange=[this]{if(!updating)p.edit(0,float(output.getValue()));};
  addAndMakeVisible(keys);keys.setAvailableRange(24,96);keys.setLowestVisibleKey(24);keys.setScrollButtonsVisible(false);keys.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colours::white);keys.setColour(juce::MidiKeyboardComponent::blackNoteColourId,juce::Colours::black);keys.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,juce::Colours::black.withAlpha(.45f));keys.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,juce::Colours::black.withAlpha(.25f));keys.setColour(juce::MidiKeyboardComponent::textLabelColourId,juce::Colours::black);keys.setColour(juce::MidiKeyboardComponent::upDownButtonBackgroundColourId,juce::Colours::black);keys.setColour(juce::MidiKeyboardComponent::upDownButtonArrowColourId,juce::Colours::white);
  addAndMakeVisible(gen);gen.onClick=[this]{midiView=std::make_unique<MidiView>(p.midiWorkspace,[this]{return currentEngine==0;},[this]{int k=int(std::lround(p.value(22)*float(p.catalog["kits"].size()-1)));return MidiWorkspace::kitKeys(p.catalog["kits"][k]);},[this]{midiView->setVisible(false);});addAndMakeVisible(*midiView);midiView->setBounds(getLocalBounds());midiView->toFront(true);};
  by.typeface=mono.body;by.setTooltip("Open jazzyrhythmer.github.io");by.setColour(juce::HyperlinkButton::textColourId,juce::Colours::white);by.setFont(juce::Font(mono.body).withHeight(16),false,juce::Justification::centred);addAndMakeVisible(by);addAndMakeVisible(about);about.onClick=[this]{if(!aboutView){aboutView=std::make_unique<AboutView>([this]{aboutView->setVisible(false);});addChildComponent(*aboutView);}aboutView->setBounds(getLocalBounds());aboutView->setVisible(true);aboutView->toFront(true);};
  setTab(0);tickAt=visualAt=juce::Time::getMillisecondCounterHiRes();startTimerHz(60);
 }

 ~Editor()override{for(int i=0;i<16;i++)if(padDown[i])p.keyboard.noteOff(1,36+i,0.f);stopTimer();setLookAndFeel(nullptr);}
 void paint(juce::Graphics&g)override{
  auto now=juce::Time::getMillisecondCounterHiRes();float dt=float(std::clamp((now-visualAt)*.001,.001,.05));visualAt=now;auto block=p.visualBlock.load(std::memory_order_relaxed);if(block!=lastBlock){lastBlock=block;audioAt=now;}audioFresh=now-audioAt<200;float target=audioFresh?p.visualLevel.load(std::memory_order_relaxed):0;smoothLevel+=(target-smoothLevel)*(1-std::exp(-dt/(target>smoothLevel?.025f:.14f)));gestureEnergy*=std::exp(-dt*6);
  g.fillAll(juce::Colours::black);g.setColour(juce::Colours::white);g.setFont(juce::Font(face).withHeight(brandHeight));g.drawSingleLineText("RHYTHMER ENGINES",brandBounds.getX(),55);g.fillRect(22,73,getWidth()-44,3);
  g.setFont(juce::Font(mono.body).withHeight(12));g.drawText("ENGINE",24,85,300,16,juce::Justification::left);g.drawText(currentEngine==0?"KIT PRESET":"PRESET",int(getWidth()*.34)+12,85,400,16,juce::Justification::left);if(currentEngine==0)g.drawText("EDIT PAD",getWidth()-164,85,140,16,juce::Justification::left);
  if(tab==2){g.drawText("PAD ENGINE",24,230,205,15,juce::Justification::left);g.drawText("PAD SOUND",252,230,310,15,juce::Justification::left);}if(tab==1)g.drawText("SECTION",24,230,170,15,juce::Justification::left);
  g.setFont(juce::Font(face).withHeight(21));g.drawFittedText(pageTitle,tab==1?225:artBounds.getX()+45,tab==1?251:artBounds.getY()-29,tab==1?getWidth()-370:artBounds.getWidth()-90,25,juce::Justification::centred,1);
  g.setColour(juce::Colours::white);if(tab!=1)g.drawRect(artBounds,3);
  if(tab==0){juce::Graphics::ScopedSaveState saved(g);g.reduceClipRegion(artBounds.reduced(6));auto area=artBounds.reduced(13);rhyvisual::Signal signal;signal.kind=currentArt;signal.note=p.visualNote.load(std::memory_order_relaxed);signal.pad=currentEngine==0?std::clamp(signal.note-36,0,15):int(std::lround(p.value(3)*15));signal.now=uint32_t(1+juce::Time::getMillisecondCounterHiRes()-tickAt);auto strike=p.visualStrike.load(std::memory_order_relaxed);bool newStrike=strike!=lastStrike;lastStrike=strike;signal.held=audioFresh&&p.visualHeld.load(std::memory_order_relaxed);signal.strike=newStrike;if(newStrike)signal.held=1;signal.level=std::max(smoothLevel,gestureEnergy);signal.width=float(area.getWidth());signal.height=float(area.getHeight());auto title=preset.getText().toStdString();signal.preset=title.c_str();unsigned count=0;auto*faces=rhyvisual::frame(visual.get(),signal,count);g.setOrigin(area.getPosition());for(unsigned i=0;i<count;i++){juce::Path shape;shape.startNewSubPath(faces[i].xy[0],faces[i].xy[1]);for(int j=2;j<8;j+=2)shape.lineTo(faces[i].xy[j],faces[i].xy[j+1]);shape.closeSubPath();g.setColour(juce::Colours::white.withAlpha(faces[i].fill));g.fillPath(shape);g.setColour(juce::Colours::white.withAlpha(faces[i].stroke));g.strokePath(shape,juce::PathStrokeType(.85f));}}
  g.setColour(juce::Colours::white);for(int k=0;k<16;k++)if(knobs[k].isVisible()){g.drawRect(cards[k],3);g.setFont(juce::Font(mono.body).withHeight(11));g.drawText(juce::String(k+1).paddedLeft('0',2),cards[k].reduced(7).removeFromTop(13),juce::Justification::left);}
  g.setFont(juce::Font(mono.body).withHeight(14));g.drawText(p.error.isNotEmpty()?p.error:p.display(26),154,getHeight()-137,getWidth()-662,27,juce::Justification::left);
 }
 void resized()override{
  int w=getWidth(),h=getHeight(),source=int(w*.34);about.setBounds(w-153,22,131,38);auto creditInk=glyphInk(juce::Font(mono.body).withHeight(16),by.getButtonText());int creditWidth=int(std::ceil(creditInk.getRight()))+4;int rawWidth=juce::GlyphArrangement::getStringWidthInt(juce::Font(face).withHeight(48),"RHYTHMER ENGINES");int logoWidth=std::min(rawWidth,about.getX()-22-creditWidth-26);brandHeight=48.f*logoWidth/std::max(1,rawWidth);auto logoInk=glyphInk(juce::Font(face).withHeight(brandHeight),"RHYTHMER ENGINES");brandBounds={22,10,logoWidth+2,57};int right=22+int(std::ceil(logoInk.getRight()));int bottom=int(std::lround(55+logoInk.getBottom()));by.setBounds(right+12,bottom-23,creditWidth,26);if(aboutView)aboutView->setBounds(getLocalBounds());if(midiView)midiView->setBounds(getLocalBounds());gen.setBounds(22,h-139,120,32);engine.setBounds(22,105,source-22,59);preset.setBounds(source+12,105,currentEngine==0?w-source-196:w-source-34,59);pad.setBounds(w-170,105,148,59);pad.setVisible(currentEngine==0);
  int count=currentEngine==0?3:2,span=w-44;for(int i=0;i<count;i++){int x=22+span*i/count,right=22+span*(i+1)/count;tabs[i].setBounds(x,178,right-x-4,40);}
  section.setBounds(22,247,176,34);padEngine.setBounds(22,247,216,34);padSound.setBounds(250,247,330,34);section.setVisible(tab==1);padEngine.setVisible(tab==2);padSound.setVisible(tab==2);
  int used=tabs[0].getBottom();for(juce::Component*c:std::array<juce::Component*,3>{&section,&padEngine,&padSound})if(c->isVisible())used=std::max(used,c->getBottom());
  int bodyTop=used+33,side=std::clamp(int(w*.225),220,290),gap=18;artBounds={22+side+gap,bodyTop,w-44-2*(side+gap),h-159-bodyTop};
  prev.setBounds(artBounds.getX(),bodyTop-31,40,28);next.setBounds(artBounds.getRight()-40,bodyTop-31,40,28);prev.setVisible(tab==1);next.setVisible(prev.isVisible());
  int cw=(side-8)/2,ch=(artBounds.getHeight()-10)/2;for(int k=0;k<16;k++){if(k>=8&&tab!=1)continue;int bank=k/4,slot=k%4,x=bank?w-22-side:22;x+=(slot%2)*(cw+8);int y=artBounds.getY()+(slot/2)*(ch+10);cards[k]={x,y,cw,ch};names[k].setBounds(x+5,y+17,cw-10,43);names[k].setMinimumHorizontalScale(.8f);names[k].setFont(juce::Font(mono.body).withHeight(16));int size=std::min(cw-18,ch-104),zone=ch-96;knobs[k].setBounds(x+(cw-size)/2,y+61+(zone-size)/2,size,size);values[k].setBounds(x+3,y+ch-32,cw-6,27);values[k].setFont(juce::Font(face).withHeight(23));}
  if(tab==1){int visibleCount=0;for(auto&knob:knobs)visibleCount+=knob.isVisible();int top=292,bottom=h-159,width=w-44,rows=std::clamp((visibleCount+3)/4,1,4),gap=10,cw=(width-3*gap)/4,ch=(bottom-top-3*gap)/rows;artBounds={22,top,width,bottom-top};prev.setBounds(w-131,245,48,36);next.setBounds(w-70,245,48,36);for(int k=0;k<16;k++){int x=22+(k%4)*(cw+gap),y=top+(k/4)*(ch+gap);cards[k]={x,y,cw,ch};names[k].setBounds(x+26,y+3,cw-32,25);names[k].setFont(juce::Font(mono.body).withHeight(14));names[k].setJustificationType(juce::Justification::centredLeft);int size=std::clamp(ch-30,36,78);knobs[k].setBounds(x+7,y+26,size,ch-29);values[k].setBounds(x+size+12,y+26,cw-size-20,ch-29);values[k].setFont(juce::Font(face).withHeight(24));}}
  else for(int k=0;k<8;k++)names[k].setJustificationType(juce::Justification::centred);
  for(int i=0;i<16;i++){int col=i%4,row=3-i/4;auto r=artBounds.reduced(7);int gap=6,cw=(r.getWidth()-gap*3)/4,ch=(r.getHeight()-gap*3)/4;kitPads[i].setBounds(r.getX()+col*(cw+gap),r.getY()+row*(ch+gap),cw,ch);}
  panic.setBounds(w-500,h-139,160,32);
  outputLabel.setBounds(w-326,h-141,86,34);output.setBounds(w-238,h-141,216,34);keys.setBounds(0,h-96,w,96);keys.setKeyWidth(float(w)/43.f);keys.setLowestVisibleKey(24);
 }

};
juce::AudioProcessorEditor*Processor::createEditor(){return new Editor(*this);}
}
juce::AudioProcessor*JUCE_CALLTYPE createPluginFilter(){return new desktop::Processor;}
