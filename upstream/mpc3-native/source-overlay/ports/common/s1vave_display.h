/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 Andrew GG. Original s1vave value formatter. */
static void fmt_u(char *s, uint32_t v)          /* decimal, no leading zeros */
{
    char b[12];
    uint32_t i = 0, j = 0;
    do { b[i++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    while (i) s[j++] = b[--i];
    s[j] = 0;
}
static uint32_t slen(const char *s) { uint32_t n = 0; while (s[n]) n++; return n; }
static void scat(char *d, const char *s) { d += slen(d); while ((*d++ = *s++)) ; }
static void fmt_ms(char *s, uint32_t ms_x10)
{
    char t[4];
    if (ms_x10 < 1000u) { fmt_u(s, ms_x10 / 10u); scat(s, "."); fmt_u(t, ms_x10 % 10u); scat(s, t); scat(s, "ms"); }
    else if (ms_x10 < 10000u) { fmt_u(s, ms_x10 / 10u); scat(s, "ms"); }
    else { fmt_u(s, ms_x10 / 10000u); scat(s, "."); fmt_u(t, (ms_x10 / 1000u) % 10u); scat(s, t); scat(s, "s"); }
}
#define ed_patch(p) (&live)
static void param_value(char *s, uint32_t p, uint32_t v)   /* <= 7 characters */
{
    char t[8];
    s[0] = 0;
    switch (PDEF[p].kind) {
    case K_BIP: if (v < 64u) { scat(s, "-"); fmt_u(t, 64u - v); } else { scat(s, v > 64u ? "+" : ""); fmt_u(t, v - 64u); } scat(s, t); break;
    case K_ONOFF: scat(s, v ? "ON" : "OFF"); break;
    case K_RANGE: scat(s, RANGE_NAME[v % 6u]); break;
    case K_SUBTYPE: scat(s, SUB_NAME[v % 3u]); break;
    case K_NOISEM: scat(s, NOISEM_NAME[v % 2u]); break;
    case K_PWMS: scat(s, PWMS_NAME[v % 3u]); break;
    case K_DRAWSW: scat(s, DRAWSW_NAME[v % 3u]); break;
    case K_MULT: fmt_u(s, v + 1u); scat(s, "x"); break;
    case K_OVTN: fmt_u(s, v); break;
    case K_COMB: fmt_u(s, v + 1u); scat(s, "x"); break;
    case K_ENVTRG: scat(s, ENVTRG_NAME[v % 3u]); break;
    case K_AMP: scat(s, AMP_NAME[v % 2u]); break;
    case K_LFOWAVE: scat(s, LFOWAVE_NAME[v % 6u]); break;
    case K_RISERM: scat(s, RISER_NAME[v % 4u]); break;
    case K_CHOM: scat(s, CHORUS_NAME[v % 5u]); break;
    case K_REVM: scat(s, REVERB_NAME[v % 7u]); break;
    case K_PMODE: scat(s, POLY_NAME[v % NPOLY_MODE]); break;
    case K_PORTAM: scat(s, PORTA_NAME[v % 3u]); break;
    case K_FXMODE: scat(s, FXMODE_NAME[v % 2u]); break;
    case K_SEMI: { uint32_t cen = PDEF[p].def; if (v < cen) { scat(s, "-"); fmt_u(t, cen - v); } else { scat(s, v > cen ? "+" : ""); fmt_u(t, v - cen); } scat(s, t); break; }
    case K_MS:
        if (p == P_PORTA_TIME) { if (!v) scat(s, "OFF"); else { fmt_u(s, GLIDE_MS[v & 127u]); scat(s, "ms"); } }
        else if (p == P_ATTACK) fmt_ms(s, ATT_MS_X10[v & 127u]);
        else if (p == P_DECAY) fmt_ms(s, DEC_MS_X10[v & 127u]);
        else if (p == P_RELEASE) fmt_ms(s, REL_MS_X10[v & 127u]);
        else if (p == P_REV_PRE) { fmt_u(s, v); scat(s, "ms"); }
        else fmt_ms(s, TIME_MS_X10[v & 127u]);
        break;
    case K_HZ:
        if (p == P_LFO_RATE && ed_patch(P_LFO_RATE)->v[P_LFO_SYNC]) {
            uint32_t slot = v < 31u ? v : (v * 30u) / 127u;
            scat(s, LFO_SYNC_NAME[slot % 31u]);
        } else {
            uint32_t h = LFO_HZ_X100[v & 127u];
            if (p == P_LFO_RATE && ed_patch(P_LFO_RATE)->v[P_LFO_MODE]) h = (h * 11u) >> 2;
            if (h >= 10000u) {
                fmt_u(s, h / 100u); scat(s, "Hz");
            } else {
                fmt_u(s, h / 100u); scat(s, "."); fmt_u(t, (h / 10u) % 10u); scat(s, t); scat(s, "Hz");
            }
        }
        break;
    case K_PCT: fmt_u(s, (v * 100u + 63u) / 127u); scat(s, "%"); break;
    case K_CUT: {
        uint32_t semi = (uint32_t)CUTOFF_TAPER[v & 127u] >> 8;
        if (semi > 136u) semi = 136u;
        uint32_t hz = CUT_HZ[semi];
        if (hz < 10000u) { fmt_u(s, hz); scat(s, "Hz"); }
        else { fmt_u(s, hz / 1000u); scat(s, "."); fmt_u(t, (hz / 100u) % 10u); scat(s, t); scat(s, "k"); }
    } break;
    case K_DDIV: {
        if (ed_patch(P_DLY_TIME)->v[P_DLY_SYNC]) {
            uint32_t div = (v * 16u) / 128u;
            if (div >= NDDIV) div = NDDIV - 1u;
            scat(s, DDIV_NAME[div]);
        } else {
            static const uint8_t ANCHOR_RAW[8] = {0, 11, 26, 55, 106, 179, 245, 255};
            static const uint16_t ANCHOR_MS[8] = {1, 14, 44, 103, 273, 517, 737, 740};
            uint32_t raw = (uint32_t)v * 255u / 127u;
            uint32_t ms = 740;
            for (uint32_t k = 0; k < 7u; k++) {
                if (raw <= ANCHOR_RAW[k + 1]) {
                    uint32_t num = raw - ANCHOR_RAW[k];
                    uint32_t den = ANCHOR_RAW[k + 1] - ANCHOR_RAW[k];
                    ms = ANCHOR_MS[k] + (num * (ANCHOR_MS[k + 1] - ANCHOR_MS[k])) / den;
                    break;
                }
            }
            fmt_u(s, ms); scat(s, "ms");
        }
    } break;
    case K_FLTCOL: scat(s, FLTCOL_NAME[v % 2u]); break;
    default: fmt_u(s, v);
    }
}
#undef ed_patch
