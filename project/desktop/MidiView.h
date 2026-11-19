// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
namespace desktop {
class MidiDragButton final:public juce::TextButton {
 bool started=false;
public:
 std::function<void()>beginDrag;
 MidiDragButton():TextButton("DRAG MIDI TO DAW"){}
 void mouseDown(const juce::MouseEvent&e)override{started=false;TextButton::mouseDown(e);}
 void mouseDrag(const juce::MouseEvent&e)override{TextButton::mouseDrag(e);if(!started&&e.getDistanceFromDragStart()>5&&beginDrag){started=true;beginDrag();}}
};
class MidiView final:public juce::Component {
 MidiWorkspace&workspace;std::function<bool()>isKit;std::function<std::array<uint8_t,8>()>drumKeys;std::function<void()>back;
 juce::TextButton close{"BACK"},create{"GENERATE"},mutate{"MUTATE"},undo{"UNDO"},save{"EXPORT MIDI"};MidiDragButton drag;
 std::array<juce::ComboBox,12>controls;std::array<juce::Label,12>labels;
 juce::Label title,help,status;std::unique_ptr<juce::FileChooser>chooser;re::Json previous;
 juce::Rectangle<int>roll;int selected=-1;juce::Slider velocity;juce::Label velocityLabel;
 const std::array<const char*,12>ids{{"mode","genre","bars","root","scale","complexity","energy","chords","color","play","gate","octave"}};
 const std::array<const char*,12>names{{"TYPE","GENRE","BARS","KEY","SCALE","RHYTHM","ENERGY","CHORD COUNT","HARMONY","PLAY STYLE","NOTE LENGTH","OCTAVE"}};
 juce::Random random;std::array<uint32_t,8>recent{};unsigned cursor=0;
 re::Json settings()const{auto s=MidiWorkspace::defaults();for(int i=0;i<12;i++)s[ids[i]]=controls[i].getSelectedId()-1;return s;}
 void sync(){auto d=workspace.snapshot();auto s=MidiWorkspace::valid(d)?d["settings"]:MidiWorkspace::defaults();for(int i=0;i<12;i++){int id=s.value(ids[i],0)+1;bool found=false;for(int n=0;n<controls[i].getNumItems();n++)found|=controls[i].getItemId(n)==id;if(!found)id=controls[i].getItemId(0);controls[i].setSelectedId(id,juce::dontSendNotification);}selected=-1;velocity.setEnabled(false);updateStatus();}
 void updateStatus(){auto d=workspace.snapshot();bool valid=MidiWorkspace::valid(d)&&!d["notes"].empty();save.setEnabled(valid);drag.setEnabled(valid);mutate.setEnabled(valid&&d.value("kind",std::string())==(isKit()?"drums":controls[0].getSelectedId()==2?"chords":"melody"));undo.setEnabled(MidiWorkspace::valid(previous));status.setText(valid?juce::String(int(d["notes"].size()))+" NOTES / "+juce::String(d["length"].get<int>()/384)+" BARS / "+juce::String(d.value("kind",std::string())):"Choose settings, then GENERATE.",juce::dontSendNotification);bool chord=!isKit()&&controls[0].getSelectedId()==2;for(int i:{7,8,9,10,11})controls[i].setEnabled(chord);controls[0].setEnabled(!isKit());if(isKit())controls[0].setSelectedId(3,juce::dontSendNotification);repaint();}
 void generate(bool changing){previous=workspace.snapshot();uint32_t seed;do{seed=uint32_t(random.nextInt());}while(!seed||std::find(recent.begin(),recent.end(),seed)!=recent.end());recent[cursor++%8]=seed;workspace.generate(settings(),isKit(),changing,seed,drumKeys());selected=-1;velocity.setEnabled(false);updateStatus();}
public:
 MidiView(MidiWorkspace&w,std::function<bool()>kit,std::function<std::array<uint8_t,8>()>keys,std::function<void()>exit):workspace(w),isKit(std::move(kit)),drumKeys(std::move(keys)),back(std::move(exit)){
  title.setText("GEN / MIDI PATTERN",juce::dontSendNotification);title.setFont(juce::Font(30,juce::Font::bold));addAndMakeVisible(title);
  help.setText("Generate notes. Drag or export MIDI to a DAW track to play and edit. Host tempo stays unchanged.",juce::dontSendNotification);help.setFont(juce::Font(16,juce::Font::bold));addAndMakeVisible(help);status.setFont(juce::Font(15,juce::Font::bold));addAndMakeVisible(status);
  for(int i=0;i<12;i++){labels[i].setText(names[i],juce::dontSendNotification);labels[i].setFont(juce::Font(14,juce::Font::bold));addAndMakeVisible(labels[i]);addAndMakeVisible(controls[i]);controls[i].onChange=[this]{updateStatus();};}
  if(isKit())controls[0].addItem("DRUMS (KIT)",3);else controls[0].addItemList({"MELODY","CHORDS"},1);
  for(unsigned i=0;i<61;i++)if(!isKit()||MidiWorkspace::supportsDrums(i))controls[1].addItem(rhythmer::gen_menu_name(i),int(i)+1);
  for(int i=1;i<=16;i++)controls[2].addItem(juce::String(i),i+1);
  for(int i=0;i<12;i++)controls[3].addItem(rhythmer::note_letter(i),i+1);
  for(int i=0;i<13;i++)controls[4].addItem(rhythmer::scales()[i].name,i+1);
  for(int i:{5,6})for(int n=0;n<=100;n+=5)controls[i].addItem(juce::String(n)+"%",n+1);
  for(int i=1;i<=16;i++)controls[7].addItem(juce::String(i),i+1);
  for(int i=0;i<6;i++)controls[8].addItem(rhythmer::chord_color_name(i),i+1);
  for(int i=0;i<3;i++)controls[9].addItem(rhythmer::chord_play_name(i),i+1);
  for(int i=10;i<=100;i+=5)controls[10].addItem(juce::String(i)+"%",i+1);
  for(int i=1;i<=6;i++)controls[11].addItem(juce::String(i),i+1);
  close.onClick=[this]{back();};create.onClick=[this]{generate(false);};mutate.onClick=[this]{generate(true);};undo.onClick=[this]{auto now=workspace.snapshot();workspace.restore(previous);previous=now;sync();};
  save.onClick=[this]{chooser=std::make_unique<juce::FileChooser>("Export MIDI pattern",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Rhythmer-Pattern.mid"),"*.mid");juce::Component::SafePointer<MidiView>self(this);chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[self](const juce::FileChooser&c){if(!self)return;auto f=c.getResult();if(f==juce::File())return;bool ok=MidiWorkspace::write(f.withFileExtension("mid"),self->workspace.snapshot());self->status.setText(ok?"MIDI SAVED":"Could not save MIDI. Choose a writable folder.",juce::dontSendNotification);});};
  drag.beginDrag=[this]{auto f=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("Rhythmer-"+juce::Uuid().toString()+".mid");if(MidiWorkspace::write(f,workspace.snapshot())){juce::DragAndDropContainer::performExternalDragDropOfFiles({f.getFullPathName()},false,this);status.setText("Drop onto a DAW MIDI track. You can also use EXPORT MIDI.",juce::dontSendNotification);}};
  for(juce::Button*b:std::array<juce::Button*,6>{&close,&create,&mutate,&undo,&save,&drag})addAndMakeVisible(b);
  velocityLabel.setText("SELECTED NOTE VELOCITY",juce::dontSendNotification);velocityLabel.setFont(juce::Font(14,juce::Font::bold));addAndMakeVisible(velocityLabel);velocity.setRange(1,127,1);velocity.setColour(juce::Slider::thumbColourId,juce::Colours::white);velocity.setColour(juce::Slider::trackColourId,juce::Colours::white);velocity.setColour(juce::Slider::backgroundColourId,juce::Colours::black);velocity.setSliderStyle(juce::Slider::LinearHorizontal);velocity.setTextBoxStyle(juce::Slider::TextBoxRight,false,48,26);velocity.setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);velocity.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::black);velocity.onValueChange=[this]{auto d=workspace.snapshot();if(selected>=0&&MidiWorkspace::valid(d)&&selected<int(d["notes"].size())){d["notes"][selected][3]=int(velocity.getValue());workspace.restore(d);repaint();}};addAndMakeVisible(velocity);sync();
 }
 void resized()override{int w=getWidth(),h=getHeight();title.setBounds(28,24,w-210,44);close.setBounds(w-165,28,130,40);help.setBounds(28,76,w-56,44);for(int i=0;i<12;i++){int x=28+(w-56)*(i%6)/6,y=128+(i/6)*76,cw=(w-56)/6;labels[i].setBounds(x,y,cw-8,21);controls[i].setBounds(x,y+23,cw-10,43);}roll={28,300,w-56,h-443};status.setBounds(28,h-136,w/2,30);velocityLabel.setBounds(w/2,h-139,200,30);velocity.setBounds(w/2+205,h-139,w/2-233,30);int span=(w-56)/5;int i=0;for(juce::Button*b:std::array<juce::Button*,5>{&create,&mutate,&undo,&save,&drag})b->setBounds(28+span*i++,h-88,span-8,48);}
 void paint(juce::Graphics&g)override{g.fillAll(juce::Colours::black);g.setColour(juce::Colours::white);g.drawRect(getLocalBounds().reduced(12),3);g.drawRect(roll,2);auto d=workspace.snapshot();if(!MidiWorkspace::valid(d))return;int lo=127,hi=0;for(auto&n:d["notes"]){lo=std::min(lo,n[2].get<int>());hi=std::max(hi,n[2].get<int>());}lo=std::max(0,lo-2);hi=std::min(127,std::max(lo+12,hi+2));float row=float(roll.getHeight()-26)/(hi-lo+1),length=d["length"].get<float>();g.setFont(12);for(int b=0;b<=int(length/384);b++){float x=roll.getX()+float(roll.getWidth())*b*384/length;g.setColour(juce::Colours::white.withAlpha(.3f));g.drawVerticalLine(int(x),roll.getY(),roll.getBottom());g.setColour(juce::Colours::white);if(b<int(length/384))g.drawText(juce::String(b+1),int(x)+4,roll.getY()+2,32,20,juce::Justification::left);}int i=0;for(auto&n:d["notes"]){float x=roll.getX()+n[0].get<float>()/length*roll.getWidth(),y=roll.getY()+25+(hi-n[2].get<int>())*row,width=std::max(2.f,n[1].get<float>()/length*roll.getWidth());g.setColour(juce::Colours::white.withAlpha(.3f+.7f*n[3].get<float>()/127));juce::Rectangle<float>r(x,y,width,std::max(3.f,row-1));g.fillRect(r);if(i++==selected){g.setColour(juce::Colours::white);g.drawRect(r.expanded(2),2);}}}
 void mouseDown(const juce::MouseEvent&e)override{if(!roll.contains(e.getPosition()))return;auto d=workspace.snapshot();if(!MidiWorkspace::valid(d)||d["notes"].empty())return;int lo=127,hi=0;for(auto&n:d["notes"]){lo=std::min(lo,n[2].get<int>());hi=std::max(hi,n[2].get<int>());}lo=std::max(0,lo-2);hi=std::min(127,std::max(lo+12,hi+2));float row=float(roll.getHeight()-26)/(hi-lo+1),length=d["length"].get<float>();selected=-1;int i=0;for(auto&n:d["notes"]){juce::Rectangle<float>r(roll.getX()+n[0].get<float>()/length*roll.getWidth(),roll.getY()+25+(hi-n[2].get<int>())*row,std::max(2.f,n[1].get<float>()/length*roll.getWidth()),std::max(3.f,row-1));if(r.contains(e.position)){selected=i;velocity.setValue(n[3].get<int>(),juce::dontSendNotification);break;}i++;}velocity.setEnabled(selected>=0);repaint();}
};
}
