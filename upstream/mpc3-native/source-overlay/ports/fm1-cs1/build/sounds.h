static const patch_t INIT_PATCH = {                   /* the globals; sound_load(0) fills the channels */
    {{FT_8, W_SAW, 0, 0, 40, 0, 0, 127, 110, 0, 0, 127, 0, 64, 64, 0, 40, 40, 20, 60, 127, 50, 0, 0, 0, 0},
     {FT_8, W_SAW, 0, 0, 40, 0, 0, 127, 110, 0, 0, 127, 0, 64, 64, 0, 40, 40, 20, 60, 127, 50, 0, 0, 0, 0}},
    60, LW_SINE, 0, 0, 0, 64, 64, 100,
    64, 0, 0, 0,  0, 0, 0,  0, 64, 0, 10, 60,  CHO_OFF, 30, 60,  40, 60, 0, 40,  70, 50, 0, 1, 40,  0, 80, 50, 0,
    .rv_shift = 12, .mix = 64, .vintage = 30, .kb_bl = 64, .kb_bh = 64, .kb_ll = 64, .kb_lh = 64,   /* (designated from here: a field added above shifted these once) */
    .ph_rate = 40, .ph_depth = 64, .ph_fb = 40, .eq_lo = 64, .eq_hi = 64, .drive_lv = 64,
};

/* sounds: a tone on each channel and the globals, the way the CS-80's two rows get combined */
/* a sound: a tone on each channel, the globals, and a few tweaks to the channels' own parameters. A field left 0
 * takes the default noted; a feet entry is FEET + 1 (0 = 8'). The classic ones are recipes after what is
 * documented about the records (the presets used, the technique), not transcriptions. */
typedef struct { uint8_t ch, off, val; } tweak_t;      /* ch 0 / 1 (2: both), offsetof(chan_patch_t, ..), the value. A list shorter than
                                                        * 12 MUST end in TWEND: a zero-filled entry reads as channel I, feet, 16' */
#define TW(c, f, v) {c, (uint8_t)offsetof(chan_patch_t, f), v}
#define TWEND {3, 0, 0}
/* the categories: every sound carries one; a user preset files itself by its panel (classify) */
enum { C_ALL = 0, C_PAD, C_STRINGS, C_BRASS, C_KEYS, C_BELLS, C_BASS, C_LEAD, C_FX, C_USER, NCAT };
static const char *const N_CAT[NCAT] = {"ALL", "PAD", "STRINGS", "BRASS", "KEYS", "BELLS", "BASS", "LEAD", "FX", "USER"};
typedef struct {
    const char *name;
    uint8_t cat;                                      /* its category (C_PAD ...) */
    uint8_t tone[2], feet1[2], level[2];              /* levels: 0 = 110 */
    uint8_t brill, reso, detune, mix;                 /* 0 = 64 (centre) */
    uint8_t porta, gliss, at_vco, at_brill, at_vcf, pbend;
    uint8_t lfo_wave, lfo_spd, lfo_vco, lfo_vcf;      /* lfo_spd 0 = 60 */
    uint8_t cho_mode, cho_spd, cho_depth;             /* cho_spd 0 = 30, cho_depth 0 = 60 */
    uint8_t rm_mod, rm_spd, rm_depth, rm_d;           /* rm_spd 0 = 64, rm_d 0 = 60 */
    uint8_t v_a, v_r;                                 /* 0: the tones' own VCA attack / release, else these on both */
    uint8_t aat_delay, aat_rise, aat_level, aat_fall; /* aat_rise 0 = 60, aat_fall 0 = 40 */
    uint8_t rv_mix, rv_size, rv_damp, rv_shim;        /* rv_size 0 = 80, rv_damp 0 = 50 */
    uint8_t rv_shift;                                 /* the shimmer's interval in semitones, 0 = 12 */
    uint8_t dly_time, dly_fb, dly_mix, dly_ping;      /* dly_time 0 = 70, dly_fb 0 = 50 */
    uint8_t dly_sync, vintage;                        /* the delay's clock division (0 free); vintage 0 = 30 */
    uint8_t dly_tape;                                 /* the delay's tape character */
    uint8_t ph_rate, ph_depth, ph_fb, ph_mix;         /* the phaser (ph_rate 0 = 40, ph_depth 0 = 64, ph_fb 0 = 40; mix 0 = off) */
    tweak_t tw[12];
} sound_t;
enum { T_STR1 = 0, T_STR2, T_STR3, T_STR4, T_BRS1, T_BRS2, T_FLUTE, T_BRS3, T_EPIANO, T_BASS, T_CLAV1, T_CLAV2,
       /* (the rest in order) */
       T_HARP1, T_HARP2, T_ORG1, T_ORG2, T_GTR1, T_GTR2, T_FNK1, T_FNK2, T_FNK3, T_FNK4 };
#define F16 (FT_16 + 1)
#define F8 (FT_8 + 1)
#define F4 (FT_4 + 1)
#define F2 (FT_2 + 1)
#define F53 (FT_53 + 1)
#define F27 (FT_27 + 1)
static const sound_t SOUNDS[] = {
    /* ---- Vangelis */
    {.name = "BLADE BRASS", .cat = C_BRASS, .rv_mix = 56, .tone = {T_BRS1, T_BRS2}, .level = {115, 115}, .brill = 60, .reso = 70, .detune = 70,
     .porta = 18, .at_vco = 30, .at_brill = 90, .cho_mode = CHO_CHORUS, .cho_spd = 22, .cho_depth = 70, .v_a = 62,
     .aat_delay = 50, .aat_rise = 70, .aat_level = 100, .rv_size = 100, .tw = {TWEND}},
    {.name = "BR BLUES", .cat = C_BRASS, .rv_mix = 68, .tone = {T_STR3, T_BRS3}, .feet1 = {F4, F8}, .level = {110, 95}, .brill = 70, .reso = 68, .detune = 72,
     .porta = 30, .at_vco = 34, .at_brill = 70, .cho_mode = CHO_CHORUS, .cho_spd = 18, .cho_depth = 75, .v_a = 40, .v_r = 80,
     .aat_delay = 45, .aat_rise = 65, .aat_level = 110, .aat_fall = 60, .rv_size = 115, .rv_shim = 25,
     .dly_time = 84, .dly_fb = 45, .dly_mix = 30, .dly_tape = 50, .dly_ping = 1, .dly_sync = 6, .tw = {TW(0, lp_cut, 100), TW(1, lp_cut, 90), TWEND}},
    {.name = "BR FLUTE", .cat = C_BRASS, .rv_mix = 56, .tone = {T_FLUTE, T_ORG2}, .feet1 = {F8, F16}, .level = {115, 60}, .brill = 60, .detune = 68,
     .porta = 60, .at_vco = 22, .at_brill = 40, .cho_mode = CHO_CHORUS, .cho_spd = 16, .cho_depth = 70, .v_a = 48,
     .aat_delay = 60, .aat_rise = 75, .aat_level = 70, .aat_fall = 50, .rv_size = 100,
     .dly_time = 90, .dly_fb = 50, .dly_mix = 45, .dly_tape = 50, .dly_ping = 1, .dly_sync = 4, .tw = {TWEND}},
    {.name = "BR STRINGS", .cat = C_STRINGS, .rv_mix = 68, .ph_rate = 20, .ph_depth = 60, .ph_fb = 30, .ph_mix = 40, .tone = {T_STR1, T_STR2}, .feet1 = {F8, F16}, .level = {115, 90}, .brill = 56, .reso = 66, .detune = 72,
     .porta = 22, .at_vco = 19, .at_brill = 50, .cho_mode = CHO_CHORUS, .cho_spd = 18, .cho_depth = 85, .v_a = 85, .v_r = 95,
     .aat_delay = 55, .aat_rise = 80, .aat_level = 90, .aat_fall = 60, .rv_size = 118, .rv_shim = 30, .tw = {TWEND}},
    {.name = "CHARIOTS", .cat = C_BRASS, .rv_mix = 44, .tone = {T_BRS1, T_BRS3}, .level = {115, 105}, .brill = 72, .reso = 66, .detune = 70,
     .porta = 10, .at_vco = 32, .at_brill = 80, .cho_mode = CHO_CHORUS, .cho_spd = 24, .cho_depth = 55, .v_a = 30,
     .aat_delay = 40, .aat_rise = 55, .aat_level = 95, .rv_size = 95,
     .dly_time = 96, .dly_fb = 55, .dly_mix = 50, .dly_tape = 50, .dly_ping = 1, .dly_sync = 4, .tw = {TWEND}},
    {.name = "UNKNOWN MAN", .cat = C_PAD, .rv_mix = 52, .tone = {T_BRS1, T_STR2}, .level = {115, 80}, .brill = 66, .reso = 64, .detune = 69,
     .porta = 26, .at_vco = 36, .at_brill = 60, .cho_mode = CHO_CHORUS, .cho_spd = 20, .cho_depth = 60, .v_a = 36,
     .aat_delay = 40, .aat_rise = 60, .aat_level = 100, .rv_size = 100, .dly_time = 100, .dly_fb = 40, .dly_mix = 35, .dly_tape = 50, .dly_sync = 3, .tw = {TWEND}},
    /* Syntorial's Blade Runner brass: two 50 % pulses an octave apart, 6 cents off, PWM at ~4 Hz, a 650 ms filter
     * swell that takes 10 s to fall, the amplifier at 250 ms / 1.9 s / 40 % / 750 ms, a big reverb at 10 % */
    {.name = "BR BRASS 2", .cat = C_BRASS, .rv_mix = 28, .tone = {T_BRS1, T_BRS2}, .feet1 = {F8, F4}, .level = {115, 85}, .brill = 64, .reso = 64, .detune = 56,
     .at_vco = 26, .at_brill = 70, .rv_size = 100, .aat_delay = 50, .aat_rise = 70, .aat_level = 80,
     .tw = {TW(2, wave, W_PULSE), TW(2, pw, 0), TW(2, pwm, 100), TW(2, pwm_spd, 78), TW(2, lp_cut, 95), TW(2, lp_res, 19),
            TW(2, f_il, 64), TW(2, f_al, 82), TW(2, f_a, 88), TW(2, f_d, 124), TW(2, f_r, 124), TW(2, v_a, 75)}},
    /* the brass swell as MusicTech builds it: two saws 60 / 40 a couple of cents apart, the filter envelope a third
     * of the range over a second, long decay and release; a tape delay with high feedback and a 10 s reverb */
    {.name = "BRASS SWELL", .cat = C_BRASS, .rv_mix = 48, .tone = {T_BRS1, T_BRS2}, .level = {118, 95}, .brill = 60, .reso = 62, .detune = 67,
     .at_vco = 19, .at_brill = 60, .cho_mode = CHO_CHORUS, .cho_spd = 18, .cho_depth = 40, .v_a = 55, .v_r = 100,
     .aat_delay = 40, .aat_rise = 80, .aat_level = 90, .rv_size = 100,
     .dly_time = 95, .dly_fb = 70, .dly_mix = 35, .dly_tape = 50, .dly_ping = 1,
     .dly_sync = 4, .tw = {TW(2, f_il, 64), TW(2, f_al, 104), TW(2, f_a, 94), TW(2, f_d, 120), TW(2, f_r, 120), TW(2, v_s, 120), TWEND}},
    /* Blade Runner Blues as a forum player had it on the Arturia: Brass 1 + 2 with the performance side up */
    {.name = "BR BLUES 2", .cat = C_BRASS, .rv_mix = 68, .tone = {T_BRS1, T_BRS2}, .level = {115, 110}, .brill = 66, .reso = 66, .detune = 70,
     .porta = 24, .at_vco = 34, .at_brill = 110, .cho_mode = CHO_CHORUS, .cho_spd = 18, .cho_depth = 70, .v_a = 36, .v_r = 85,
     .aat_delay = 40, .aat_rise = 60, .aat_level = 115, .aat_fall = 60, .rv_size = 115,
     .dly_time = 84, .dly_fb = 45, .dly_mix = 30, .dly_tape = 50, .dly_ping = 1, .dly_sync = 6, .tw = {TWEND}},
    /* the End Titles' sequenced bass: two saws an octave apart, the filter under an envelope; no chorus */
    {.name = "END TITLES", .cat = C_BASS, .rv_mix = 20, .tone = {T_BRS1, T_BRS2}, .feet1 = {F16, F8}, .level = {118, 100}, .brill = 60, .reso = 72, .detune = 66,
     .rv_size = 70, .dly_time = 80, .dly_fb = 40, .dly_mix = 25, .dly_tape = 50, .dly_ping = 1,
     .dly_sync = 4, .tw = {TW(2, f_il, 64), TW(2, f_al, 100), TW(2, f_a, 0), TW(2, f_d, 70), TW(2, v_a, 0), TW(2, v_r, 40), TWEND}},
    {.name = "CHIMES", .cat = C_BELLS, .rv_mix = 60, .tone = {T_FLUTE, T_BASS}, .feet1 = {F4, F4}, .level = {110, 90}, .brill = 70,
     .cho_mode = CHO_CHORUS, .cho_spd = 20, .cho_depth = 40, .rm_mod = 115, .rm_spd = 78, .rm_depth = 60, .rm_d = 85,
     .rv_size = 105, .rv_shim = 30, .tw = {TWEND}},
    /* ---- Toto, Thriller */
    {.name = "AFRICA", .cat = C_STRINGS, .rv_mix = 32, .tone = {T_STR1, T_STR4}, .level = {120, 80}, .brill = 56, .reso = 76, .mix = 48,
     .cho_mode = CHO_CHORUS, .cho_spd = 20, .cho_depth = 40, .rv_size = 80, .tw = {TWEND}},
    {.name = "AFRICA 2", .cat = C_STRINGS, .rv_mix = 20, .tone = {T_STR3, T_STR4}, .level = {115, 110}, .brill = 60, .reso = 64, .detune = 66,
     .cho_mode = CHO_CHORUS, .cho_spd = 20, .cho_depth = 50, .rv_size = 70,
     .tw = {TW(2, lp_cut, 70), TW(2, f_al, 76), TW(2, f_a, 59), TW(2, f_d, 116), TW(2, v_d, 124), TW(2, v_s, 0), TWEND}},
    {.name = "ROSANNA", .cat = C_BRASS, .rv_mix = 28, .tone = {T_BRS1, T_BRS2}, .level = {115, 110}, .brill = 62, .reso = 68, .detune = 67,
     .cho_mode = CHO_CHORUS, .cho_spd = 24, .cho_depth = 50, .v_a = 20, .rv_size = 70, .tw = {TWEND}},
    {.name = "HUMAN NATURE", .cat = C_PAD, .rv_mix = 32, .tone = {T_STR1, T_BRS2}, .level = {110, 110}, .brill = 58, .reso = 80, .detune = 66,
     .porta = 22, .gliss = 1, .cho_mode = CHO_CHORUS, .cho_spd = 24, .cho_depth = 75, .rv_size = 85,
     .tw = {TW(0, v_r, 95), TWEND}},
    {.name = "BILLIE JEAN", .cat = C_STRINGS, .rv_mix = 32, .tone = {T_STR1, T_BRS2}, .level = {110, 110}, .brill = 58, .reso = 64, .detune = 66,
     .cho_mode = CHO_CHORUS, .cho_spd = 24, .cho_depth = 75, .rv_size = 85, .tw = {TW(0, v_r, 90), TWEND}},
    /* Syntorial's Billie Jean keys recipe: a narrow pulse, PWM at a quarter note, a short filter blip, fast chorus */
    {.name = "BILLIE KEYS", .cat = C_KEYS, .rv_mix = 32, .tone = {T_HARP1, T_HARP2}, .level = {118, 0}, .mix = 0, .brill = 62, .reso = 60,
     .cho_mode = CHO_CHORUS, .cho_spd = 60, .cho_depth = 45, .rv_size = 75,
     .tw = {TW(0, pw, 113), TW(0, pwm, 76), TW(0, pwm_spd, 63), TW(0, lp_cut, 83), TW(0, f_al, 70), TW(0, f_d, 83), TWEND}},
    /* ---- the eighties */
    {.name = "BORN USA", .cat = C_BRASS, .rv_mix = 36, .tone = {T_BRS1, T_STR2}, .level = {118, 100}, .brill = 74, .reso = 62, .detune = 68,
     .cho_mode = CHO_CHORUS, .cho_spd = 26, .cho_depth = 45, .v_a = 12, .rv_size = 90, .tw = {TW(1, lp_cut, 100), TWEND}},
    {.name = "XMAS BASS", .cat = C_BASS, .rv_mix = 16, .tone = {T_STR1, T_BASS}, .level = {0, 120}, .mix = 127, .brill = 84, .reso = 118,
     .lfo_wave = LW_SAWDN, .lfo_spd = 70, .lfo_vcf = 127, .rv_size = 50, .tw = {TW(1, v_r, 40), TWEND}},
    {.name = "WHO BASS", .cat = C_BASS, .rv_mix = 20, .tone = {T_BASS, T_BASS}, .feet1 = {F16, F8}, .level = {120, 90}, .brill = 70, .reso = 90, .detune = 66,
     .rv_size = 60, .tw = {TW(2, lp_res, 90), TW(2, f_al, 100), TW(2, f_d, 60), TWEND}},
    {.name = "BABOOSHKA", .cat = C_KEYS, .rv_mix = 32, .tone = {T_FLUTE, T_ORG2}, .level = {118, 55}, .brill = 66, .detune = 67,
     .porta = 40, .at_vco = 26, .at_brill = 30, .lfo_spd = 66, .cho_mode = CHO_CHORUS, .cho_spd = 20, .cho_depth = 50,
     .v_a = 20, .aat_delay = 45, .aat_rise = 60, .aat_level = 80, .rv_size = 80, .dly_time = 80, .dly_fb = 35, .dly_mix = 25, .dly_tape = 50, .dly_sync = 3, .tw = {TWEND}},
    {.name = "DUCHESS", .cat = C_PAD, .rv_mix = 48, .tone = {T_STR3, T_STR4}, .level = {115, 110}, .brill = 60, .reso = 64, .detune = 70,
     .porta = 45, .cho_mode = CHO_CHORUS, .cho_spd = 18, .cho_depth = 80, .v_a = 70, .v_r = 85, .rv_size = 100, .tw = {TWEND}},
    {.name = "TURN IT ON", .cat = C_BRASS, .rv_mix = 24, .tone = {T_BRS1, T_BRS3}, .level = {118, 110}, .brill = 70, .reso = 70, .detune = 68,
     .cho_mode = CHO_CHORUS, .cho_spd = 26, .cho_depth = 45, .v_a = 8, .rv_size = 70, .tw = {TW(2, f_a, 20), TWEND}},
    {.name = "TIME STRINGS", .cat = C_STRINGS, .rv_mix = 52, .ph_rate = 28, .ph_depth = 70, .ph_fb = 35, .ph_mix = 55, .tone = {T_STR1, T_STR2}, .level = {115, 115}, .brill = 58, .reso = 64, .detune = 71,
     .cho_mode = CHO_CHORUS, .cho_spd = 16, .cho_depth = 90, .v_a = 80, .v_r = 100, .rv_size = 110, .tw = {TWEND}},
    {.name = "VIENNA", .cat = C_STRINGS, .rv_mix = 56, .ph_rate = 30, .ph_depth = 50, .ph_fb = 20, .ph_mix = 35, .tone = {T_STR1, T_BRS3}, .level = {115, 95}, .brill = 54, .reso = 72, .detune = 69,
     .cho_mode = CHO_CHORUS, .cho_spd = 20, .cho_depth = 65, .v_a = 55, .v_r = 90, .rv_size = 110,
     .dly_time = 100, .dly_fb = 50, .dly_mix = 35, .dly_tape = 50, .dly_ping = 1, .dly_sync = 4, .tw = {TWEND}},
    {.name = "OXYGENE", .cat = C_PAD, .rv_mix = 64, .tone = {T_STR3, T_STR4}, .feet1 = {F8, F16}, .level = {115, 100}, .brill = 56, .detune = 72,
     .lfo_spd = 50, .lfo_vcf = 100, .cho_mode = CHO_CHORUS, .cho_spd = 14, .cho_depth = 95, .v_a = 90, .v_r = 110,
     .rv_size = 120, .rv_shim = 20, .tw = {TW(0, pwm, 60), TW(0, pwm_spd, 36), TW(0, wave, W_BOTH), TWEND}},
    {.name = "ENO WASH", .cat = C_PAD, .rv_mix = 76, .ph_rate = 12, .ph_depth = 90, .ph_fb = 50, .ph_mix = 50, .tone = {T_STR3, T_ORG2}, .feet1 = {F8, F16}, .level = {110, 80}, .brill = 48, .detune = 70,
     .cho_mode = CHO_CHORUS, .cho_spd = 12, .cho_depth = 85, .v_a = 110, .v_r = 115, .rv_size = 125, .rv_damp = 70,
     .rv_shim = 35, .tw = {TWEND}},
    {.name = "WALKING", .cat = C_KEYS, .rv_mix = 28, .tone = {T_STR3, T_FNK2}, .level = {110, 110}, .brill = 72, .reso = 84, .detune = 68,
     .cho_mode = CHO_CHORUS, .cho_spd = 44, .cho_depth = 95, .v_a = 15, .rv_size = 75,
     .tw = {TW(1, wave, W_PULSE), TW(1, pw, 40), TW(1, pwm, 70), TW(1, hp_cut, 55), TW(0, hp_cut, 50), TWEND}},
    {.name = "4001 PLUCK", .cat = C_KEYS, .rv_mix = 36, .tone = {T_STR3, T_STR4}, .level = {118, 0}, .mix = 0, .brill = 58, .reso = 100,
     .cho_mode = CHO_CHORUS, .cho_spd = 24, .cho_depth = 50, .v_r = 100, .rv_size = 90,
     .tw = {TW(0, hp_cut, 55), TW(0, lp_cut, 35), TW(0, lp_res, 120), TW(0, f_al, 110), TW(0, f_d, 45), TW(0, f_a, 0), TWEND}},
    {.name = "4001 STRINGS", .cat = C_STRINGS, .rv_mix = 40, .ph_rate = 22, .ph_depth = 60, .ph_fb = 30, .ph_mix = 45, .tone = {T_STR1, T_STR2}, .feet1 = {F8, F16}, .level = {115, 110}, .brill = 110, .reso = 64, .detune = 68,
     .lfo_spd = 78, .lfo_vco = 20, .cho_mode = CHO_CHORUS, .cho_spd = 22, .cho_depth = 60, .v_a = 64, .v_r = 90,
     .rv_size = 95, .tw = {TW(2, lp_cut, 127), TW(2, hp_cut, 0), TWEND}},
    {.name = "LONG BRASS", .cat = C_BRASS, .rv_mix = 60, .tone = {T_BRS2, T_BRS3}, .level = {112, 112}, .brill = 60, .reso = 66, .detune = 70,
     .at_vco = 22, .at_brill = 100, .cho_mode = CHO_CHORUS, .cho_spd = 20, .cho_depth = 60, .v_a = 95, .v_r = 90,
     .aat_delay = 70, .aat_rise = 100, .aat_level = 110, .aat_fall = 70, .rv_size = 110,
     .dly_time = 104, .dly_fb = 55, .dly_mix = 40, .dly_tape = 50, .dly_ping = 1, .dly_sync = 5, .tw = {TWEND}},
    {.name = "SHRIEK LEAD", .cat = C_LEAD, .rv_mix = 48, .tone = {T_FNK3, T_FNK1}, .feet1 = {F2, F4}, .level = {115, 90}, .brill = 80, .reso = 96, .detune = 70,
     .porta = 20, .at_vco = 34, .at_brill = 60, .cho_mode = CHO_CHORUS, .cho_spd = 30, .cho_depth = 50,
     .rv_size = 100, .dly_time = 90, .dly_fb = 50, .dly_mix = 35, .dly_tape = 50, .tw = {TWEND}},
    {.name = "SQUELCH BASS", .cat = C_BASS, .rv_mix = 12, .tone = {T_FNK1, T_BASS}, .feet1 = {F16, F16}, .level = {110, 110}, .brill = 60, .reso = 100, .detune = 66,
     .rv_size = 50, .tw = {TW(2, f_a, 0), TW(2, f_d, 50), TW(2, v_r, 35), TWEND}},
    /* ---- the manual's own examples and the plain pairs */
    {.name = "TRUMPET", .cat = C_BRASS, .rv_mix = 28, .tone = {T_BRS1, T_BRS2}, .level = {115, 60}, .brill = 70, .reso = 64, .detune = 66,
     .lfo_spd = 64, .lfo_vco = 30, .v_a = 25, .v_r = 45, .rv_size = 75,
     .tw = {TW(0, f_il, 16), TW(0, f_al, 112), TW(0, lp_cut, 80), TWEND}},
    {.name = "CLARINET", .cat = C_BRASS, .rv_mix = 28, .tone = {T_CLAV1, T_EPIANO}, .level = {118, 0}, .mix = 0, .brill = 58, .lfo_spd = 62, .lfo_vco = 22,
     .v_a = 40, .v_r = 50, .rv_size = 80,
     .tw = {TW(0, pw, 0), TW(0, pwm, 0), TW(0, lp_cut, 72), TW(0, f_al, 76), TW(0, f_d, 70), TW(0, lp_res, 20), TWEND}},
    {.name = "OBOE", .cat = C_BRASS, .rv_mix = 28, .tone = {T_HARP1, T_HARP2}, .level = {118, 0}, .mix = 0, .brill = 64, .lfo_spd = 64, .lfo_vco = 20,
     .v_a = 35, .v_r = 50, .rv_size = 80,
     .tw = {TW(0, pw, 113), TW(0, hp_cut, 60), TW(0, lp_cut, 90), TW(0, f_al, 84), TW(0, f_d, 65), TW(0, v_s, 110), TWEND}},
    {.name = "TIMPANI", .cat = C_FX, .rv_mix = 36, .tone = {T_GTR1, T_BASS}, .feet1 = {F8, F16}, .level = {100, 118}, .brill = 56, .reso = 76, .detune = 64,
     .lfo_spd = 40, .rv_size = 85,
     .tw = {TW(0, wave, W_SAW), TW(0, noise, 120), TW(0, vcf_lv, 100), TW(0, f_r, 60), TW(1, sine, 100), TW(1, pwm, 40), TWEND}},
    {.name = "STRINGS 1+2", .cat = C_STRINGS, .rv_mix = 28, .tone = {T_STR1, T_STR2}, .cho_mode = CHO_OFF, .rv_size = 80, .tw = {TWEND}},
    {.name = "BRASS 1+2", .cat = C_BRASS, .rv_mix = 20, .tone = {T_BRS1, T_BRS2}, .cho_mode = CHO_OFF, .rv_size = 60, .tw = {TWEND}},
    {.name = "FLUTE+BASS", .cat = C_KEYS, .rv_mix = 24, .tone = {T_FLUTE, T_BASS}, .level = {110, 100}, .cho_mode = CHO_OFF, .rv_size = 70, .tw = {TWEND}},
    {.name = "ORGANS", .cat = C_KEYS, .rv_mix = 32, .ph_rate = 60, .ph_depth = 50, .ph_fb = 20, .ph_mix = 60, .tone = {T_ORG1, T_ORG2}, .feet1 = {F8, F4}, .level = {110, 100}, .brill = 72, .reso = 76,
     .cho_mode = CHO_CHORUS, .cho_spd = 40, .cho_depth = 60, .rv_size = 90, .tw = {TWEND}},
    {.name = "CLAVS", .cat = C_KEYS, .rv_mix = 12, .tone = {T_CLAV1, T_CLAV2}, .level = {110, 100}, .cho_mode = CHO_OFF, .rv_size = 50, .tw = {TWEND}},
    {.name = "HARPSI", .cat = C_KEYS, .rv_mix = 20, .tone = {T_HARP1, T_HARP2}, .cho_mode = CHO_OFF, .rv_size = 60, .tw = {TWEND}},
    {.name = "E.PIANO", .cat = C_KEYS, .rv_mix = 24, .tone = {T_EPIANO, T_STR4}, .level = {120, 50}, .cho_mode = CHO_CHORUS, .cho_spd = 30, .cho_depth = 50,
     .rv_size = 70, .dly_mix = 20, .dly_tape = 50, .tw = {TWEND}},
    {.name = "FUNKY 1+2", .cat = C_KEYS, .rv_mix = 12, .tone = {T_FNK1, T_FNK2}, .cho_mode = CHO_OFF, .rv_size = 50, .tw = {TWEND}},
    /* ---- with the effects */
    {.name = "TAPE PAD", .cat = C_PAD, .tone = {T_STR1, T_STR4}, .level = {110, 100}, .brill = 58, .reso = 60, .detune = 72, .cho_mode = CHO_CHORUS, .cho_spd = 18,
     .cho_depth = 70, .v_a = 85, .v_r = 95, .aat_delay = 60, .aat_rise = 80, .aat_level = 70, .rv_mix = 55, .rv_size = 105, .rv_damp = 60,
     .dly_time = 95, .dly_fb = 60, .dly_mix = 40, .dly_ping = 1, .dly_sync = 4, .dly_tape = 90, .ph_rate = 15, .ph_depth = 60, .ph_fb = 25, .ph_mix = 35,
     .vintage = 50, .tw = {TWEND}},
    {.name = "SHIMMER CHOIR", .cat = C_PAD, .tone = {T_FLUTE, T_STR4}, .feet1 = {F8, F4}, .level = {115, 95}, .brill = 62, .cho_mode = CHO_CHORUS, .cho_spd = 22,
     .cho_depth = 65, .v_a = 100, .v_r = 110, .aat_delay = 70, .aat_rise = 90, .aat_level = 60, .rv_mix = 85, .rv_size = 122, .rv_damp = 35, .rv_shim = 65,
     .vintage = 40, .tw = {TW(2, sine, 60), TWEND}},
    {.name = "PHASE STRINGS", .cat = C_STRINGS, .tone = {T_STR1, T_STR2}, .detune = 70, .cho_mode = CHO_CHORUS, .cho_spd = 24, .cho_depth = 60, .v_a = 50, .v_r = 80,
     .rv_mix = 40, .rv_size = 90, .ph_rate = 32, .ph_depth = 95, .ph_fb = 70, .ph_mix = 100, .vintage = 35, .tw = {TWEND}},
    {.name = "ECHO KEYS", .cat = C_KEYS, .tone = {T_EPIANO, T_CLAV2}, .mix = 50, .brill = 66, .cho_mode = CHO_CHORUS, .cho_spd = 40, .cho_depth = 45, .v_r = 60,
     .rv_mix = 30, .rv_size = 75, .dly_time = 88, .dly_fb = 55, .dly_mix = 45, .dly_ping = 1, .dly_sync = 5, .dly_tape = 75, .tw = {TWEND}},
    {.name = "TAPE BRASS", .cat = C_BRASS, .tone = {T_BRS1, T_BRS3}, .level = {115, 105}, .brill = 60, .reso = 66, .detune = 71, .porta = 20, .at_vco = 30, .at_brill = 80,
     .cho_mode = CHO_CHORUS, .cho_spd = 20, .cho_depth = 70, .v_a = 60, .aat_delay = 45, .aat_rise = 70, .aat_level = 95, .rv_mix = 50, .rv_size = 100,
     .dly_time = 100, .dly_fb = 50, .dly_mix = 40, .dly_ping = 1, .dly_sync = 3, .dly_tape = 65, .vintage = 40, .tw = {TWEND}},
    {.name = "DEEP DRIFT", .cat = C_PAD, .tone = {T_STR3, T_BASS}, .feet1 = {F8, F16}, .level = {105, 115}, .brill = 50, .reso = 70, .detune = 75, .lfo_wave = LW_SINE,
     .lfo_spd = 30, .lfo_vcf = 100, .cho_mode = CHO_CHORUS, .cho_spd = 14, .cho_depth = 80, .v_a = 90, .v_r = 100, .aat_delay = 50, .aat_rise = 80,
     .aat_level = 80, .rv_mix = 60, .rv_size = 110, .rv_damp = 65, .rv_shim = 25, .ph_rate = 8, .ph_depth = 100, .ph_fb = 55, .ph_mix = 70, .vintage = 70,
     .tw = {TWEND}},
    /* ---- after CS-80 V presets, our names (tools/arturia_convert.py) */
#include "cs1_sounds_conv.h"
};

#define NSOUND (sizeof SOUNDS / sizeof SOUNDS[0])
static void tone_load(uint32_t ch, uint32_t tone)     /* a channel's tone, keeping its feet and level */
{
    uint8_t feet = patch.ch[ch].feet, level = patch.ch[ch].level;
    patch.ch[ch] = TONES[tone % NTONE];
    patch.ch[ch].feet = feet;
    patch.ch[ch].level = level;
    set.tone[ch] = (uint8_t)(tone % NTONE);
}

#define DEF(v,d) ((v)?(v):(d))
static void sound_load(uint32_t i)
{
    const sound_t *sd = &SOUNDS[i % NSOUND];
    uint32_t ch;
    for (ch = 0; ch < NCH; ch++) {
        tone_load(ch, sd->tone[ch]);
        patch.ch[ch].feet = sd->feet1[ch] ? (uint8_t)(sd->feet1[ch] - 1u) : (uint8_t)FT_8;
        patch.ch[ch].level = DEF(sd->level[ch], 110);
    }
    patch.brill = DEF(sd->brill, 64);
    patch.reso = DEF(sd->reso, 64);
    patch.detune = DEF(sd->detune, 64);
    patch.mix = DEF(sd->mix, 64);
    patch.porta = sd->porta;
    patch.gliss = sd->gliss;
    patch.pbend = sd->pbend;
    patch.at_spd = 0;
    patch.at_vco = DEF(sd->at_vco, 30);
    patch.at_vcf = sd->at_vcf;
    patch.lfo_wave = sd->lfo_wave;
    patch.lfo_spd = DEF(sd->lfo_spd, 60);
    patch.lfo_vco = sd->lfo_vco;
    patch.lfo_vcf = sd->lfo_vcf;
    patch.lfo_vca = 0;
    patch.cho_mode = sd->cho_mode;
    patch.cho_spd = DEF(sd->cho_spd, 30);
    patch.cho_depth = DEF(sd->cho_depth, 60);
    patch.rm_mod = sd->rm_mod;
    patch.rm_spd = DEF(sd->rm_spd, 64);
    patch.rm_depth = sd->rm_depth;
    patch.rm_a = 10;
    patch.rm_d = DEF(sd->rm_d, 60);
    patch.aat_delay = sd->aat_delay;
    patch.aat_rise = DEF(sd->aat_rise, 60);
    patch.aat_level = sd->aat_level;
    patch.aat_fall = DEF(sd->aat_fall, 40);
    patch.rv_mix = sd->rv_mix;
    patch.rv_size = DEF(sd->rv_size, 80);
    patch.rv_damp = DEF(sd->rv_damp, 50);
    patch.rv_shim = sd->rv_shim;
    patch.rv_shift = DEF(sd->rv_shift, 12);
    patch.dly_time = DEF(sd->dly_time, 70);
    patch.dly_fb = DEF(sd->dly_fb, 50);
    patch.dly_mix = sd->dly_mix;
    patch.dly_ping = sd->dly_ping;
    patch.dly_damp = 40;
    patch.dly_sync = sd->dly_sync;
    patch.dly_tape = sd->dly_tape;
    patch.ph_rate = DEF(sd->ph_rate, 40); patch.ph_depth = DEF(sd->ph_depth, 64); patch.ph_fb = DEF(sd->ph_fb, 40); patch.ph_mix = sd->ph_mix;
    patch.lfo_sync = 0;
    patch.drive = 0; patch.drive_lv = 64; patch.eq_lo = patch.eq_hi = 64;
    patch.vintage = DEF(sd->vintage, 30);
    patch.kb_bl = patch.kb_bh = patch.kb_ll = patch.kb_lh = 64;
    for (ch = 0; ch < NCH; ch++) {
        if (sd->at_brill) patch.ch[ch].at_brill = sd->at_brill;
        if (sd->v_a) patch.ch[ch].v_a = sd->v_a;
        if (sd->v_r) patch.ch[ch].v_r = sd->v_r;
    }
    for (i = 0; i < 12u && sd->tw[i].ch < 3u; i++) {  /* the tweaks to the channels' own parameters */
        if (sd->tw[i].ch != 1u) ((uint8_t *)&patch.ch[0])[sd->tw[i].off] = sd->tw[i].val;
        if (sd->tw[i].ch != 0u) ((uint8_t *)&patch.ch[1])[sd->tw[i].off] = sd->tw[i].val;
    }
    set.sound = (uint8_t)(sd - SOUNDS);
}
