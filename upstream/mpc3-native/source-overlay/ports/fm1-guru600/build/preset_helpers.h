static void host_tracks_init(void)                /* as felucca_init: defaults, empty patterns */
{
    uint32_t i, k;
    for (i = 0; i < G_COUNT; i++)
        song.g[i] = GP[i].def;
    song.g[G_RTYPE] = 0;                          /* ROOM, the power-on reverb until 1.2: the renders (goldens) as
                                                   * before, ROOM bit for bit; HALL, the default since: reverb_test.c */
    for (k = 0; k < NTRK; k++) {
        for (i = 0; i < P_E0; i++)
            trk[k].p[i] = TP[i].def;
        for (i = 0; i < NSTEP; i++)
            trk[k].step[i].time = ST_REST;
    }
    song.master_q12 = 4096;
}

static void host_preset_values(track_t *t, uint32_t e, uint32_t pi, const preset_t *p)
{
    static const uint8_t FX_DEF[4] = {0, 24, 28, 36};
    uint32_t i;
    t->eng_req = (uint8_t)e;
    t->preset = (uint8_t)pi;
    for (i = 0; i < 8u; i++)
        t->p[P_E0 + i] = p->e[i];
    t->p[P_ATK] = p->env[0];
    t->p[P_DEC] = p->env[1];
    t->p[P_SUS] = p->env[2];
    t->p[P_REL] = p->env[3];
    t->p[P_ED_FLT] = p->fenv;
    t->p[P_VOICE] = p->mono ? V_LEGATO : V_POLY;
    for (i = 0; i < 4u; i++)
        t->p[P_DIST + i] = (int16_t)(p->fx[i] ? p->fx[i] - 1 : FX_DEF[i]);
    fm6_track_loaded(t);                          /* FM6: the preset's patch (ui.c apply_preset_to) */
}

static void host_preset_req(track_t *t, uint32_t e, uint32_t pi)
{
    uint32_t n = ENGINES[e]->npresets;
#if !FELUCCA_FM4
    if (e == ENGI_DIGITAL) {                      /* DIGITAL preset pi (retired): converted, as ui.c fm4_load_preset */
        uint8_t v[FP_SIZE + 1u];
        uint32_t tr = (uint32_t)(t - trk) % NTRK;
        fm4_preset_values(t->p, pi);
        t->preset = (uint8_t)fm4_convert(t->p, v);
        t->eng_req = ENGI_FM6;
        fm6_set_patch(tr, v);
        fm6_slot[tr] = (uint8_t)t->p[P_E7];
        return;
    }
#endif
    if (!n) {
        t->eng_req = (uint8_t)e;
        t->preset = 0;
        for (uint32_t i = 0; i < 8u; i++)
            t->p[P_E0 + i] = ENGINES[e]->edit[i].def;
        return;
    }
    pi %= n;
    host_preset_values(t, e, pi, &ENGINES[e]->presets[pi]);
}

static void host_preset(track_t *t, uint32_t e, uint32_t pi)
{
    host_preset_req(t, e, pi);
    t->engine = t->eng_req;                       /* (DIGITAL without FELUCCA_FM4: FM6) */
    eng_mem_claim(t);                             /* (voice.c engine_block would at the next block) */
}
