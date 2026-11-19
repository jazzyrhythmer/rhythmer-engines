// SPDX-License-Identifier: GPL-3.0-only
// Per-track state around the pinned FM-1 Amenity/Jungle DSP. No audio allocations.
#include "rhythmer/engine/break_manipulator.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "rhythmer/engine/jungle_state.inc"
#pragma GCC diagnostic pop
#include <algorithm>
#include <cmath>
namespace rhythmer {
BreakManipulator::BreakManipulator():state(new State{}){state->jungle_init();state->jungle_sequence_mode(controls[9]);}
BreakManipulator::~BreakManipulator()=default;
void BreakManipulator::configure(unsigned bars,unsigned bank){
    bars=std::min(bars,4u);bank%=sampler_banks;
    if(bars_==bars&&bank_==bank)return;
    stop();bars_=bars;bank_=bank;
    state->source_bars=bars?bars:2;bind();
}
void BreakManipulator::tempo(unsigned t){tempo_=std::clamp(t,200u,4000u);state->jungle_tempo(tempo_);}
void BreakManipulator::transport(bool playing,unsigned ticks){
    if(playing&&!transport_playing){state->jungle_start_phase(ticks*2);tick_phase=0;}
    transport_playing=playing;state->jungle_play(playing||state->six_seq_gates||live_count);
}
void BreakManipulator::bind(){
    const auto* s=loops[source_];if(!s||s->frames<16||s->multi){state->jungle_bind(nullptr,0,0);return;}
    state->jungle_bind(reinterpret_cast<const uint8_t*>(s),0,48000);state->jungle_sequence_mode(controls[9]);
}
void BreakManipulator::sources(const SampleData* a,const SampleData* b){
    if(a==loops[0]&&b==loops[1])return;
    // The host retires source data only after sample_gone/silence on the audio task.
    stop();loops[0]=a;loops[1]=b;bind();
}
void BreakManipulator::note(int key,int velocity){
    if(!enabled())return;
    if(key==16||key==17){unsigned which=unsigned(key-16);if(!loops[which])return;
        if(source_!=which){source_=which;auto* s=loops[which];
            state->jungle_swap(reinterpret_cast<const uint8_t*>(s));}
        return;
    }
    if(key==18){param(1,0);param(2,0);state->jungle_straight();return;}
    if(key>=0&&key<16){
        // PERF commands are switches. Leave the current chop gain untouched;
        // keep them independent from generated sequence notes and their releases.
        for(unsigned i=0;i<live_count;i++)if(live_order[i]==key){for(unsigned j=i+1;j<live_count;j++)live_order[j-1]=live_order[j];--live_count;break;}
        live_order[live_count++]=uint8_t(key);state->jungle_roll(unsigned(key));
        // Stretch also starts from a manually triggered chop, without waiting
        // for a later sequencer note. This does not add a held slice gate.
        if(key==2)state->six_seq_hit=state->last_slice;
        state->jungle_play(1);return;
    }
    int slice=key-int(sampler_first_note+bank_*16);if(slice<0||slice>=16)return;
    gain=std::clamp(velocity,1,127)/127.f;state->jungle_play(1);state->jungle_sequence_note(unsigned(slice));
}
void BreakManipulator::off(int key){
    if(key>=0&&key<16){
        for(unsigned i=0;i<live_count;i++)if(live_order[i]==key){for(unsigned j=i+1;j<live_count;j++)live_order[j-1]=live_order[j];--live_count;break;}
        if(live_count)state->jungle_roll(live_order[live_count-1]);
        else{state->jungle_roll_off();state->six_seq_release=1;}
    }
    else{int slice=key-int(sampler_first_note+bank_*16);if(slice>=0&&slice<16)state->jungle_sequence_off(unsigned(slice));}
    if(!transport_playing&&!state->six_seq_gates&&!live_count)state->jungle_play(0);
}
void BreakManipulator::release(){live_count=0;activity_=0;transport_playing=false;state->jungle_roll_off();state->jungle_sequence_off(32);state->jungle_play(0);}
void BreakManipulator::stop(){live_count=0;gain=1;activity_=0;transport_playing=false;state->jungle_init();state->view_playing=state->view_ext=0;filter_age=filter_q_age=0;filter_q=8;state->jungle_filter_resonance(8);state->jungle_sequence_mode(1);for(unsigned i=0;i<10;i++)param(i,controls[i]);tick_phase=phase=0;read=32;previous=next=0;std::fill(std::begin(output),std::end(output),0);}
void BreakManipulator::param(unsigned id,int value){if(id>=10)return;
    if(id==3&&controls[3]!=value){filter_age=filter_q_age=0;filter_q=14;state->jungle_filter_resonance(14);}
    controls[id]=value;
    if(id==1||id==2){(id==1?variation_view:drift_view).store(unsigned(value),std::memory_order_relaxed);feedback_version.fetch_add(1,std::memory_order_release);}
    switch(id){case 0:state->jungle_feedback(value);break;case 1:state->jungle_variation(value);break;case 2:state->jungle_warp(value);break;case 3:state->jungle_filter(value);break;case 4:state->jungle_freeze(value);break;case 5:state->jungle_auto_roll(value);break;case 6:case 7:state->jungle_stretch_grain(controls[6],controls[7]);break;case 8:state->jungle_stretch_slow(value);break;case 9:state->jungle_sequence_mode(value);break;}}
float BreakManipulator::frame(){
    if(read==32){uint32_t mask=0;
        filter_age=std::min(filter_age+32,44100u);filter_q_age+=32;
        if(filter_age>=22050&&filter_q>8&&filter_q_age>=1323){filter_q_age=0;state->jungle_filter_resonance(--filter_q);}
        for(unsigned i=0;i<32;i++){tick_phase+=double(tempo_)*4*48;if(tick_phase>=44100.0*600){tick_phase-=44100.0*600;mask|=uint32_t(1)<<i;}}
        std::fill(std::begin(output),std::end(output),0);state->jungle_tick_mask(mask);state->jungle_render(output,32);read=0;}
    return output[2*read++]/8388608.f;
}
void BreakManipulator::render(float* l,float* r,unsigned frames,float rate){
    const double step=44100.0/std::max(8000.f,rate);
    for(unsigned i=0;i<frames;i++){phase+=step;while(phase>=1){previous=next;next=frame();phase-=1;}
        l[i]=r[i]=gain*(previous+float(phase)*(next-previous));}
    activity_.store((transport_playing?1u:0u)|(state->six_seq_gates?2u:0u)|((state->ctl_roll==3||state->six_seq_roll==3||state->roll_now==3)?4u:0u)|(live_count?8u:0u),std::memory_order_relaxed);
}
bool BreakManipulator::sounding()const{return enabled()&&(state->jungle_playing()||state->voices[0].active||state->voices[1].active);}
}
