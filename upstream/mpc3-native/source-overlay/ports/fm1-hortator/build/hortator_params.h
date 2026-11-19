/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Drum machine fork: 2026 DEADACTIVE */
/* Parameter descriptions and value formatting. P_E0..P_E7 are described by the track's model. */
static const char *const N_DIV[] = {"1/4", "1/8", "1/16", "1/32", "8T", "16T", "1/2", "1/1", "2BAR", "4BAR"};   /* ids fixed: slow rates appended */
static const char *const N_DTIME[] = {"1/4", "1/8", "1/16", "1/32", "8T", "16T", "1/2", "1/1", "2BAR", "4BAR",   /* the
                                      * delay's TIME: N_DIV's ids, then its own (appended: fx.c div_samples) */
                                      "4T", "1/4D", "1/8D", "1/16D", "32T"};
static const char *const N_DMODE[] = {"NORM", "PING"};               /* G_DMODE: NORMAL, PING-PONG (fx.c) */
static const char *const N_ONOFF[] = {"OFF", "ON"};
static const char *const N_CLOCK[] = {"INT", "USB", "TRS",   /* G_CLOCK: = the input ring's source (1 USB, 2 TRS, */
#if FELUCCA_BLE
                                      "BLE",                   /* 3 BLE: bt_midi.c; */
                                      "LINK",                  /* 4: FM-Link's follower, bt_fml.c) */
#endif
};
static const char *const N_NOTE[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char *const N_DASH[] = {"--"};
static const char *const N_SYNC[] = {"OFF", "USB",   /* G_SYNC: SY_ bits (midi_out.c; no TRS out on the FM-1) */
#if FELUCCA_BLE
                                     "BLE", "BOTH",
#endif
};
static const char *const N_ROUTE[] = {"NOTE", "SEL", "TRACK"};   /* G_ROUTE: RT_ (seq.c midi_block) */
static const char *const N_SLOT[] = {"-", "A", "B", "C", "D"};   /* G_SLOT 1..4: the project slots A..D */
static const char *const N_GO[] = {"--", "GO"};
static const char *const N_SLCR[] = {"OFF", "GATE", "STUT"};             /* SL_OFF .. SL_STUT (slicer.c) */
static const char *const N_SLDIV[] = {"1/8", "1/16", "1/32", "8T", "16T", "32T"};   /* SL_DEN */
static const char *const N_RTYPE[] = {"ROOM", "SPRNG", "HALL"};      /* G_RTYPE: the reverb bus's model (fx.c); SPRING in 5;
                                                                      * (Skiff) HALL 2 */
static const char *const N_CHOKE[] = {"OFF", "1", "2", "3", "4"};
static const char *const N_SRC[] = {"STEP", "G-KCK", "G-SNR", "G-HAT"};   /* P_SRC: its steps or a Grids channel */
static const char *const N_GMODE[] = {"MAP", "EUCL"};
static const char *const N_GHOST[] = {"MUTE", "KEEP", "HIDE"};
static const char *const N_CSRC[] = {"OFF", "T1", "T2", "T3", "T4", "T5", "T6", "T7", "T8"};
static const char *const N_KNEE[] = {"HARD", "SOFT"};
static const char *const N_LWAVE[] = {"SQUAR", "SAW", "RSAW", "SINE", "TRI", "S&H", "WANDR", "EXP+", "EXP-", "RWALK"};
static const char *const N_LMODE[] = {"SYNC", "HZ", "TIME"};
static const char *const N_LTRIG[] = {"FREE", "HIT", "PLAY"};
static const char *const N_LSYNC[17] = {"8BAR", "4BAR", "2BAR", "1BAR", "1/2", "1/4.", "1/4", "1/4T", "1/8.", "1/8",
                                        "1/8T", "1/16.", "1/16", "1/16T", "1/32", "1/32T", "1/64"};

static const char *const N_RMODEL[] = {"OFF", "STRNG", "PIPE", "CHORD", "MODAL"};
static const char *const N_RCHORD[RS_NCHORD + 1] = {"OCT", "5TH", "4TH", "MAJ", "MIN", "SUS2", "SUS4", "DIM", "AUG",
                                                   "MAJ6", "MIN6", "MAJ7", "MIN7", "DOM7", "M7b5", "DIM7", "7SUS4",
                                                   "ADD9", "QUART", "CLUST", 0};
static const char *const N_RDEST[] = {"R.TUN", "R.DCY", "R.MIX", "R.TON", "R.STR", "R.POS", "F.CUT", "F.RES"};   /* LFO DEST 11..18 */
static const char *const N_FTYPE[] = {"OFF", "LP", "BP", "HP", "NOT"};   /* FILTER TYPE (FT_) */
static const char *const N_PRIO[] = {"HIGH", "LOW"};                 /* P_PRIO */
static const char *const N_SLOOP[] = {"OFF", "FWD", "PING", "LOOPI", "ONE"};   /* P_SLOOP: the SAMPLE model's LOOP
                                                                     * (dm_sample.c; stored: append only) */
static const char *const N_SPLAY[] = {"KEY", "CHOP"};                /* P_SPLAY: the SAMPLE model's PLAY (dm_sample.c) */
static const char *const N_LSRC[] = {"LFO1", "LFO2"};                /* an LFO DEST slot's SRC */
static const char *const N_ITYPE[] = {"OFF", "SOFT", "HARD", "FOLD", "FUZZ", "CRUSH", "PHASR", "FLANG", "CHOR"};   /* INSERT
                                                                     * TYPE (IT_; insert.c) */
static const char *const N_IBITS[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15",
                                      "16", 0};             /* CRUSH BITS (insert.c ins_bits_sh) */
static const char *const N_IRATE[] = {"689", "919", "1.1k", "1.4k", "1.8k", "2.2k", "2.8k", "3.7k", "4.4k", "5.5k",
                                      "7.4k", "8.8k", "11k", "15k", "22k", "44k", 0};   /* CRUSH RATE (INS_HOLD) */
#define LFO_TP(b, rf)                                                                                     \
    [b + LF_WAVE] = PE("WAVE", N_LWAVE, 3), [b + LF_MODE] = PE("MODE", N_LMODE, 0),                         \
    [b + LF_RATE] = PD("RATE", rf, 0, 127, 23), [b + LF_MORPH] = PD("MORPH", F_INT, 0, 127, 0),              \
    [b + LF_DEPTH] = PD("DEPTH", F_BIPCT, -64, 64, 0), [b + LF_DEST] = PD("DEST", F_LDEST, 0, 27, 0),        \
    [b + LF_TRIG] = PE("TRIG", N_LTRIG, 2), [b + LF_PHASE] = PD("PHASE", F_LPHASE, 0, 127, 0)
static const param_desc_t TP[P_COUNT] = {
    [P_MODEL] = PE("MODEL", N_MODEL, 0),
    [P_LEVEL] = PD("LVL", F_DB, 0, 127, 104),
    [P_PAN] = PD("PAN", F_BIPCT, -64, 63, 0),
    [P_MUTE] = PE("MUTE", N_ONOFF, 0),
    [P_CHOKE] = PE("CHOKE", N_CHOKE, 0),
    [P_NOTE] = PD("NOTE", F_INT, 0, 127, 36),
    [P_DIST] = PD("DST", F_PCT, 0, 127, 0),
    [P_CHOR] = PD("CHO", F_PCT, 0, 127, 0),
    [P_DLY] = PD("DLY", F_PCT, 0, 127, 0),
    [P_REV] = PD("REV", F_PCT, 0, 127, 0),
    [P_SLCR] = PE("SLCR", N_SLCR, 0),
    [P_SLPAT] = PD("PAT", F_INT, 1, 16, 1),        /* SL_PAT[] */
    [P_SLRATE] = PE("RATE", N_SLDIV, 1),
    [P_SLDEPTH] = PD("DEPTH", F_PCT, 0, 127, 127),
    [P_SLEN] = PD("LEN", F_STEPS, 1, NSTEP, 16),
    [P_SDIV] = PE("DIV", N_DIV, 2),
    [P_SSWING] = PD("SWG", F_PCT, 0, 100, 0),
    [P_LSET] = PE("SET", N_DASH, 0),       /* the LAYER (MIDI LEARN: "T1 L.SET") */
    [P_LKEY] = PD("SLOT", F_LSLOT, -16, -1, -16),  /* dm_sample.c: SET's samples 1..N (stored -16..-1; >= 0 a KEY note) */
    [P_LLEVEL] = PD("LEVEL", F_PCT, 0, 127, 0),
    [P_LTUNE] = PD("TUNE", F_SEMI, -24, 24, 0),
    [P_LDEC] = PD("DECAY", F_INT, 0, 127, 127),
    [P_SRC] = PE("SRC", N_SRC, 0),
    [P_DUCK] = PE("DUCK", N_ONOFF, 0),
    LFO_TP(P_LFO1, F_LRATE1),
    LFO_TP(P_LFO2, F_LRATE2),
    [P_RMODEL] = PE("MODEL", N_RMODEL, 0),
    [P_RTUNE] = PD("TUNE", F_NOTEO, 24, 96, 48),
    [P_RDECAY] = PD("DECAY", F_RDECAY, 0, 127, 72),
    [P_RMIX] = PD("MIX", F_PCT, 0, 127, 64),
    [P_RTONE] = PD("TONE", F_PCT, 0, 127, 100),
    [P_RSTRCT] = PD("STRCT", F_INT, 0, 127, 0),
    [P_RPOS] = PD("POS", F_PCT, 0, 127, 64),
    [P_FTYPE] = PE("TYPE", N_FTYPE, 0),
    [P_FCUT] = PD("CUT", F_CUTOFF, 0, 127, 127),
    [P_FRESO] = PD("RESO", F_INT, 0, 127, 0),
    [P_FENV] = PD("ENV", F_INT, -64, 63, 0),
    [P_FDEC] = PD("DECAY", F_FDEC, 0, 127, 40),
    [P_PRIO] = PE("PRIO", N_PRIO, 0),
    [P_LSRC1] = PE("SRC", N_LSRC, 0), [P_LSRC2] = PE("SRC", N_LSRC, 1),   /* slots 1 / 2: LFO 1 / LFO 2 (the routing
                                                                            * before slots) */
    [P_LS3SRC] = PE("SRC", N_LSRC, 0), [P_LS3DEST] = PD("DEST", F_LDEST, 0, 27, 0),
    [P_LS3DEPTH] = PD("DEPTH", F_BIPCT, -64, 64, 0),
    [P_LS4SRC] = PE("SRC", N_LSRC, 1), [P_LS4DEST] = PD("DEST", F_LDEST, 0, 27, 0),
    [P_LS4DEPTH] = PD("DEPTH", F_BIPCT, -64, 64, 0),
    [P_RKEY] = PD("KEYF", F_PCT, 0, 127, 0),       /* RESON KEYFOLLOW: 0 off (the ring at TUNE), 127 = 100 % */
    [P_SRR] = PD("SRR", F_SRR, 0, 127, 0),         /* SRR (insert.c): 0 off (as before), else the held rate */
    [P_ITYPE] = PE("TYPE", N_ITYPE, IT_OFF),       /* INSERT: OFF as before; A B C named by the TYPE (ins_desc) */
    [P_IA] = PD("A", F_PCT, 0, 127, 64),
    [P_IB] = PD("B", F_PCT, 0, 127, 96),
    [P_IC] = PD("C", F_PCT, 0, 127, 96),
    [P_IMIX] = PD("MIX", F_PCT, 0, 127, 127),
    [P_SATK] = PD("ATK", F_INT, 0, 127, 0),        /* the SAMPLE model's envelope (dm_sample.c): 0 none (as before) */
    [P_SSUS] = PD("SUS", F_PCT, 0, 127, 0),        /* 0: one-shot, DECAY to silence (as before); else the level held */
    [P_SREL] = PD("REL", F_INT, 0, 127, 40),       /* the release once the note's gate closes (SUS above 0) */
    [P_SLOOP] = PE("LOOP", N_SLOOP, 0),            /* OFF as before; FWD, PING-PONG between START and END; LOOPI (FWD,
                                                    * never released); ONE (to END whatever the gate) */
    [P_SPLAY] = PE("PLAY", N_SPLAY, 0),            /* KEY as before; CHOP: a hit's note picks the sample (chop) */
    [P_SCUT] = PE("CUT", N_ONOFF, 0),              /* OFF as before (hits overlap); ON: a hit cuts the one sounding */
};

static const param_desc_t GP[G_COUNT] = {
    [G_BPM] = PD("BPM", F_BPM, 40, 240, 120),
    [G_SWING] = PD("SWG", F_PCT, 0, 100, 0),
    [G_CLOCK] = PE("CLK", N_CLOCK, 0),
    [G_DTIME] = PE("TIME", N_DTIME, 1),
    [G_DFDBK] = PD("FDBK", F_PCT, 0, 120, 60),
    [G_DCOLOR] = PD("COLR", F_PCT, 0, 127, 70),
    [G_DMIX] = PD("MIX", F_PCT, 0, 127, 90),
    [G_RSIZE] = PD("SIZE", F_PCT, 0, 127, 90),
    [G_RDAMP] = PD("DAMP", F_PCT, 0, 127, 60),
    [G_CRATE] = PD("CRT", F_LFOHZ, 0, 127, 40),
    [G_CDEPTH] = PD("CDP", F_PCT, 0, 127, 60),
    [G_MIDI] = PE("MIDI", N_DASH, 0),
    [G_SYNC] = PE("SYNC", N_SYNC, 0),
    [G_ROUTE] = PE("ROUT", N_ROUTE, 0),
    [G_INFO] = PD("CPU", F_INT, 0, 0, 0),
    [G_SLOT] = {"SLOT", F_ENUM, 1, 4, 1, N_SLOT, 0},
    [G_NAME] = PE("NAME", N_DASH, 0),
    [G_LOAD] = PE("LOAD", N_GO, 0),
    [G_SAVE] = PE("SAVE", N_GO, 0),
    [G_CLRSEQ] = PE("CLRSQ", N_GO, 0),
    [G_INITSND] = PE("INIT", N_GO, 0),
    [G_DRCH] = PD("CH", F_INT, 1, 16, 10),
    [G_GMODE] = PE("MODE", N_GMODE, 0),
    [G_GX] = PD("X", F_INT, 0, 127, 64),
    [G_GY] = PD("Y", F_INT, 0, 127, 64),
    [G_GCHAOS] = PD("CHAOS", F_PCT, 0, 127, 0),
    [G_GFILL1] = PD("FIL K", F_PCT, 0, 127, 64),
    [G_GFILL2] = PD("FIL S", F_PCT, 0, 127, 64),
    [G_GFILL3] = PD("FIL H", F_PCT, 0, 127, 64),
    [G_GLEN1] = PD("LEN K", F_STEPS, 1, 32, 16),
    [G_GLEN2] = PD("LEN S", F_STEPS, 1, 32, 12),
    [G_GLEN3] = PD("LEN H", F_STEPS, 1, 32, 8),
    [G_CSRC] = PE("SRC", N_CSRC, 0),
    [G_CTHR] = PD("THRSH", F_CTHR, 0, 127, 26),
    [G_CRAT] = PD("RATIO", F_CRAT, 0, 127, 45),
    [G_CREL] = PD("REL", F_CREL, 0, 127, 26),
    [G_CATK] = PD("ATK", F_CATK, 0, 127, 2),
    [G_CKNEE] = PE("KNEE", N_KNEE, 1),
    [G_CMKUP] = PD("MKUP", F_CMKUP, 0, 127, 0),
    [G_CGHOST] = PE("GHOST", N_GHOST, CG_KEEP),
    [G_RTYPE] = PE("TYPE", N_RTYPE, 0),
    [G_DMODE] = PE("MODE", N_DMODE, 0),
};

/* after G_COUNT: the TOOLS actions (G_CLRALL, G_INITALL) and GLOBAL 3/7's MUTE, METRO, THRU and QUANT (G_MUTEBAR,
 * G_METRO, G_THRU, G_QUANT: the device settings settings.mutebar, fx_metro, midi_thru, rec_quant); their knob values
 * are song.act, not song.g / projects */
static const char *const N_MUTEBAR[] = {"NOW", "BAR"};
static const char *const N_METRO[] = {"OFF", "LOW", "MID", "HIGH"};
static const char *const N_QUANT[] = {"1/16", "1/8", "1/32", "OFF"};   /* seq.c QUANT_16 .. QUANT_OFF */
static const param_desc_t GP_ACT[8] = {PE("CLR*", N_GO, 0), PE("INIT*", N_GO, 0), PE("MUTE", N_MUTEBAR, 0),
                                       PE("METRO", N_METRO, 0), PE("COPY", N_GO, 0), PE("PASTE", N_GO, 0),
                                       PE("THRU", N_ONOFF, 0), PE("QUANT", N_QUANT, 0)};

/* RESON STRCT on CHORD: the chord type, 0..127 split evenly over N_RCHORD (reson.c rs_chord) */
static const param_desc_t RS_CHORD_DESC = {"CHORD", F_INT, 0, 127, 0, N_RCHORD, 0};

/* the INSERT's A B C as its TYPE names them (Skiff 1.3, from Felucca 1.5; insert.c): the drives, CRUSH, the swept
 * ones; OFF: TP's A B C */
#define PU(l, f, df, n, u) {l, f, 0, 127, df, n, u}
static const param_desc_t INS_DESC[3][3] = {
    {PU("DRIVE", F_PCT, 64, 0, 0), PU("TONE", F_CUTOFF, 96, 0, 0), PU("LEVEL", F_DB, 96, 0, 0)},
    {PU("BITS", F_INT, 64, N_IBITS, 0), PU("RATE", F_INT, 96, N_IRATE, "Hz"), PU("LPF", F_CUTOFF, 96, 0, 0)},
    {PU("RATE", F_LFOHZ, 64, 0, 0), PU("DEPTH", F_PCT, 96, 0, 0), PU("FDBK", F_PCT, 96, 0, 0)},
};
#undef PU
static const param_desc_t *ins_desc(int32_t type, uint32_t k)
{
    if (type <= IT_OFF || type >= IT_N)
        return &TP[P_IA + k];
    return &INS_DESC[type < IT_CRUSH ? 0 : type == IT_CRUSH ? 1 : 2][k];
}

static const param_desc_t *track_desc(const track_t *t, uint32_t id)
{
    if (id >= P_IA && id <= P_IC)
        return ins_desc(t->p[P_ITYPE], id - P_IA);
    if (id >= P_E0 && id <= P_E7)
        return &DMODELS[(uint32_t)t->p[P_MODEL] % NMODELS].edit[id - P_E0];
    if (id == P_RSTRCT && t->p[P_RMODEL] == RS_CHORD)
        return &RS_CHORD_DESC;
    return &TP[id];
}

