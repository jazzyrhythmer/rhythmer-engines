// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
// Factory kit adaptation by Jazzy Rhythmer.
// Reuses the zero-lookahead Limiter branch of Melodee engine/effects.cpp.
// Stereo-linked, no latency, allocation or locks. Unity below the ceiling.
#pragma once
#include <algorithm>
#include <cmath>
namespace rhythmer {
class KitOutputStage {
 float gain_=1,release_=std::exp(-1.f/(.075f*44100.f));unsigned epoch_=0;
public:
 void prepare(float rate){release_=std::exp(-1.f/(.075f*std::max(8000.f,rate)));gain_=1;epoch_=0;}
 void process(float*l,float*r,unsigned frames,unsigned epoch=0){
  if(epoch!=epoch_){gain_=1;epoch_=epoch;}
  constexpr float ceiling=.92f;
  for(unsigned i=0;i<frames;i++){
   if(!std::isfinite(l[i])||!std::isfinite(r[i])){l[i]=r[i]=0;continue;}
   float level=std::max(std::fabs(l[i]),std::fabs(r[i]));float target=level>ceiling?ceiling/level:1.f;
   gain_=target<gain_?target:target+(gain_-target)*release_;
   l[i]*=gain_;r[i]*=gain_;
  }
 }
};
}
