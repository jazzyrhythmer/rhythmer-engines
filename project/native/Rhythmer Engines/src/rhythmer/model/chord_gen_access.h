// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "rhythmer/model/project.h"
#include "rhythmer/model/scales.h"
#include <string>
namespace rhythmer {
inline bool chord_gen_available(const Project& p,unsigned at){
 const auto& t=p.tracks[at%track_count];
 if(t.break_bars||t.engine==EngineKind::None||t.engine==EngineKind::Audio)return false;
 if(t.engine==EngineKind::Plugin){if(t.plugin>=max_plugins||!p.plugins[t.plugin].used())return false;const std::string n=p.plugins[t.plugin].name;return n!="Acid Engine"&&n!="Rhythmer Kit"&&n!="Groove Kit"&&n!="Break Engine";}
 return t.engine==EngineKind::Midi||t.engine==EngineKind::Keygroup||(t.engine==EngineKind::Sampler&&!drum_pads(t));
}
}
