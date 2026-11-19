static uint32_t scale_mask(const track_t *t)
{
    const micro_scale_t *s = micro_scale(t);
    if (s) { /* approximate pitch classes for the keyboard LEDs only */
        uint32_t mask = 0;
        for (uint32_t i = 0; i < s->count; i++)
            mask |= 1u << ((s->pitch[i] + 5000) / 10000 % 12);
        return mask;
    }
    return SCALE_MASK[clamp(t->p[P_SCALE], 0, SCALE_LEGACY - 1)];
}

static uint32_t scale_count(const track_t *t)
{
    if (micro_scale(t)) return micro_scale(t)->count;
    uint32_t mask = scale_mask(t), count = 0;
    while (mask) {
        count += mask & 1u;
        mask >>= 1;
    }
    return count;
}

static void host_preset_values(track_t *t, uint32_t e, uint32_t pi, const preset_t *p)
{
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
    cz_factory_loaded(t);
    if(e==ENGI_PROPHET){uint32_t k=(uint32_t)(t-trk)%NPART;p5_ready[k]=1;p5_preset_loaded(t,pi);}   /* (its program, as ui.c) */
    fm6_track_loaded(t);                          /* FM6: the preset's patch (ui.c apply_preset_to) */
}

static void host_preset_req(track_t *t, uint32_t e, uint32_t pi)
{
    uint32_t n = ENGINES[e]->npresets;
    if (!n) {
        t->eng_req = (uint8_t)e;
        t->preset = 0;
        for (uint32_t i = 0; i < 8u; i++)
            t->p[P_E0 + i] = ENGINES[e]->edit[i].def;
        return;
    }
    pi %= n;
    host_preset_values(t, e, pi, eng_preset(ENGINES[e], pi));
}

static void host_preset(track_t *t, uint32_t e, uint32_t pi)
{
    host_preset_req(t, e, pi);
    if (t->engine != t->eng_req && t >= &trk[0] && t < &trk[NPART])
        eng_state_clear((uint32_t)(t - trk));     /* as voice.c engine_block on a switch */
    t->engine = t->eng_req;
}
