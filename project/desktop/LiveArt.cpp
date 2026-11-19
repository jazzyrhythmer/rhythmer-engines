// SPDX-License-Identifier: AGPL-3.0-only
#include "LiveArt.h"
// The freestanding adapter's small std/juce shims are isolated from the real
// JUCE ABI. Each editor owns its original Motion and KitPose state.
namespace original_art {
#include "../src/animation/mini.h"
struct Renderer {
#include "desktop_art_context.inc"
};
}
namespace rhyvisual {
struct Context{original_art::Renderer renderer;Face faces[384]{};};
Context*create(){return new Context;}
void destroy(Context*c){delete c;}
const Face*frame(Context*c,const Signal& in,unsigned& count){
 using namespace original_art;
 static const char*names[]={"Sophie","Aphex","Elixir","FM-106","FMini-1","CS-1","OberVave","K-WASUB","Hera","Braids","Plaits","Hortator","MONO2K","Prophet","S1Vave","Dexed","Guru-600","Acid Engine","Rhythmer Kit","Break Engine","Frodhes","Tenebrae"};
 if(in.strike){c->renderer.motion[0].held=false;c->renderer.kit_poses[0].previous_held.fill(-1);}
 polygon_count=0;ArtSignal signal;signal.now=in.now;signal.part.last_note=in.note;signal.held_keys.fill(-1);if(in.held)signal.held_keys[0]=in.note;signal.meters[0]=in.level;
 ui::Paint painter;c->renderer.performance_art(painter,signal,names[original_art::std::clamp(in.kind,0,21)],juce::String(in.preset),0,0,in.width,in.height,in.pad,1,true);
 count=original_art::std::min(polygon_count,384u);
 for(unsigned i=0;i<count;i++){for(int p=0;p<8;p++)c->faces[i].xy[p]=polygons[i].xy[p];c->faces[i].fill=polygons[i].fill;c->faces[i].stroke=polygons[i].stroke;}
 return c->faces;
}
}
