#define __attribute__(x)
#define memset felucca_memset
#define memcpy felucca_memcpy
#define memcmp felucca_memcmp
#include "felucca_tables.h"
#include "firmware/src/libc.c"
#undef memset
#undef memcpy
#undef memcmp
#ifndef HOSTSIM_NO_FM1_IN                    /* (the SMK-37 emulator brings its own: web/emu/smk_web_input.h) */
static struct { volatile uint32_t notes, buttons; } fm1_in;
#endif
#include "firmware/src/core.h"
#include "firmware/src/engines.c"
#include "firmware/src/fmini_rec.c"
#if !FELUCCA_FLASH                           /* (the flash build, the emulator: fmini_bank.c after storage.c, felucca_web.c) */
#include "firmware/src/fmini_bank.c"
#endif
#if !FELUCCA_FLASH
/* every host test: the FMini RAM presets exist before main (the power-on fmini_init, fb_boot, in the device) */
#undef __attribute__                          /* (this file drops the sources' section attributes: keep this one) */
__attribute__((constructor)) static void hostsim_fmini_presets(void) { fb_names_init(); }
#define __attribute__(x)
#endif
#include "firmware/src/params.c"
#include "firmware/src/mod.c"
#include "firmware/src/voice.c"
#include "firmware/src/slicer.c"
#include "firmware/src/fx.c"
#undef __attribute__
