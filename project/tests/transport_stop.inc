// SPDX-License-Identifier: AGPL-3.0-or-later
static void stopCheck(juce::AudioProcessor&p,Head&head,const juce::String&format){
 auto block=[&](juce::MidiBuffer&m,int n=512){juce::AudioBuffer<float>b(2,n);b.clear();p.processBlock(b,m);head.ppq+=n*2/head.rate;return b;};
 auto stopped=[&]{head.playing=false;int last=-1,frame=0;float latePeak=0;for(int k=0;k<90;k++){juce::MidiBuffer m;auto b=block(m);for(int c=0;c<2;c++)for(int n=0;n<b.getNumSamples();n++){float v=std::abs(b.getSample(c,n));require(std::isfinite(v),"Stop produced nonfinite audio");if(v>1e-7f)last=std::max(last,frame+n);if(frame+n>p.getLatencySamples()+256)latePeak=std::max(latePeak,v);}frame+=b.getNumSamples();}require(latePeak<1e-7f,"DAW STOP leaves audio active: "+format+" peak="+juce::String(latePeak,9));head.playing=true;for(int k=0;k<8;k++){juce::MidiBuffer m;auto b=block(m);require(b.getMagnitude(0,b.getNumSamples())<1e-7f,"Restart without MIDI replays held notes");}head.playing=false;return last;};
 int cases=0;const int engines=param(p,1)->getNumSteps();require(engines==22,"Engine stop coverage");
 for(double rate:{44100.,48000.,96000.}){head.rate=rate;head.playing=false;p.releaseResources();p.prepareToPlay(rate,512);
 for(int engine=0;engine<engines;engine++)for(bool sustain:{false,true}){param(p,1)->setValueNotifyingHost(engine/float(engines-1));ready(p);param(p,2)->setValueNotifyingHost(0);ready(p);head.playing=true;juce::MidiBuffer m;if(sustain)m.addEvent(juce::MidiMessage::controllerEvent(1,64,127),0);int root=engine==0?36:60;for(int n:{root,root+2,root+7})m.addEvent(juce::MidiMessage::noteOn(1,n,uint8_t(112)),0);for(int k=0;k<36;k++){block(m);m.clear();if(sustain&&k==18)for(int n:{root,root+2,root+7})m.addEvent(juce::MidiMessage::noteOff(1,n),0);}int last=stopped();cases++;std::cout<<format<<" STOP engine="<<engine<<" rate="<<rate<<" sustain="<<sustain<<" last_frame="<<last<<std::endl;}
 }
 // Stop exactly when a newly loaded graph takes over, while old notes are held.
 head.rate=44100;head.playing=false;p.releaseResources();p.prepareToPlay(44100,512);param(p,1)->setValueNotifyingHost(1/float(engines-1));ready(p);head.playing=true;juce::MidiBuffer m;m.addEvent(juce::MidiMessage::noteOn(1,60,uint8_t(112)),0);block(m);param(p,1)->setValueNotifyingHost(18/float(engines-1));
 bool loaded=false;for(int k=0;k<2000;k++){if(param(p,26)->getCurrentValueAsText()=="READY"){loaded=true;break;}pump(5);}require(loaded,"Swap-stop load timed out");stopped();cases++;
 param(p,27)->setValueNotifyingHost(1);head.playing=false;{juce::MidiBuffer empty;block(empty);}
 std::cout<<format<<" TRANSPORT_STOP_PASS cases="<<cases<<" held_chords,sustain,three_rates,restart_without_notes,graph_swap"<<std::endl;
}
