// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
namespace rhyvisual {
struct Face{float xy[8],fill,stroke;};
struct Signal{int kind=0,note=60,held=0,pad=0,strike=0;uint32_t now=0;float level=0,width=480,height=320;const char* preset="";};
struct Context;
Context*create();void destroy(Context*);
const Face*frame(Context*,const Signal&,unsigned& count);
}
