#define __attribute__(x)
#define memset felucca_memset
#define memcpy felucca_memcpy
#define memcmp felucca_memcmp
#include "felucca_tables.h"
#include "firmware/src/libc.c"
#undef memset
#undef memcpy
#undef memcmp
static struct { volatile uint32_t notes, buttons; } fm1_in;
#include "firmware/src/core.h"
#include "firmware/src/engines.c"
#include "firmware/src/params.c"
#include "firmware/src/mod.c"
#include "firmware/src/voice.c"
#include "firmware/src/slicer.c"
#include "firmware/src/fx.c"
#undef __attribute__
