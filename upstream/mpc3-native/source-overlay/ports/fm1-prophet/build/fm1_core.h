#define __attribute__(x)
#define memset melodee_memset
#define memcpy melodee_memcpy
#define memcmp melodee_memcmp
#include "melodee_tables.h"
#include "firmware/src/libc.c"
#undef memset
#undef memcpy
#undef memcmp
#undef __attribute__                                  /* the RAM tables the firmware fills at boot (fm1_main) */
__attribute__((constructor)) static void host_dsp_tables(void) { dsp_tables_init(); }
#define __attribute__(x)
static struct { volatile uint32_t notes, buttons; } fm1_in;
/* Serial host fixtures retain their established admission policy. Dedicated
 * resident/dual-core tests select the target policy explicitly. */
#ifndef MELODEE_RAM_AUDIO
#define MELODEE_RAM_AUDIO 0
#endif
#include "firmware/src/core.h"
#include "firmware/src/engines.c"
/* the packed factory banks, unpacked for comparisons (four slots each, so a few can be held at once) */
static const p5_patch_t *p5_fac(uint32_t k)
{
    static p5_patch_t s[4];
    static uint32_t n;
    p5_patch_t *p = &s[n++ % 4u];
    p5_factory(k, p);
    return p;
}
static const uint8_t *cz_fac(uint32_t k)
{
    static uint8_t s[4][CZ_BYTES];
    static uint32_t n;
    uint8_t *p = s[n++ % 4u];
    cz_factory(k, p);
    return p;
}
#include "firmware/src/params.c"
#include "firmware/src/mod.c"
#include "firmware/src/voice.c"
#include "firmware/src/mix.c"
#undef __attribute__
