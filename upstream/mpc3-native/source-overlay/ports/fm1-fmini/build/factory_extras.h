/* GPL-3.0-only: original FMini sound loader; device ARP/octave excluded. */
static const uint8_t FA_XG[12] = {G_RTYPE, G_RSIZE, G_RDAMP, G_DTIME, G_DRREV, G_DFDBK, G_ENGGO, G_CDEPTH, G_CRATE, G_INFO, G_DRLVL, G_MIDI};
static void fa_factory_extras(track_t *t, uint32_t n)
{
    fmini_x_t xs, *x = &xs;
    uint32_t i;
    if (n >= FMINI_NSLOT)
        return;
    fb_extra(n, x);                             /* the slot's record (saved or factory) */
    {   uint8_t bnd, wh, frl, arl, vs;          /* the preset's PITCH BEND range, MOD WHEEL position, RELEASEs, VELOCITY SENS. */
        uint32_t l = fb_pset(n, &bnd, &wh, &frl, &arl, &vs);
        t->mw = wh;
        if (t == &trk[0]) {
            bend_mode = bnd;
            fm_vel_sens = vs;
        }
        if (l >= 4u) {                          /* the saved releases */
            t->frel = (uint8_t)(0x80u | frl);
            t->p[P_REL] = (int16_t)arl;
        } else {                                /* an older preset: the RELEASEs are its DECAY, as they always sounded */
            t->frel = 0;
            t->p[P_REL] = t->p[P_DEC];
        }
    }
    t->p[P_LEVEL] = x->level;
    t->p[P_LRATE] = x->lfo[0];
    t->p[P_LWAVE] = x->lfo[1];
    t->p[P_LFADE] = x->lfo[2];
    t->p[P_LSYNC] = x->lfo[3];
    t->p[P_LD_PIT] = x->mod[0];
    t->p[P_LD_AMP] = x->mod[1];
    t->p[P_ED_PIT] = x->mod[2];
    if (t != &trk[0])
        return;
    for (i = 0; i < 12u; i++)
        song.g[FA_XG[i]] = (int16_t)clamp(x->fx[i], GP[FA_XG[i]].min, GP[FA_XG[i]].max);
}
