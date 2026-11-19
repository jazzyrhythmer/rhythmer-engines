// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "rhythmer/engine/sample_data.h"
#include "rhythmer/model/sampler.h"
#include <memory>
#include <atomic>
namespace rhythmer {
class BreakManipulator {
public:
    BreakManipulator();~BreakManipulator();
    void configure(unsigned bars,unsigned bank);
    bool feedback(unsigned& version,unsigned& variation,unsigned& drift)const{
        auto before=feedback_version.load(std::memory_order_acquire);variation=variation_view.load(std::memory_order_relaxed);drift=drift_view.load(std::memory_order_relaxed);
        auto after=feedback_version.load(std::memory_order_acquire);if(before!=after||after==version)return false;version=after;return true;
    }
    unsigned activity()const{return activity_.load(std::memory_order_relaxed);}
    unsigned source()const{return source_.load(std::memory_order_relaxed);}
    void initial_source(unsigned value){source_=value%2;bind();}
    unsigned first_pad()const{return bank_*16;}
    bool enabled()const{return bars_!=0;}
    void sources(const SampleData* a,const SampleData* b);
    void tempo(unsigned tenths);
    void transport(bool playing,unsigned ticks);
    void note(int key,int velocity);void off(int key);void release();void stop();
    void param(unsigned id,int value);
    void render(float*,float*,unsigned frames,float rate);
    bool sounding()const;
private:
    struct State;std::unique_ptr<State> state;
    bool transport_playing=false;
    unsigned bars_=0,bank_=0;std::atomic<unsigned> source_{0};
    std::atomic<unsigned> feedback_version{0},variation_view{0},drift_view{0};
    int controls[10]{0,0,0,64,0,0,38,100,4,1};
    const SampleData* loops[2]{};
    std::atomic<unsigned> activity_{0};
    unsigned filter_age=0,filter_q_age=0,filter_q=8;
    double tick_phase=0,phase=0;unsigned tempo_=1200,read=32;
    int32_t output[64]{};float previous=0,next=0,gain=1;
    uint8_t live_order[16]{};unsigned live_count=0;
    void bind();float frame();
};
}
