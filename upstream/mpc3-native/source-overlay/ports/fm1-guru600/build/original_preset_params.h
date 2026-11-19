/* GPL-3.0-only: original Guru-600 preset loading */
static int param_kept(uint32_t i)
{
    return i == P_LEVEL || i == P_PAN || i == P_MUTE || (i >= P_AMODE && i <= P_SGATE) ||
           (i >= P_SLCR && i <= P_SLDEPTH) || i == P_CHRD || i == P_VOIC || i == P_SQNT ||
           (i >= P_FM4_DEC && i <= P_FM4_REL);         /* (Guru-600: CHRD MEM's chord, chord.c P_CMEM) */
}
static void preset_params(uint32_t eng, uint32_t pi, int16_t *p)
{
    const engine_t *e = ENGINES[eng % NENGINES];
    uint32_t i;
    for (i = 0; i < P_E0; i++)                        /* the rest of the sound to its defaults: a preset */
        if (!param_kept(i))                           /* sounds the same after any edit */
            p[i] = TP[i].def;
    for (i = 0; i < 8u; i++)
        p[P_E0 + i] = (int16_t)e->presets[pi].e[i];
    p[P_ATK] = e->presets[pi].env[0];
    p[P_DEC] = e->presets[pi].env[1];
    p[P_SUS] = e->presets[pi].env[2];
    p[P_REL] = e->presets[pi].env[3];
    p[P_ED_FLT] = e->presets[pi].fenv;
    p[P_VOICE] = e->presets[pi].mono ? V_LEGATO : V_POLY;   /* mono presets keep the legato feel */
    {   /* the sends */
        static const uint8_t FX_DEF[4] = {0, 24, 28, 36};
        const preset_t *pr = &e->presets[pi];
        for (i = 0; i < 4u; i++)
            p[P_DIST + i] = (int16_t)(pr->fx[i] ? pr->fx[i] - 1 : FX_DEF[i]);
    }
#if FELUCCA_P600
    if (eng % NENGINES == ENGI_P600) {                /* the rest of a Prophet-600 program (eng_p600.c) */
        p600_preset(p, pi);
        p[P_M3SRC] = MS_VEL;                          /* EXTRAS: VEL > FLT (its amount the program's) */
        p[P_M3DST] = MD_CUT;
        p[P_M4SRC] = MS_MODW;                         /* .. WHEEL: the mod wheel adds to the LFO on the program's own
                                                         * destinations, as the original's (its amount the program's) */
        p[P_M4DST] = MD_DEPTH;
    }
#endif
}
