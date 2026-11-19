// SPDX-License-Identifier: GPL-3.0-only
#include "rhythmer/model/acid_gen.h"
#include "rhythmer/shell/fm1_gen_bridge.h"
#include <algorithm>
namespace rhythmer {
Pattern acid_generate_pattern(const Pattern& source,AcidGenSettings& s,bool mutate,unsigned root,uint16_t scale){
    Pattern out=source;out.events.clear();out.euclids.clear();
    const unsigned grid=source.q();
    const unsigned count=std::clamp<unsigned>(source.steps(),1,64);
    uint8_t notes[64]{},flags[64]{};
    // A sustained event is NOTE followed by TIE cells, including the loop edge.
    for(const auto& e:source.events){unsigned at=e.tick/grid;if(at>=count)continue;
        unsigned cells=std::min<unsigned>(count,(e.length+grid-1)/grid);
        for(unsigned n=1;n<cells;n++){unsigned cell=(at+n)%count;notes[cell]=e.key;flags[cell]=2;}
    }
    for(const auto& e:source.events){unsigned at=e.tick/grid;if(at>=count)continue;
        notes[at]=e.key;flags[at]=1|(e.flags&step_accent?4:0)|(e.flags&step_slide?8:0);
    }
    // FM-1 six_gen_apply_to snaps existing notes DOWN before TB-3PO mutates.
    unsigned mask=scale?scale:0xfff;
    for(unsigned i=0;i<count;i++){int n=notes[i];unsigned guard=12;
        while(guard--&&!(mask&(1u<<unsigned((n-int(root%12)+120)%12))))--n;
        if(n<0)n+=12;
        notes[i]=uint8_t(std::clamp(n,0,127));
    }
    const uint8_t cfg[5]={s.density,s.accent,s.slide,s.octaves,s.octave};
    acid_generate(notes,flags,count,cfg,&s.seed,mutate,root,scale);
    for(unsigned n=0;n<count;n++)if((flags[n]&3)==1){
        Event e;e.tick=n*grid;e.key=notes[n];e.velocity=flags[n]&4?127:96;
        unsigned end=n+1;while(end<n+count&&(flags[end%count]&3)==2)++end;
        e.length=uint16_t((end-n)*grid);
        if(flags[n]&4)e.flags|=step_accent;
        if(flags[n]&8)e.flags|=step_slide;
        out.add(e);
    }
    out.sort();return out;
}
}
