/* ---------------- chord sequence (2.89, the owner's request): record chords in chord mode, play them as notes in keyboard mode ----------------
   A "chord" is what the stock decoded in chord mode: the root class (CLS, 0-11) and the type (DE49, 0-8: major minor 7th maj7 m7 dim
   aug sus4 add9), up to SQ_MAX chords. (2.88 tried to run the stock's own chord code from keyboard mode; it re-tuned the notes
   already sounding, the owner's test, so 2.89 plays the chord's notes on the keyboard voice instead.)

   Record: in chord mode, HOLD KEYBOARD (SQ_LONG ms) = recording on / off (keyboard mode is not entered). Recording clears the old
     sequence. The keyboard LED blinks slowly. A chord ends when every chord button has been up for SQ_CLOSE ms (the chord is the
     decoded one while a button was down). Hold deletes the chord being played, or else the last recorded one (the keyboard LED
     blinks twice; no stock Hold). Entering keyboard mode (a short KEYBOARD press) ends recording.
   Jazz mode (2.94): the chord's jazz extra combination is recorded with it and put back for playback, and bro_on() (mod2.c) is true in
   keyboard mode while a recorded chord sounds, so the jazz voicing the chord has in chord mode is applied to it here too.
   Arpeggio (2.97, layout 2.101): Omni 1 only. Bb minor + a PATTERN button picks it (keyboard mode; the button is then not the stock's): rock 1 up,
   rock 2 down, slow rock up and down, country octaves (up over 2 octaves), swing random, the yellow button off (the default). With two or more note keys down the stock sees them one at a time (the physical keys are
   hidden, the step's key is pressed for it): the Omni voice is monophonic, so it plays the last one only otherwise.
   The Sustain knob: the note length (staccato .. legato); with Bb minor held, the swing (soft takeover, see GT_MIN).
   The speed is the tempo knob's (the note value x the rhythm's own step time, at least ARP_MIN ms); with the rhythm running the steps are the
   rhythm's (on the note value's grid), so the arpeggio follows the beat. Bb minor + a VOICE button picks the note value (voice 1 quarter, 2 eighth =
   the default, 3 sixteenth, 4 swing eighth, 5 swing sixteenth). Bb minor held: the PATTERN LED of the picked mode, the VOICE LED of the note value, the yellow PATTERN
   button's LEDs off (2.99). All of it is the default again at every power-on: arpeggio off, eighths.
   Step sequencer (2.105, 2.107): long press KEYBOARD in keyboard mode = edit mode (a short press ends it), see SQ_SB / sq_edit_btn / sq_tick.
   Play: in keyboard mode, with a sequence recorded, Hold (hidden from the stock) plays the next chord, then the next, and wraps.
     SQ_REAL 1 (2.90): the chord is started with the stock's own held-chord code (see sq_chord_on): chord mode's sound; it stays until the
     next Hold or until keyboard mode is left. SQ_REAL 0 (2.89): the chord's notes (root position, folded into the 20 note keys, in the pitch classes of the chord class + the keyboard
     Transpose) are pressed for the stock like the player's own note keys (injected into the key-matrix scan), so they sound on the
     keyboard voice, go to MIDI, follow the octave, and start / follow the rhythm exactly like notes played by hand. They stay down
     until the next Hold (a gap of SQ_GAP ms releases the old chord first, so a note common to two chords is played again) or until
     keyboard mode is left. Nothing else is touched: no stock chord code runs, no flag changes, the notes already sounding stay as played.
   KEYBOARD: so that a long press can be told from a short one, a short press (no Manual/Auto or PATTERN button down, the panel
     live, KEYBOARD seen up since) reaches the stock when KEYBOARD is let go, as a press of SQ_INJ scans, instead of when it goes
     down. In keyboard mode a long press is a short one. Every combination with another button is as before.
   RAM: sizeof(sq_t) bytes at SQB (the stretch between the stock bss and 0x2000E000). Not saved at power-off. */
#if FW_VERSION == 29
#define SQB       0x2000DEE0u
#else
#define SQB       0x2000DF00u
#endif
#define SQMAGIC   0x53455141u      /* "SEQA" */
#define SQ_MAX    32u              /* chords of the Hold list (2 bytes each, 2.105; 4 before) */
#define SQ_CSTEPS 64u              /* chord steps: 8 pages of 8 (2.109) */
/* 2.109: the big data lives in the chip's OCRAM (the stock uses only 0x20200000-0x20201AA8 of its 128 KB: its own MPU setup makes 0x20200000-0x2021FFFF
   read / write, and the flashloader's reserved region (get-property 12) runs up to 0x2021DEB8, so the RAM is there). The DTCM stretch (SQB, 256 bytes) keeps the
   state; the arrays are here: the Hold list, the note / drum steps and the chord steps. Power-on contents are random: sqx_t has its own magic. */
#ifndef SQ_XP
#define SQ_XP     ((sqx_t *)0x20210000u)
#endif
#define XQ        (SQ_XP)
#define SQXMAGIC  0x58513131u
#define SQ_LONG   700u             /* ms KEYBOARD is held to toggle recording */
#define SQ_CLOSE  40u              /* ms with no chord button down that close a recorded chord */
/* The arpeggio's note value, picked with Bb minor + a VOICE button: 1 quarter (omni 1), 2 eighth (omni 2, the default), 3 sixteenth (harp), 4 swing
   eighth (celeste), 5 swing sixteenth (a. piano). In half stock-rhythm steps (an OM-27 step = 3 stock steps = a 16th, so an eighth = 6 stock steps =
   12 half steps). Swing: the first note of each pair gets 2/3 of the pair, the second 1/3 (a triplet feel). */
static const u8 SQ_DIV[5] = { 24, 12, 6, 12, 6 };
static const u8 SQ_SWG[5] = { 0, 0, 0, 1, 1 };
static const u8 SQ_VK[5] = { 0x24, 0x25, 0x26, 0x33, 0x34 };                 /* the voice buttons 1-5 (as the mode pick with Gb 7th) */
/* A recorded chord is 2 bytes (2.105): the root class | DE49 type << 4, and the stock's row | jazz extra << 4. The 3rd and 5th classes are what the
   stock's chord press (0x6000C178) works out from the root and the row: (root + 4 + T1[row-1], root + 7 + T2[row-1]) mod 12, row 0 = a major triad (T1 = T2 = 0).
   When a chord is recorded the row is found by matching the classes the stock really has (15 = no row matches: a major triad). */
static const s8 SQ_T1[8] = { -1, 0, 0, -1, -1, 0, 1, -2 };
static const s8 SQ_T2[8] = { 0, 3, 4, 3, 2, 1, 0, 0 };
/* The step sequencer (2.105): 32 steps = 4 pages of 8, each step a note (index 0-19 of the keyboard's 20 note keys) and a drum pad (0-7 = keys 4-11), both
   optional. The 12 buttons under the LED rows: voice 1-4 and pattern 1-4 are the 8 steps of a page, voice 5 / yellow VOICE / pattern 5 / yellow PATTERN
   the pages 1-4. */
static void sq_ff(volatile u8 *p, u32 n) { for (u32 i = 0; i < n; i++) p[i] = 0xFFu; }   /* (a plain loop becomes a memset call: no libc here) */
static const u8 SQ_SB[12] = { 0x24, 0x25, 0x26, 0x33, 0x28, 0x29, 0x2A, 0x2B, 0x34, 0x27, 0x2C, 0x2D };
#define ARP_MIN   50u              /* ms: the fastest arpeggio step (a key is down at least 15 ms, up 35) */
/* The Sustain knob (2.103; nothing on Omni 1 otherwise): the arpeggio's note length, and with Bb minor held its swing. Sensor channel 2's latest
   7-bit reading is kept by the stock at 0x20002A5C + channel (1.0.3.0; its reading store 0x6000366C). One knob, two jobs, so each job keeps its value
   until the knob is moved (more than KN_TH) after the job was handed over (soft takeover: no jump when Bb minor is pressed or let go).
   Note length: GT_MIN % of the step (staccato) at the bottom .. the whole step at the top (GT_FULL and above: legato, the key stays down until the
   next step's). Swing (the swing note values only): the first note's share of the pair, SW_MIN % (straight) at the bottom .. SW_MAX % at the top;
   the default is the triplet feel (2/3), the note length's default is in the middle. */
#define SQ_SUSV   RA(20002A3C)     /* stock: the latest 7-bit reading of each sensor, + channel */
#define GT_MIN    15u
#define GT_FULL   124u
#define SW_MIN    50u
#define SW_MAX    75u
#define KN_TH     4u
#ifndef SQ_GPIO2_CLR
#define SQ_GPIO2_CLR 0x401BC088u    /* GPIO2 DR_CLEAR: bit 4 = the bass gate */
#endif
#define BASS_CUT  50u              /* 2.122: ms the 108 rhythm bass gate stays open per note (as the Classic tone's) */
#define SQ_RUN    RA(2000DE47)     /* stock: rhythm running */
#define SQ_STARTP RA(2000DE3F)     /* stock: a rhythm start is pending (0x600081EC sets it, v30 0x2000DE61; the rhythm's own tick takes it up) */
#define SQ_STEP   RA(2000DE0E)     /* stock: the rhythm's step counter (u16, counts from 0) */
#define SQ_STEPT  RA(20002A90)     /* stock: one step's time in 200 us (u32), from the tempo knob: 357 slowest .. 83 fastest (0x60007554, the sensor loop) */
#define SQ_INJ    45u              /* scans the delayed KEYBOARD press reads as down */
#define SQ_BLINK  256u             /* ms per half period of the recording LED */
/* the stock's chord start / stop (found in the 1.0.3.0 code: 0x6000C178, the chord press, calls 0x60008D24 with the 3 classes and the two MIDI
   queues: it programs the 3 chord oscillators (QTMR1 ch 0-2, bit 0x2000 = output on), unmutes the analog side and, when 0x200009D8 is set,
   starts the chord's MIDI notes; 0x60009184 is its stop: the oscillators' bit off, the chord's MIDI off). 1.0.3.0 addresses only. */
#ifndef F_8D24
#if FW_VERSION == 30
#define F_8D24 ((void(*)(u32, u32, u32, u32, u32))(0x60008D24u | 1u))
#define F_9184 ((void(*)(u32, u32))(0x60009184u | 1u))
#define F_819C ((void(*)(u32))(0x6000819Cu | 1u))      /* the bass / chord-active flag (v29 DE42: 1 on; 0 off, with the bass gate) the press path sets after the chord starts */
#else
static void F_8D24(u32 a, u32 b, u32 c, u32 d, u32 e) { (void)a; (void)b; (void)c; (void)d; (void)e; }
static void F_9184(u32 a, u32 b) { (void)a; (void)b; }
static void F_819C(u32 a) { (void)a; }
#endif
#endif
/* 2.113: the stock's rhythm stop, as the Start button's stop branch (0x6000A972 in 1.0.3.0): 0x60008170(0) clears the running flag (DE47, v30 DE69) and asks
   the rhythm to stop (v30 DE60), 0x600081CC clears its step counter. A chord press in chord mode with Sync Start calls 0x60008170(1) itself (0x6000CED8: the
   running flag at once, not only the pending start): sq_post undoes that while chords are being recorded. 1.0.3.0 addresses only. */
#ifndef F_8170
#if FW_VERSION == 30
#define F_8170 ((void(*)(u32))(0x60008170u | 1u))
#define F_81CC ((void(*)(void))(0x600081CCu | 1u))
#else
static void F_8170(u32 a) { (void)a; }
static void F_81CC(void) { }
#endif
#endif
#define SQ_CLS    RA(20000378)     /* stock: the chord's classes (root first), 0-11 */
#define SQ_ANC    RA(2000037C)     /* stock: the chord anchor = the pressed button's (class, row code): what the stock re-applies when something else changes (Transpose) */
#define SQ_BASSF  RA(2000DE42)     /* stock: the flag 0x6000819C sets (1) / clears (0, with the bass gate): the rhythm's bass / chord rows look at it */
#define SQ_TYPE   RA(2000DE49)     /* stock: chord type 0-8 */
#define SQ_TONE   RA(2000DE65)     /* stock: the rhythm tone (0 Normal, 1 Classic), which the drum pad function reads */
/* The stock's drum pad function: the button handler calls 0x6000BE0C(key, &DE65) at 0x6000C9CA for a pad key (4-11) in keyboard mode: the pad's drum
   sound (Normal or Classic tone) and its MIDI. 2.108: the step sequencer calls it itself, so its drums play in any mode and press no key.
   1.0.3.0 address only. */
#ifndef F_BE0C
#if FW_VERSION == 30
#define F_BE0C ((void(*)(u32, const volatile u8 *))(0x6000BE0Cu | 1u))
#else
static void F_BE0C(u32 k, const volatile u8 *t) { (void)k; (void)t; }
#endif
#endif
/* The stock's plate (strum) note table update: c178, the chord apply, calls 0x6000BECC(c0, c1, c2, table, anchor, MA_KB, MA_KS) before it starts the chord: the notes
   the strum plate plays for the chord. A sequence chord has to do it too (2.110: the strum played the last chord pressed by hand). It also re-tunes the plate notes
   that sound: not while a keyboard note sounds (plate slots 0 / 1 are the keyboard's). 1.0.3.0 address only. */
#ifndef F_BECC
#if FW_VERSION == 30
#define F_BECC ((void(*)(u32, u32, u32, u32, u32, u32, u32))(0x6000BECCu | 1u))
#else
static void F_BECC(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; }
#endif
#endif
#define SQ_PT     RA(20006AD8)     /* stock: the plate note table (v30 0x20006AF8) */
#define BL_MS     70u              /* the Start LED's flash at each quarter note */
#define SQ_KT     RA(20000288)     /* stock: the 20 keyboard note keys' ids, lowest note first (note = 0x30 + index + 12 x octave) */
typedef struct {
    u32 magic;
    unsigned short k0, c0;         /* KEYBOARD went down; last chord button down (TICKS low 16 bits) */
    u8 n, pos, rec, cv;            /* chords recorded, next chord, recording on, a chord is being recorded (cc / ct valid) */
    u8 cc[3], ct, ign, injk, on;   /* the chord being recorded; buttons still down after Hold dropped it are ignored; delayed KEYBOARD press scans left; a recorded chord is sounding */
    u8 kst, krel, kinit, kup;      /* KEYBOARD: 0 idle, 1 counting (hidden), 2 long done; counters */
    u8 hseen, hoff, hdn, hreq;     /* Hold: down seen, release counter, hidden from the stock, wanted */
    u8 want, led, osc, hl;         /* a chord wanted; the LED bit was ours; the chord's oscillators were started by us; the Hold LED shows a sequence chord is on (1: it was off before, 2: it was on) */
    u8 bass, anc, a0, a1;          /* the Auto flag was set by us; the anchor was ours, its old class and row code */
    u8 kact, kv, km, ks;           /* the keyboard effect (ui_tick): on, the mode bytes to put back */
    u8 cx, am, arp, ak;            /* the jazz extra combination of the chord being recorded; arpeggio mode (0 off 1 up 2 down 3 up-down 4 octaves 5 random), active; the step's key now pressed (0xFF none) */
    u8 ap, ad, bbm, pp;            /* arpeggio: its place in the held notes, up-down direction; Bb minor is down; PATTERN keys down last scan */
    u8 pk, aen, abeat, pend;       /* PATTERN keys swallowed; the arpeggio is possible now; it was following the rhythm last tick; a step is due (the rhythm's grid) */
    u8 sph, kcl, lshow, dv;        /* swing: the next gap is the short one; the octave marks are to be cleared; the LEDs are ours (1 arpeggio, 2 step sequencer edit); note value 0-4 */
    u8 vk, pv, gt, swv;            /* voice buttons swallowed / down last scan; the Sustain knob: note length and swing values (0-127) */
    u8 nref, nst, se, ss;          /* the knob's reading when its job last changed, its state; the step sequencer's edit mode is on; the selected step 0-31 */
    u8 sp, sact, snk, cm;          /* its play position (0xFF none); playing; the note key it holds down (0xFF none); the chord memory: bit 0 = it holds chord STEPS (not a Hold list), bit 1 = a step chord is sounding */
    u8 dmute, hcb, dnw, dpd;       /* the stock rhythm's drums are muted (keyboard mode, drum steps recorded); Hold was used in a combination; fresh drum pads / pads down last scan */
    u8 spend, sl, cp, ssph;        /* a sequencer step is due (the note value's grid); the drum layer being edited (0, 1); the chord steps' play position (0xFF none); swing: the next gap is the short one */
    u8 bl, cpend, bon, ust;        /* the Start / Sync Start LED bit (port 6) we are driving (0 none); a chord step is due (a quarter note on the rhythm's grid); it is flashing; the Start button was pressed (ms left) */
    u8 go, mute, skd, sws;         /* chord mode: the rhythm may run (it was started with KEYBOARD + Start, or was running in keyboard mode); the stock rhythm is muted (it runs without that); the Start button down last scan; its press is swallowed */
    u8 fpg, frc, fma, fho;         /* the page the playing step was on last tick (0xFF none): the page shown follows it; 2.113: recording in chord mode holds the
                                      panel as Manual with Hold off (frc), the Manual/Auto state and the Hold flag to put back */
    u8 rpre, pad5[3];              /* 2.113: the rhythm was running before the stock button handler ran (sq_post) */
    u8 tpoff, tro, tpi, trx;       /* 2.114: the note sequence's transpose: off (Bb minor + yellow VOICE; 0 = on, the default), its semitones (signed), a key index whose
                                      mark is to be cleared (0xFF none), it is transposing now (keyboard mode, notes playing, on, not editing: the note keys are not the stock's) */
    u8 tl, pad6; unsigned short tpt;   /* 2.114: the key index the sequencer pressed last (0xFF none); when tpi was set */
    signed char toff[20];          /* 2.114: per key index (SQ_KT), the semitones added to its note, set when the sequencer presses that key (kb_note via sq_aoff) */
    u8 bmn, pal, dna, pfr;         /* 2.119: B minor down; drum pads pressed with it (their extra sound, hidden from the stock); fresh pads written as extra (edit); extra pads to play */
    u8 dvk, dvi, dvp, bsn;         /* 2.119: a pad is held: the Sustain knob sets its drum's volume; that instrument; a preview is due; 2.122: a 108 bass note to cut */
    unsigned short dvt, bst0;      /* 2.119: the last preview; 2.122: when that bass note began */
    u8 yh, yus, yinj, pvs;         /* 2.123: yellow VOICE held (keyboard mode, hidden from the stock); the knob was used meanwhile; scans left of the delayed stock press; A 440 preview sounding */
    u8 pvk; s8 pvo; u8 kvc, sfst;  /* 2.123: the preview's note is marked (its note-off too); semitones it adds to key index 9; the next voice kv_apply refreshes; 2.126: the first count change after the start */
    unsigned short pvt, pvtl;      /* 2.123: last volume change; when the preview stopped */
    u8 b3a, b3l, b3req, b3st;      /* 2.127: the third bank's voice applied (0 none); the last one picked; a request from the yellow VOICE (1 enter, 2 leave); 2.128: entering, step */
    unsigned short yt0, b3t;       /* 2.127: when the yellow VOICE went down; 2.128: when the entering step began */
    u8 vinj, sdp, sprv, ekk;       /* 2.128: scans left of a VOICE 5 press for the stock (picks the banjo); 2.129: the drum steps' own position; a step picked in edit mode (preview); the preview's note key (0xFF none) */
    unsigned short ekt, ypt0;      /* 2.129: when the preview's note went down; 2.130: when the yellow PATTERN went down */
    u8 p3a, p3cur, yph, ypu;       /* 2.130: the PATTERN bank 3 (the saved drum patterns) is on; its slot loaded (0 none); the yellow PATTERN held (ours); its long press was used */
    u8 p3inj, p3boot, scl, scy;    /* 2.130: scans left of the delayed stock press; the slot is to be loaded after power-on; 2.132: the scale strum on (bit 0); KEYBOARD + yellow VOICE held (bit 0), yellow VOICE down (bit 1), the plates are the scale's (bit 2) */
    u32 ph, php, rng, koff, pnew;  /* the note keys physically down (bit = key id - 12), this scan / the scan before; a random state; the keys the arpeggio pressed an octave up (bit = index in SQ_KT); fresh note keys */
    unsigned short at0, gap, kt0, lst;   /* the arpeggio's step start, gap to the next, when it stopped; the rhythm's step counter last tick */
    unsigned short sbw, sbd, sg0, slst, lsm;   /* step / page buttons swallowed / down last scan; the sequencer's step start, its step counter last tick; the LEDs' states to put back */
    unsigned short sgap, bt0, bst, bq;         /* the gap of the sequencer step now playing (ms); the Start LED's pulse start; the rhythm's step counter for it; the free-running quarter note (arpeggio alone) */
    u8 pcf, cut, res, tst;                     /* 2.133: the drum slot (1-5) waiting for the overwrite answer (0 none); 2.135: the keyboard filter's cutoff / resonance knob values (0 = none), the tempo knob's state (bit 0 seen, bit 1 locked, bit 2 Bb minor held) */
    u8 tref, tfz, dtm, dfb;                    /* 2.135: the tempo reading the lock is taken at; the stock tempo is frozen (read by tempo_hook); 2.137: the delay's time / feedback knob values (0 = none) */
    u8 gbu, rvm, rvd, pad18;                          /* 2.141: gbu unused now; rvm / rvd = the reverb's amount / decay knob values (0 = off), not saved */
    u32 lmk; int gl0;                          /* 2.144: the arpeggio latch's held keys (bit k - 12); the glide's start offset (1/256 cent), 0 = none */
    unsigned short glt; u8 lat, lfz;           /* the glide's start (TICKS, low 16 bits); the latch is on; all keys were let go since the last press (the next press starts a new set) */
    u8 gln, glv, pad19[2];                     /* the last keyboard note played (the note the next glide starts from); gln is valid */
} sq_t;
typedef struct {
    u32 xmagic;
    u8 seq[SQ_MAX][2];             /* the Hold list: root class | type << 4; the stock's row | jazz extra << 4 */
    u8 stn[32], std[32];           /* the note / drum steps: each step's note (index in SQ_KT, 0xFF none) and its two drum pads (layer 1 low nibble, layer 2 high nibble: 0-7, 15 none) */
    u8 cs[SQ_CSTEPS][2];           /* the chord steps (same format as the Hold list; 0xFF = empty) */
    u8 stx[32];                    /* 2.119: per drum step, bit 0 / 1 = layer 1 / 2 holds the pad's EXTRA sound (SQ_ALT) */
    u8 sto[32];                    /* 2.129: per step, the order things were written (2 bits each, newest in bits 0-1: 1 note, 2 drum layer 1, 3 drum layer 2): Hold takes the newest out */
    u8 sno[32];                    /* 2.146: per note step, the keyboard octave (OCT 0-2) it was written at; 0xFF = not known (an old sequence: the step plays in the octave of the first one) */
} sqx_t;
_Static_assert(sizeof(sqx_t) <= 0x380u, "sqx_t: below the drum volumes in the OCRAM");
/* 2.119 (owner): a volume per drum instrument (0 silent .. 127 as recorded), applied to every hit of that instrument (patterns, pads, plates, the sequencer, MIDI in),
   in the OCRAM after sqx_t; saved at power-off as settings records ver 4 (mod2.c: 3 records of 11 values) and loaded from them the first time it is used. */
#define DV_N      25u
/* 2.123: the same records also keep the keyboard-mode voice volumes (part 3) and the arpeggio settings (part 4): one flat array of 5 parts x 11 bytes, byte
   part x 11 + j = record byte 3 + j. 0-24 drum volumes, 33-42 keyboard-mode volume per voice (DE89), 44-47 arpeggio mode / note value / length / swing. */
typedef struct { u32 magic; u8 v[DV_N]; u8 rs; u8 pad0[7]; u8 kv[10]; u8 pad1; u8 ar[4]; u8 kv3[5]; u8 b3; u8 p3s; } dv_t;   /* 2.127: 48-52 the third bank's keyboard volumes, 53 its voice (0 = not in it) */
#define DV_PARTS  5u
#define DV_BYTES  (DV_PARTS * 11u)
_Static_assert(__builtin_offsetof(dv_t, kv) == 4u + 33u && __builtin_offsetof(dv_t, ar) == 4u + 44u && sizeof(dv_t) >= 4u + DV_BYTES, "dv_t: the record layout");
#define DVP       ((dv_t *)((u8 *)SQ_XP + 0x380u))
#define DVMAGIC   0x44564F4Du                   /* 2.123: new layout (2.119-2.122: ...4C) */
static u32 dv_def(u32 n) { return n == 44u ? 0u : n == 45u ? 1u : n == 46u ? 64u : n == 47u ? 85u : n == 53u ? 0u : n == 54u ? 0u : 127u; }   /* 2.130: 54 = the PATTERN bank 3 (bit 7) and its slot */   /* arpeggio off, eighths, length 64, swing 85 */
static void dv_load(dv_t *D);
static dv_t *dv(void) {
    dv_t *D = DVP;
    if (D->magic != DVMAGIC) { volatile u8 *w = D->v; for (u32 i = 0; i < DV_BYTES; i++) w[i] = (u8)dv_def(i); dv_load(D); D->magic = DVMAGIC; }
    return D;
}
/* 2.130: the drum-pattern slots (Bb minor + PATTERN in the sequencer's edit mode; the PATTERN bank 3): 5 slots of 44 bytes in the OCRAM after the 1 KB block:
   32 drum step bytes (XQ->std), 8 bytes of the extra-sound bits (2 per step), [40] = used. Saved as records ver 5 (20 parts of 11 bytes), as the volumes (ver 4). */
#define PT_SLOT   44u
#define PT_PARTS  20u
#define PT_BYTES  (PT_PARTS * 11u)
typedef struct { u32 magic; u8 b[PT_BYTES]; } pt_t;
#ifndef PTP
#define PTP       ((pt_t *)((u8 *)SQ_XP + 0x400u))
#endif
#define PTMAGIC   0x50544E31u
static u32 pt_def(u32 n) { return (n % PT_SLOT) < 32u ? 0xFFu : 0u; }
#ifdef STATE_SECTOR
static int st_valid(const volatile u8 *r, u32 ver, u32 parts) { return r[0] == 0xA5 && r[1] == ver && r[2] < parts && r[15] == 0 && r[14] == rec_sum(r); }
static void st_load(volatile u8 *w, u32 ver, u32 parts, u32 clamp) {   /* the newest record of each part wins (the records are in write order) */
    for (int i = 0; i < 256; i++) {
        const volatile u8 *r = (const volatile u8 *)(STATE_SECTOR + 16u * (u32)i);
        if (!st_valid(r, ver, parts)) continue;
        for (u32 j = 0; j < 11u; j++) w[r[2] * 11u + j] = (u8)(clamp && r[3 + j] > 127u ? 127u : r[3 + j]);
    }
}
static void st_build(const volatile u8 *b, u32 ver, u32 part, u8 *r0) {
    volatile u8 *r = r0;
    for (int i = 0; i < 16; i++) r[i] = 0;
    r[0] = 0xA5; r[1] = (u8)ver; r[2] = (u8)part;
    for (u32 j = 0; j < 11u; j++) r[3 + j] = b[part * 11u + j];
    r[14] = rec_sum(r);
}
/* the parts to write: all = 0: those that differ from their newest record (none: from the defaults); all = 1 (after an erase): those not at the defaults */
static u32 st_need(const volatile u8 *b, u32 ver, u32 parts, u32 (*def)(u32), int all) {
    u32 m = 0;
    for (u32 part = 0; part < parts; part++) {
        u8 r[16]; st_build(b, ver, part, r);
        int last = -1;
        if (!all) for (int i = 0; i < 256; i++) { const volatile u8 *o = (const volatile u8 *)(STATE_SECTOR + 16u * (u32)i); if (st_valid(o, ver, parts) && o[2] == part) last = i; }
        int diff = 0;
        if (last >= 0) { const volatile u8 *o = (const volatile u8 *)(STATE_SECTOR + 16u * (u32)last); for (u32 j = 3; j <= 13u; j++) if (o[j] != r[j]) diff = 1; }
        else for (u32 j = 0; j < 11u; j++) if (r[3 + j] != def(part * 11u + j)) diff = 1;
        if (diff) m |= 1u << part;
    }
    return m;
}
static void dv_load(dv_t *D) { st_load(D->v, 4u, DV_PARTS, 1u); }
static void dv_build(u32 part, u8 *r) { st_build(DVP->v, 4u, part, r); }
static u32 dv_need(int all) { return DVP->magic == DVMAGIC ? st_need(DVP->v, 4u, DV_PARTS, dv_def, all) : 0u; }   /* never loaded or touched: nothing changed */
static void pt_load(pt_t *T) { st_load(T->b, 5u, PT_PARTS, 0u); }
static void pt_build(u32 part, u8 *r) { st_build(PTP->b, 5u, part, r); }
static u32 pt_need(int all) { return PTP->magic == PTMAGIC ? st_need(PTP->b, 5u, PT_PARTS, pt_def, all) : 0u; }
#else
static void dv_load(dv_t *D) { (void)D; }
static void pt_load(pt_t *T) { (void)T; }
#endif
static pt_t *pt(void);
static u32 sq_sto_rm(u32 o, u32 it);
static void pt_save(u32 k) {                           /* 2.130: the sequencer's drums (both layers, the extra sounds) into slot k (1-5) */
    volatile u8 *b = pt()->b + (k - 1u) * PT_SLOT;
    for (u32 q = 0; q < 32u; q++) b[q] = XQ->std[q];
    for (u32 j = 0; j < 8u; j++) { u32 v = 0; for (u32 q = 0; q < 4u; q++) v |= (XQ->stx[j * 4u + q] & 3u) << (2u * q); b[32u + j] = (u8)v; }
    b[40] = 1u;
}
static void pt_loadslot(u32 k) {                       /* 2.130: slot k into the sequencer's drums (an unused slot: none); the write order starts again */
    const volatile u8 *b = pt()->b + (k - 1u) * PT_SLOT;
    for (u32 q = 0; q < 32u; q++) {
        XQ->std[q] = b[40] ? b[q] : 0xFFu;
        XQ->stx[q] = b[40] ? (u8)((b[32u + q / 4u] >> (2u * (q % 4u))) & 3u) : 0u;
    }
    for (u32 q = 0; q < 32u; q++) XQ->sto[q] = (u8)sq_sto_rm(sq_sto_rm(XQ->sto[q], 2u), 3u);   /* the drums came in at once: only the note keeps its place */
}
static pt_t *pt(void) {
    pt_t *T = PTP;
    if (T->magic != PTMAGIC) { volatile u8 *w = T->b; for (u32 i = 0; i < PT_BYTES; i++) w[i] = (u8)pt_def(i); pt_load(T); T->magic = PTMAGIC; }
    return T;
}
/* 2.135 (owner): cutoff + resonance on the keyboard-mode notes (the strum-voice bus A of the mixer: in keyboard mode it carries the note keys, hence the arpeggio and
   the note sequencer; the plates play drums on bus B). Bb minor + the Sustain knob = cutoff, Bb minor + the Tempo knob = resonance; the stock tempo does not change
   while Bb minor is held, nor after, until the knob is moved again (tempo_hook skips the stock tempo routine); both at 0 = off. A Chamberlin state-variable low-pass
   (the lp output) run by mix_tail on the 16 frames of bus A; the control task sets f / q / on. Not saved: off at power-on. OCRAM SQ_XP + 0x3C0 (below the pattern slots). */
typedef struct { int lp, bp; unsigned f, q, on; } flt_t;
#define FLTB      ((flt_t *)((u8 *)SQ_XP + 0x3C0u))
static const unsigned short FTAB[128] = {
    16384, 15903, 15433, 14975, 14527, 14091, 13666, 13252, 12849, 12456, 12075, 11703, 11342, 10992, 10651, 10320,
    9998, 9686, 9383, 9089, 8803, 8526, 8258, 7997, 7745, 7500, 7262, 7032, 6809, 6592, 6383, 6180,
    5983, 5792, 5607, 5428, 5255, 5087, 4925, 4767, 4615, 4467, 4324, 4185, 4051, 3921, 3795, 3674,
    3556, 3442, 3331, 3224, 3121, 3020, 2923, 2829, 2739, 2650, 2565, 2483, 2403, 2326, 2251, 2178,
    2108, 2041, 1975, 1911, 1850, 1790, 1733, 1677, 1623, 1571, 1520, 1471, 1424, 1378, 1334, 1291,
    1249, 1209, 1170, 1132, 1096, 1061, 1026, 993, 961, 930, 900, 871, 843, 816, 790, 764,
    740, 716, 693, 671, 649, 628, 608, 588, 569, 551, 533, 516, 499, 483, 468, 453,
    438, 424, 410, 397, 384, 372, 360, 348, 337, 326, 316, 306, 296, 286, 277, 268 };
/* 2.137 (owner): a delay on the same keyboard-mode bus, after the filter: hold Gb 7th (key 0x35, the mode pick's button; keyboard mode only) + the Sustain knob = the
   time (50 ms .. 0.52 s; 2.141: hold Gb MINOR (key 0x32) instead, 50 ms .. 0.44 s), + the Tempo knob = the repeats (feedback); the echo is mixed in at half level. Both 0 = off. The stock tempo and the note length do not change
   meanwhile (as with the filter), and the held-Gb-7th release does not cycle the modulation mode when a knob was used. A 16-bit ring buffer of DLYN samples at
   SQ_XP + 0x2000 (OCRAM, 0x20212000..0x2021E000, nothing else uses it); the state at SQ_XP + 0x500 (2.141). The time glides (a few samples per block) so a turn does not click. */
typedef struct { unsigned on, wr, d, dt, fb, clr, magic; } dly_t;   /* 2.138: + magic: RAM at power-on is not zero (2.137 used garbage as buffer indices and hung the unit) */
#define DLYMAGIC  0x444C5931u
#define DLYB      ((dly_t *)((u8 *)SQ_XP + 0x500u))   /* 2.141: was +0x3D8, whose last word (magic) overlapped the third bank's B3RT at +0x3F0 */
#define DLYBUF    ((short *)((u8 *)SQ_XP + 0x2000u))
#define DLYN      21000u                                /* 2.141: was 24576 (0.52 s): the reverb's buffer follows this one in the same OCRAM */
static void dly_mix(int *buf) {
    dly_t *L = DLYB; short *m = DLYBUF;
    if (L->magic != DLYMAGIC || L->on != 1u) return;        /* never touches the buffer until sq_tick has set the state up and the delay is used */
    if (L->clr < DLYN) {                                    /* turned on: the buffer is cleared first, a little each block (the sound passes untouched meanwhile) */
        volatile short *mv = m; u32 c = L->clr; for (u32 k = 0; k < 64u && c < DLYN; k++, c++) mv[c] = 0;   /* (volatile: no memclr call here) */
        L->clr = c < DLYN ? c : DLYN; return;
    }
    u32 w = L->wr, d = L->d, dt = L->dt; int fb = (int)L->fb;
    if (w >= DLYN) w = 0;
    if (dt >= DLYN) dt = DLYN - 1u;
    if (d >= DLYN) d = dt;
    if (fb > 127) fb = 127;
    if (d + 3u <= dt) d += 3u; else if (d >= dt + 3u) d -= 3u; else d = dt;
    for (int i = 0; i < 16; i++) {
        int x = buf[2 * i];
        u32 r = w >= d ? w - d : w + DLYN - d;
        int e = (int)m[r];                                  /* the echo, x >> 14 */
        long long y = (long long)x + ((long long)e * 8192);
        buf[2 * i] = y > 0x7FFFFFFFLL ? 0x7FFFFFFF : y < -0x7FFFFFFFLL ? -0x7FFFFFFF : (int)y;
        int v = (x >> 14) + ((e * fb) >> 7);
        m[w] = (short)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
        if (++w >= DLYN) w = 0;
    }
    L->wr = w; L->d = d;
}
/* 2.141 (owner): a reverb on the same bus, after the delay: hold Db minor (key 12; keyboard mode only) + the Sustain knob = the amount, + the Tempo knob = the decay;
   amount 0 = off (and not saved: off at power-on). A small Schroeder reverb (4 damped feedback combs in parallel, 2 allpasses in series) run at HALF the sample
   rate (two frames averaged in, the output interpolated): 3229 16-bit samples at SQ_XP + 0x2000 + 2 x DLYN, right after the delay's buffer (OCRAM the delay already
   uses on the unit); the state at SQ_XP + 0x520. The stock tempo and the note length do not change meanwhile (as with the filter and the delay). */
typedef struct { unsigned on, magic, clr, amt, fb; int prev; unsigned short ix[6]; int lp[4]; } rvb_t;
#define RVBMAGIC  0x52564231u
#define RVBB      ((rvb_t *)((u8 *)SQ_XP + 0x520u))
#define RVBBUF    ((short *)((u8 *)SQ_XP + 0x2000u + 2u * DLYN))
#define RVN       3229u
static const unsigned short RV_LEN[6] = { 607, 646, 695, 738, 303, 240 };      /* 4 combs, 2 allpasses (Freeverb's lengths x 24/44.1) */
static const unsigned short RV_OFF[6] = { 0, 607, 1253, 1948, 2686, 2989 };
static int rv_sat(int v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : v; }
/* 2.144 (owner): glide while Db minor (the reverb's key) is held in keyboard mode: a new note starts from the last note's pitch and slides to its own in GLD_MS, as a global
   pitch offset (1/256 cent) the vibrato engine adds to the sample tune and the analog oscillators (vib_pitch); all sounding notes bend with it. Not saved. */
#define GLD_MS 120u
static u32 sq_arp_cover(void) { const sq_t *Q = (const sq_t *)SQB; return Q->magic == SQMAGIC && (Q->arp || Q->lat); }   /* 2.145 */
static u32 sq_ph_now(void) { const sq_t *Q = (const sq_t *)SQB; return Q->magic == SQMAGIC ? Q->ph : 0u; }
static int gl_cents(void) {
    sq_t *Q = (sq_t *)SQB;
    if (Q->magic != SQMAGIC || !Q->gl0) return 0;
    u32 el = (TICKS - Q->glt) & 0xFFFFu;
    if (el >= GLD_MS) { Q->gl0 = 0; return 0; }
    return (Q->gl0 * (int)(GLD_MS - el)) / (int)GLD_MS;                          /* |gl0| <= 614400, GLD_MS 120: fits an int */
}
static void gl_note(u32 n) {                              /* every keyboard sample note-on (kb_note_on) */
    sq_t *Q = (sq_t *)SQB;
    if (Q->magic != SQMAGIC) return;
    if (Q->glv && LED(0x15) && cf(CF_EXTRAS) && R8(KEY(12u) + 4u)) {
        int d = ((int)Q->gln - (int)n) * 25600;           /* semitones -> 1/256 cent */
        if (d > 614400) d = 614400; else if (d < -614400) d = -614400;
        Q->gl0 = d; Q->glt = (unsigned short)TICKS;
    }
    Q->gln = (u8)n; Q->glv = 1u;
}
static void rvb_mix(int *buf) {
    rvb_t *R = RVBB; short *m = RVBBUF;
    if (R->magic != RVBMAGIC || R->on != 1u) return;         /* never touches the buffer until sq_tick has set the state up and the reverb is used */
    if (R->clr < RVN) {                                      /* turned on: the buffer is cleared first, a little each block (the sound passes untouched meanwhile) */
        volatile short *mv = m; u32 c = R->clr; for (u32 k = 0; k < 64u && c < RVN; k++, c++) mv[c] = 0;
        R->clr = c < RVN ? c : RVN; return;
    }
    int fb = (int)R->fb, amt = (int)R->amt, prev = R->prev; u32 ix[6]; int lp[4];
    if (fb > 31000) fb = 31000;
    if (amt > 127) amt = 127;
    for (u32 j = 0; j < 6u; j++) ix[j] = R->ix[j] < RV_LEN[j] ? R->ix[j] : 0u;
    for (u32 j = 0; j < 4u; j++) lp[j] = R->lp[j];
    for (int s = 0; s < 8; s++) {
        int x = ((buf[4 * s] >> 14) + (buf[4 * s + 2] >> 14)) >> 1;        /* 2 frames -> 1, in the int16 range */
        int in = x >> 3, acc = 0;
        for (u32 j = 0; j < 4u; j++) {                                      /* parallel damped combs */
            short *c = m + RV_OFF[j] + ix[j]; int o = *c;
            lp[j] += ((o - lp[j]) * 22938) >> 15;
            *c = (short)rv_sat(in + ((lp[j] * fb) >> 15));
            acc += o;
            if (++ix[j] >= RV_LEN[j]) ix[j] = 0;
        }
        int y = acc >> 2;
        for (u32 j = 4; j < 6u; j++) {                                      /* series allpasses, feedback 0.5 */
            short *c = m + RV_OFF[j] + ix[j]; int bo = *c;
            *c = (short)rv_sat(y + (bo >> 1));
            y = rv_sat(bo - y);
            if (++ix[j] >= RV_LEN[j]) ix[j] = 0;
        }
        int y0 = (prev + y) >> 1; prev = y;                                 /* back to 48 kHz: the first of the two frames is the midpoint */
        long long a = (long long)buf[4 * s] + ((long long)(y0 * amt) << 7), b = (long long)buf[4 * s + 2] + ((long long)(y * amt) << 7);
        buf[4 * s]     = a > 0x7FFFFFFFLL ? 0x7FFFFFFF : a < -0x7FFFFFFFLL ? -0x7FFFFFFF : (int)a;
        buf[4 * s + 2] = b > 0x7FFFFFFFLL ? 0x7FFFFFFF : b < -0x7FFFFFFFLL ? -0x7FFFFFFF : (int)b;
    }
    for (u32 j = 0; j < 6u; j++) R->ix[j] = (unsigned short)ix[j];
    for (u32 j = 0; j < 4u; j++) R->lp[j] = lp[j];
    R->prev = prev;
}
static void flt_mix(int *buf) {
    flt_t *F = FLTB;
    if (F->on != 1u) { F->lp = 0; F->bp = 0; return; }       /* (2.138: RAM at power-on is not zero) */
    long long f = F->f, q = F->q; int lp = F->lp, bp = F->bp;
    for (int i = 0; i < 16; i++) {
        int x = buf[2 * i] >> 6;                              /* 25 bits: room for the resonance's gain */
        lp += (int)((f * bp) >> 14);
        int hp = x - lp - (int)((q * bp) >> 14);
        bp += (int)((f * hp) >> 14);
        if (lp > 0x1FFFFFFF) lp = 0x1FFFFFFF; else if (lp < -0x1FFFFFFF) lp = -0x1FFFFFFF;
        if (bp > 0x1FFFFFFF) bp = 0x1FFFFFFF; else if (bp < -0x1FFFFFFF) bp = -0x1FFFFFFF;
        int o = lp > 0x1FFFFFF ? 0x1FFFFFF : lp < -0x1FFFFFF ? -0x1FFFFFF : lp;
        buf[2 * i] = o * 64;
    }
    F->lp = lp; F->bp = bp;
}
/* The two stock drum hit functions in the ITCM: 0x20200344 (instrument, choke) at the fixed level 127 (drum keys, plates, Classic rhythm, MIDI in) and 0x20200488
   (instrument, choke, velocity) (Normal rhythm). They are identical but for the level, and 0x20200344 then calls 0x60008674 (the auto-off timer). The flash code
   reaches them only through the veneers 0x600187C0 / 0x600187E8, whose targets make_patch.py points at drum_fix / drum_vel (entry +0x94 / +0x98). 1.0.3.0 only. */
#ifndef F_HIT
#if FW_VERSION == 30
#define R344   ((void(*)(u32, u32))0x20200345u)
#define R488   ((void(*)(u32, u32, u32))0x20200489u)
#define F_8674 ((void(*)(void))0x60008675u)
#define F_HIT  ((void(*)(u32, u32))0x600187C1u)        /* the veneer: a hit at the instrument's volume (ours, through drum_fix) */
#define SQ_PADT 0x60033370u                             /* stock: the drum keys' instruments, 8 per tone (0 Normal, 1 Classic) */
#else
static void R344(u32 i, u32 c) { (void)i; (void)c; }
static void R488(u32 i, u32 c, u32 v) { (void)i; (void)c; (void)v; }
static void F_8674(void) { }
static void F_HIT(u32 i, u32 c) { (void)i; (void)c; }
#define SQ_PADT 0u
#endif
#endif
static const u32 DV_F[128] = {   /* v / 127 as IEEE floats (this code has no floating point) */
    0x00000000u, 0x3C010204u, 0x3C810204u, 0x3CC18306u, 0x3D010204u, 0x3D214285u, 0x3D418306u, 0x3D61C387u,
    0x3D810204u, 0x3D912245u, 0x3DA14285u, 0x3DB162C6u, 0x3DC18306u, 0x3DD1A347u, 0x3DE1C387u, 0x3DF1E3C8u,
    0x3E010204u, 0x3E091224u, 0x3E112245u, 0x3E193265u, 0x3E214285u, 0x3E2952A5u, 0x3E3162C6u, 0x3E3972E6u,
    0x3E418306u, 0x3E499326u, 0x3E51A347u, 0x3E59B367u, 0x3E61C387u, 0x3E69D3A7u, 0x3E71E3C8u, 0x3E79F3E8u,
    0x3E810204u, 0x3E850A14u, 0x3E891224u, 0x3E8D1A34u, 0x3E912245u, 0x3E952A55u, 0x3E993265u, 0x3E9D3A75u,
    0x3EA14285u, 0x3EA54A95u, 0x3EA952A5u, 0x3EAD5AB5u, 0x3EB162C6u, 0x3EB56AD6u, 0x3EB972E6u, 0x3EBD7AF6u,
    0x3EC18306u, 0x3EC58B16u, 0x3EC99326u, 0x3ECD9B36u, 0x3ED1A347u, 0x3ED5AB57u, 0x3ED9B367u, 0x3EDDBB77u,
    0x3EE1C387u, 0x3EE5CB97u, 0x3EE9D3A7u, 0x3EEDDBB7u, 0x3EF1E3C8u, 0x3EF5EBD8u, 0x3EF9F3E8u, 0x3EFDFBF8u,
    0x3F010204u, 0x3F03060Cu, 0x3F050A14u, 0x3F070E1Cu, 0x3F091224u, 0x3F0B162Cu, 0x3F0D1A34u, 0x3F0F1E3Cu,
    0x3F112245u, 0x3F13264Du, 0x3F152A55u, 0x3F172E5Du, 0x3F193265u, 0x3F1B366Du, 0x3F1D3A75u, 0x3F1F3E7Du,
    0x3F214285u, 0x3F23468Du, 0x3F254A95u, 0x3F274E9Du, 0x3F2952A5u, 0x3F2B56ADu, 0x3F2D5AB5u, 0x3F2F5EBDu,
    0x3F3162C6u, 0x3F3366CEu, 0x3F356AD6u, 0x3F376EDEu, 0x3F3972E6u, 0x3F3B76EEu, 0x3F3D7AF6u, 0x3F3F7EFEu,
    0x3F418306u, 0x3F43870Eu, 0x3F458B16u, 0x3F478F1Eu, 0x3F499326u, 0x3F4B972Eu, 0x3F4D9B36u, 0x3F4F9F3Eu,
    0x3F51A347u, 0x3F53A74Fu, 0x3F55AB57u, 0x3F57AF5Fu, 0x3F59B367u, 0x3F5BB76Fu, 0x3F5DBB77u, 0x3F5FBF7Fu,
    0x3F61C387u, 0x3F63C78Fu, 0x3F65CB97u, 0x3F67CF9Fu, 0x3F69D3A7u, 0x3F6BD7AFu, 0x3F6DDBB7u, 0x3F6FDFBFu,
    0x3F71E3C8u, 0x3F73E7D0u, 0x3F75EBD8u, 0x3F77EFE0u, 0x3F79F3E8u, 0x3F7BF7F0u, 0x3F7DFBF8u, 0x3F800000u,
};
/* 2.120: the engine IGNORES the velocity for drums (its drum envelope is fixed, 0x20200A78), so 2.119's velocity did nothing. A drum channel (78 + instrument)
   is rendered at its envelope x a per-channel gain float that the hit sets to 1.0 (DRM_G = 0x20006E18 + channel x 20 + 0x10, read by the renderer's drum
   loop 0x202005EC / 0x20200FA0): the stock hit runs as it is, then that gain is set to volume / 127, with interrupts off so no audio block hears 1.0.
   Only in the Normal (108) tone (owner: the Classic / 84 tone always at the stock level; the volumes stay, unused, for when 108 is back). */
#ifndef DRM_G
#define DRM_G(ch) (0x20006E18u + (ch) * 20u + 0x10u)
#endif
#ifndef DV_IRQ_OFF
#define DV_IRQ_OFF(pm) __asm volatile ("mrs %0, primask\n cpsid i" : "=r"(pm) :: "memory")
#define DV_IRQ_ON(pm)  __asm volatile ("msr primask, %0" :: "r"(pm) : "memory")
#endif
static u32 dv_vol(u32 i) { return (i < DV_N && R8(SQ_TONE) == 0u) ? dv()->v[i] : 127u; }
static void dv_after(u32 i, u32 v) { R32(DRM_G(78u + (i > 24u ? 24u : i))) = DV_F[v]; }
void drum_fix(u32 i, u32 c) {
    u32 v = dv_vol(i);
    if (v >= 127u) { R344(i, c); return; }
    if (!v) { F_8674(); return; }                    /* silent: no hit (the stock's auto-off timer still sees the press) */
    u32 pm; DV_IRQ_OFF(pm);
    R344(i, c); dv_after(i, v);
    DV_IRQ_ON(pm);
}
void drum_vel(u32 i, u32 c, u32 vel) {
    u32 v = dv_vol(i);
    if (v >= 127u) { R488(i, c, vel); return; }
    if (!v) return;
    u32 pm; DV_IRQ_OFF(pm);
    R488(i, c, vel); dv_after(i, v);
    DV_IRQ_ON(pm);
}
/* 2.123 (owner): a volume per voice for keyboard mode only: the keyboard-mode copies of the voices (slot v + 12; omni 2's slot 13 is OM-84 Keys) get their
   melodic layers' volume ([13]) = the flash value (the voice table's init data, as the tool wrote it) x kv / 127; a boost layer (boost_set) follows its sub
   layer again. Chord mode (slots 0-9) and the plates are untouched. Re-applied every so often (the stock could reload the table). */
#ifdef VT
#define VT_FLASH(a) (0x60037F64u + ((a) - 0x20000000u))   /* 1.0.3.0: the DTCM init data (copied to 0x20000000 at power-on) */
static void kv_apply(u32 v) {
    if (v >= NV || FW_VERSION != 30) return;
    u32 kv = (v == 9u && ((sq_t *)SQB)->b3a) ? dv()->kv3[((sq_t *)SQB)->b3a - 1u] : dv()->kv[v], c = v + 12u;   /* 2.127: the third bank's own */
    for (u32 l = 0; l < 3u; l++) {
        u32 b = VT + c * 0x40u + l * 16u;
        int boost = v != 1u && BOOST_L[v] && l == BOOST_L[v] && R8(b) == 1u && R8(b + 1u) == 1u;
        if (!boost) R8(b + 13u) = (u8)((R8(VT_FLASH(b + 13u)) * kv + 63u) / 127u);
    }
    if (v != 1u) boost_set(c, v, R8(VT + c * 0x40u + SUB_OFF[v]));
}
#else
static void kv_apply(u32 v) { (void)v; }
#endif
/* 2.127 (owner): a third voice bank (the yellow VOICE held, both its lights on) of 5 one-sample voices. They borrow the banjo's slot (voice 9): the banjo
   is a one-zone instrument (voices/make_voices.py), so the engine's RAM table of the strum bank's sample addresses (0x202012A8 + 4 x sample, built by the
   ITCM loader 0x202011F8 at power-on) gets sample 93 = the picked sound; its sub = the owner's pick (set_sub, slot 9 and 21). Leaving: the banjo's sound,
   sub and alt back. All 6 sounds have the same length and loop. 1.0.3.0 only. */
#define B3_SMP0   0x60918AD4u
#define B3_STRIDE 66152u
#define B3_SI     93u
static const u8 B3_SUB[5] = { 2, 18, 3, 2, 19 };      /* Mellow Pad, Strings, Sine Pad, Mellow Pad, Strings 2 */
#ifdef VT
#define B3RT ((volatile u8 *)(SQ_XP_ADDR + 0x3F0u))  /* [0] = 0xB3 valid, [1] the voice applied, [2] the banjo's alt sub was on (mod2.c alt_on) */
#define B3_OK (FW_VERSION == 30 && cf(CF_EXTRAS))
/* 2.139 (owner): the sub of each third-bank voice is the tool's pick: settings block [38..42] (VOICE 1..5); 0 / invalid / no block = the default above */
static u8 b3sub(u32 k) {
    u32 i = cfg_ok() ? R8(CFG + 37u + k) : 0u;
    return (i < 32u && ((SUBS_OK >> i) & 1u)) ? (u8)i : B3_SUB[k - 1u];
}
/* 2.141: (2.140's b3_dec removed: the layer byte [3] is the layer's OCTAVE shift (note + 12 x byte), not a decay) */
static void b3_apply(sq_t *Q, u32 k) {
    if (B3RT[0] != 0xB3u) { B3RT[0] = 0xB3u; B3RT[1] = 0; B3RT[2] = 0; }
    if (k && !Q->b3a) B3RT[2] = (u8)alt_on(9u);           /* the banjo's alt sub, before the slot is borrowed */
    B3RT[1] = 0;                                         /* alt_on reads the VT again while we set the sub */
    R32(0x202012A8u + 4u * B3_SI) = B3_SMP0 + (k ? k : 0u) * B3_STRIDE;
    if (k) set_sub(9u, b3sub(k)); else set_sub(9u, B3RT[2] ? alt(9u) : nrm(9u));
    B3RT[1] = (u8)k; Q->b3a = (u8)k; if (k) Q->b3l = (u8)k;
    kv_apply(9u);
    F_E7DC(R8(0x20007624u));      /* 2.128: the slot's layers loaded again: the loader keeps each zone's sample ADDRESS (and the sub) per layer from load time */
}
#else
#define B3_OK 0
#ifndef F_E7DC
static void F_E7DC(u32 i) { (void)i; }
#endif
static void b3_apply(sq_t *Q, u32 k) { Q->b3a = (u8)k; if (k) Q->b3l = (u8)k; }
#endif
/* 2.119 (owner): B minor + a drum pad plays the pad's extra sound (the instruments keyboard mode has no pad for): sidestick, conga high, conga mid, tom mid, cowbell,
   tambourine, cabasa, claves (Normal-tone numbers; the same in the Classic tone) */
static const u8 SQ_ALT[8] = { 13, 4, 5, 17, 7, 15, 12, 3 };

_Static_assert(sizeof(sq_t) <= 0x100u && SQB + sizeof(sq_t) <= 0x2000E000u && (SQB & 3u) == 0, "sq_t: in the free stretch above the stock bss");
static sq_t *sq(void) {
    sq_t *Q = (sq_t *)SQB;
    if (Q->magic != SQMAGIC) {
        volatile u8 *b = (volatile u8 *)SQB;
        for (u32 i = 0; i < sizeof(sq_t); i++) b[i] = 0;
        Q->magic = SQMAGIC; Q->dv = 1u; Q->gt = 64u; Q->swv = 85u;        /* the default: the arpeggio off, on eighths */
        { const dv_t *D = dv(); Q->res = (u8)(127u - (D->rs < 127u ? D->rs : 127u)); }                 /* 2.142 (owner): the resonance as it was at power-off; the cutoff always starts at 0 (not saved) */
        { const dv_t *D = dv(); if (D->ar[0] <= 5u) Q->am = D->ar[0]; if (D->ar[1] < 5u) Q->dv = D->ar[1]; Q->gt = D->ar[2]; Q->swv = D->ar[3]; }   /* 2.123: as saved */
        Q->snk = 0xFFu; Q->sp = 0xFFu; Q->cp = 0xFFu; Q->bst = 0xFFFFu; Q->fpg = 0xFFu; Q->tpi = 0xFFu; Q->tl = 0xFFu; Q->sdp = 0xFFu; Q->ekk = 0xFFu;
        { const dv_t *D = dv(); Q->p3cur = (u8)(D->p3s & 7u); if (Q->p3cur > 5u) Q->p3cur = 0; Q->p3a = (u8)(D->p3s >> 7); Q->p3boot = 1u; }   /* 2.130: the PATTERN bank 3 as it was */
        XQ->xmagic = 0;                                                  /* the arrays start empty as well */
    }
    if (XQ->xmagic != SQXMAGIC) {
        volatile u8 *x = (volatile u8 *)XQ;
        for (u32 i = 0; i < sizeof(sqx_t); i++) x[i] = 0xFFu;           /* 0xFF = empty everywhere */
        { volatile u8 *z = XQ->stx; for (u32 q = 0; q < 32u; q++) z[q] = 0; z = XQ->sto; for (u32 q = 0; q < 32u; q++) z[q] = 0; }   /* 2.119: no extra sounds; 2.129: no order */
        XQ->xmagic = SQXMAGIC;
    }
    return Q;
}
/* the octave of the 'octaves' mode: kb_note (mod2.c: the stock's note + the keyboard Transpose) adds 12 to the note of a key the arpeggio pressed an octave up. The mark is
   per key and set when the arpeggio presses it (2.104: it was latched while no key was down, which stopped working when the knob made the notes legato: the
   keys never were all up), so a note-off, 30 ms after its key is let go, finds the mark of its own note-on. Cleared 80 ms after the arpeggio stops. */
static int sq_aoff(u32 note) {
    const sq_t *Q = (const sq_t *)SQB;
    if (Q->magic != SQMAGIC) return 0;
    int idx = (int)note - 0x30 - 12 * (int)R8(OCT);               /* the stock's note: 0x30 + index + 12 x octave */
    if (idx < 0 || idx >= 20) return 0;
    if (Q->pvk && idx == 9) return Q->pvo;                         /* 2.123: the volume preview: A 440 */
    return (((Q->koff >> idx) & 1u) ? 12 : 0) + Q->toff[idx];     /* 2.114: + the note sequence's transpose, marked as the sequencer pressed this key */
}
/* 2.146 (owner): a sequence over more than one octave. The note steps hold the key (0-19) only; the keyboard octave (OCT 0-2, the Up / Down buttons) it was played at is kept in
   sno and played again as the step's distance from the sequence's first note (12 semitones per octave), through the same per-key mark as the transpose (sq_aoff), so the whole
   sequence still follows the octave the keyboard is set to. */
__attribute__((noinline)) static int sq_octoff(u32 t) {
    u32 o = XQ->sno[t & 31u];
    if (o > 2u) return 0;
    for (u32 q = 0; q < 32u; q++) if (XQ->stn[q] < 20u && XQ->sno[q] <= 2u) return ((int)o - (int)XQ->sno[q]) * 12;
    return 0;
}
/* 2.114: the sequencer is about to press key index nt (0xFF: none any more): its transpose mark is set now; the key it pressed before keeps its mark 80 ms more (its
   note-off comes 30 ms after it is let go), then it is cleared, so a key the player presses later sounds as it is. */
static void sq_tmark(sq_t *Q, u32 nt, u32 now, int oc) {          /* 2.146: oc = the step's octave against the sequence's first note, in semitones */
    if (Q->tl != 0xFFu && Q->tl != nt && Q->toff[Q->tl]) {
        if (Q->tpi != 0xFFu && Q->tpi != Q->tl) Q->toff[Q->tpi] = 0;   /* an older one: at least a step (50 ms) ago */
        Q->tpi = Q->tl; Q->tpt = (unsigned short)now;
    }
    if (nt < 20u) { if (Q->tpi == nt) Q->tpi = 0xFFu; Q->toff[nt] = (signed char)((Q->trx ? (signed char)Q->tro : 0) + oc); }
    Q->tl = (u8)(nt < 20u ? nt : 0xFFu);
}
static u32 sq_on(void) { const sq_t *Q = (const sq_t *)SQB; return Q->magic == SQMAGIC && Q->on; }
static u32 sq_on_magic(void) { return ((const sq_t *)SQB)->magic == SQMAGIC; }   /* 2.122 */
static u32 sq_frc(void) { const sq_t *Q = (const sq_t *)SQB; return Q->magic == SQMAGIC && Q->frc; }   /* 2.113: the panel is held as Manual, Hold off (recording) */
/* 2.113: something is recorded for the sequencers to play in chord mode (the chord steps, or the note / drum steps): Start alone then starts them */
static u32 sq_has(const sq_t *Q) {
    for (u32 q = 0; q < 32u; q++) if (XQ->stn[q] != 0xFFu || XQ->std[q] != 0xFFu) return 1u;
    if (Q->cm & 1u) for (u32 q = 0; q < SQ_CSTEPS; q++) if (XQ->cs[q][0] != 0xFFu) return 1u;
    return 0;
}
/* 2.129 (owner): the write order of a step (sto): push an item (taking it out first if it is there), and Hold takes the newest one that is still there */
static u32 sq_sto_rm(u32 o, u32 it) { u32 r = 0, n = 0; for (u32 j = 0; j < 3u; j++) { u32 e = (o >> (2u * j)) & 3u; if (e && e != it) { r |= e << (2u * n); n++; } } return r; }
static void sq_sto_push(u32 t, u32 it) { XQ->sto[t] = (u8)(((sq_sto_rm(XQ->sto[t], it) << 2) | it) & 0x3Fu); }
static int sq_item_there(u32 t, u32 it) { return it == 1u ? XQ->stn[t] != 0xFFu : it == 2u ? (XQ->std[t] & 15u) != 15u : (XQ->std[t] >> 4) != 15u; }
static void sq_item_clear(u32 t, u32 it) {
    if (it == 1u) XQ->stn[t] = 0xFFu; else if (it == 2u) XQ->std[t] |= 0x0Fu; else XQ->std[t] |= 0xF0u;
    XQ->sto[t] = (u8)sq_sto_rm(XQ->sto[t], it);
}
static void sq_hold_step(u32 t) {
    for (u32 j = 0; j < 3u; j++) { u32 it = XQ->sto[t] & 3u; if (!it) break; XQ->sto[t] >>= 2; if (sq_item_there(t, it)) { sq_item_clear(t, it); return; } }
    static const u8 FB[3] = { 1, 3, 2 };                  /* no order known (cleared some other way): the note, then layer 2, then layer 1 */
    for (u32 j = 0; j < 3u; j++) if (sq_item_there(t, FB[j])) { sq_item_clear(t, FB[j]); return; }
}
/* 2.113 (owner): while chords are recorded in chord mode, Auto works as Manual (a chord sounds as long as its button is down, never with the beat) and the stock
   Hold is off (Hold is ours then: it deletes a chord); when the recording ends both are put back as they were. Manual/Auto goes through ma_set (the stock
   button's own branches, the third state included); Hold is its LED flag, which the stock button code reads. */
static void sq_force(sq_t *Q, u32 on) {
    if (on && !Q->frc) {
        Q->frc = 1u; Q->fma = (u8)ma_state(); Q->fho = (u8)(LED(0x10) != 0u);
        if (Q->fma != MA_MANUAL) ma_set(MA_MANUAL);
        if (Q->fho) F_LEDSET(0x10, 0);
    } else if (!on && Q->frc) {
        Q->frc = 0;
        if (ma_state() != Q->fma) ma_set(Q->fma);
        if (Q->fho && !LED(0x10)) F_LEDSET(0x10, 1);
    }
}
static int sq_beatmute(void) { const sq_t *Q = (const sq_t *)SQB; return Q->magic == SQMAGIC && Q->mute; }   /* r27_row / r27_bass, the rhythm interrupt: the stock rhythm's drums and bass rest */
static int sq_dmute(void) { const sq_t *Q = (const sq_t *)SQB; return Q->magic == SQMAGIC && Q->dmute; }   /* r27_row, the rhythm interrupt: the stock drums rest */
static u32 sq_pop(u32 x) { u32 n = 0; while (x) { n += x & 1u; x >>= 1; } return n; }
static u32 sq_btn(u32 i) { return i < 36u ? i : (i == 36u ? 0x32u : 0x35u); }   /* the 38 chord button ids */
static int sq_pat_down(void) {     /* a PATTERN button (27 mode's lock needs them) */
    for (u32 k = 0x28u; k <= 0x2Du; k++) if (key_down(k)) return 1;
    return 0;
}
static u32 sq_any(void) {          /* a chord button physically down (raw, last scan) */
    for (u32 i = 0; i < 38u; i++) if (R8(KEY(sq_btn(i)))) return 1;
    return 0;
}
static void sq_c12(u32 c0, u32 row, u8 *c1, u8 *c2) {     /* the 3rd and 5th classes of a root and row, as the stock's chord press works them out */
    int t1 = 0, t2 = 0;
    if (row >= 1u && row <= 8u) { t1 = SQ_T1[row - 1u]; t2 = SQ_T2[row - 1u]; }
    *c1 = (u8)(((int)c0 + 4 + t1 + 12) % 12); *c2 = (u8)(((int)c0 + 7 + t2 + 12) % 12);
}
static void sq_commit(sq_t *Q) {
    if (Q->cv && ((Q->cm & 1u) || Q->n < SQ_MAX)) {
        u32 c0 = Q->cc[0] % 12u, row = 15u;
        for (u32 r = 0; r < 9u; r++) { u8 a, b; sq_c12(c0, r, &a, &b); if (a == Q->cc[1] && b == Q->cc[2]) { row = r; break; } }   /* the stock's own row for these classes */
        u8 e0 = (u8)(c0 | (Q->ct << 4)), e1 = (u8)(row | (Q->cx << 4));
        if (Q->cm & 1u) { u32 t = Q->ss & 63u; XQ->cs[t][0] = e0; XQ->cs[t][1] = e1; }   /* a chord STEP: at the selected step */
        else { XQ->seq[Q->n][0] = e0; XQ->seq[Q->n][1] = e1; Q->n++; }                   /* a Hold list: the next place */
    }
    Q->cv = 0;
}
static void sq_expand(const u8 *e, u8 *o) {              /* a recorded chord -> the 3 classes, type | jazz extra << 4 sq_chord_on takes */
    u32 row = e[1] & 15u;
    o[0] = (u8)(e[0] & 15u); sq_c12(o[0], row > 8u ? 0u : row, &o[1], &o[2]);
    o[3] = (u8)((e[0] >> 4) | ((e[1] >> 4) << 4));
}
static void sq_toggle(sq_t *Q) {   /* KEYBOARD held long enough, in chord mode (row_filter) */
    if (Q->rec) { sq_commit(Q); Q->rec = 0; kb_blink_start(r27()); }
    else { Q->rec = 1; if (!(Q->cm & 1u)) Q->n = 0; Q->pos = 0; Q->cv = 0; Q->ign = 0; Q->want = 0; }   /* chord steps already there are kept: you go on editing them */
}
/* The stock's chord start / stop, called as the stock calls them: the three classes into CLS and the type into DE49, the chord-held
   flag DE64, F_8D24 = start the chord (oscillators + MIDI), F_8D38 / F_9184 = off (MIDI / oscillators).
   The strum plates' table (the stock's chord press, 0x6000C178 -> 0x6000BECC, re-tunes every sounding plate note, the keyboard notes
   with them: 2.88's trouble) is not touched. In the Auto state the rhythm plays the chord from CLS / DE49 / DE64 as it does for a held button. */
static void sq_chord_off(sq_t *Q) {
    if (!Q->on) return;
    F_8D38(MA_KB, MA_KS);                                  /* the held chord's MIDI notes off */
    if (Q->osc) F_9184(MA_KB, MA_KS);                      /* the oscillators off */
    if (Q->bass) { F_819C(0); Q->bass = 0; }
    if (Q->anc) { R8(SQ_ANC) = Q->a0; R8(SQ_ANC + 1) = Q->a1; Q->anc = 0; }   /* the anchor as the stock left it */
    R8(DE64) = 0; Q->on = 0; Q->osc = 0; Q->cm &= (u8)~2u; sq_jz_set(0);
    if (Q->hl) { if (Q->hl == 1u) F_LEDSET(0x10, 0); Q->hl = 0; }   /* the Hold LED as it was */
}
static void sq_chord_on(sq_t *Q, const u8 *e) {
    for (u32 j = 0; j < 3u; j++) R8(SQ_CLS + j) = e[j];
    R8(SQ_TYPE) = e[3] & 15u; R8(DE64) = 1u;
    sq_jz_set(e[3] >> 4);                                      /* jazz mode: the extra combination (m6/9, half-diminished, 13b9, minor-major 9) of the chord */
    if (!Q->anc) { Q->a0 = R8(SQ_ANC); Q->a1 = R8(SQ_ANC + 1); Q->anc = 1u; }
    R8(SQ_ANC) = e[0]; R8(SQ_ANC + 1) = e[3] & 15u;                 /* the anchor too: a Transpose press makes the stock apply the chord again from it (2.93: the Auto bass went) */
    if (!LED(0x15) || (!Q->ph && Q->snk == 0xFFu)) F_BECC(e[0], e[1], e[2], SQ_PT, SQ_ANC, MA_KB, MA_KS);   /* the strum plate plays this chord (chord mode; keyboard mode: no note sounds) */
    F_8D24(e[0], e[1], e[2], MA_KB, MA_KS); Q->osc = 1u;   /* as the stock's chord press does in every state; in Auto the rhythm then opens and closes the oscillators step by step */
    if (ma_state() == MA_AUTO) { F_819C(1); Q->bass = 1u; }  /* ... and the flag the rhythm's chord / bass row looks at */
    if (!Q->hl) { Q->hl = LED(0x10) ? 2u : 1u; if (!LED(0x10)) F_LEDSET(0x10, 1); }   /* the Hold LED lit: a sequence chord is on */
    Q->on = 1u;
}
/* keyboard mode, Bb minor held: a VOICE button picks the arpeggio's note value (the button is then not the stock's: the voice does not change) */
static u32 sq_voicesel(sq_t *Q, u32 row, u32 bits) {
    u32 kbe = LED(0x15) != 0u;
    for (u32 i = 0; i < 5u; i++) {
        u32 k = SQ_VK[i]; if (k / 9u != row) continue;
        u32 b = colbit(k), m = 1u << i, dn = (bits & b) != 0u;
        if (dn && !(Q->pv & m) && kbe && Q->bbm && !Q->se) { Q->vk |= (u8)m; Q->dv = (u8)i; kb_blink_start(r27()); }
        else if (dn && !(Q->pv & m) && Q->b3a && !Q->se && !Q->bbm) { Q->vk |= (u8)m; dv()->b3 = (u8)(i + 1u); }   /* 2.127: the third bank: its voice i + 1 */
        if (dn) Q->pv |= (u8)m; else { Q->pv &= (u8)~m; Q->vk &= (u8)~m; }
        if (Q->vk & m) bits &= ~b;
        if (i == 4u && Q->vinj) { Q->vinj--; bits |= b; }   /* 2.128: our press of VOICE 5 (the banjo), for the stock */
    }
    if (row == 4u) {                       /* 2.114: Bb minor + the yellow VOICE: the note sequence's transpose on / off (the button is then not the stock's) */
        u32 b = colbit(0x27u), m = 0x20u, dn = (bits & b) != 0u;
        if (dn && !(Q->pv & m) && kbe && Q->bbm && !Q->se) { Q->vk |= (u8)m; Q->tpoff ^= 1u; Q->tro = 0; kb_blink_start(r27()); }
        else if (dn && !(Q->pv & m) && (kbe || B3_OK) && !Q->se && !Q->yinj) { Q->yh = 1u; Q->yus = 0; Q->yt0 = (unsigned short)TICKS; }   /* 2.123: held alone: the knob may set the voice volume; 2.127: a long press = the third bank */
        if (!dn && Q->yh) {
            Q->yh = 0;
            if (!Q->yus) {                                                 /* let go, the knob unused: */
                if (Q->b3a) Q->b3req = 2u;                                              /* a tap in the third bank: back to the stock's banks */
                else Q->yinj = SQ_INJ;                                                  /* a tap: the stock gets its press (the voice bank) now */
            }
        }
        if (dn && Q->yh && !Q->yus && !Q->b3a && !Q->b3st && B3_OK && ((TICKS - Q->yt0) & 0xFFFFu) >= SQ_LONG) { Q->b3req = 1u; Q->yus = 1u; }   /* 2.128: held 0.7 s (the knob unused): the third bank, at once */
        if (dn) Q->pv |= (u8)m; else { Q->pv &= (u8)~m; Q->vk &= (u8)~m; }
        if ((Q->vk & m) || Q->yh) bits &= ~b;
        if (Q->yinj) { Q->yinj--; bits |= b; }
    }
    return bits;
}
/* The step sequencers' edit modes. Keyboard mode (2.105, 2.107): long press KEYBOARD; the notes and drums. Chord mode (2.108): while recording chords, a step
   button pressed BEFORE the first chord makes the chord memory 32 STEPS (a chord per step) instead of a Hold list; the same buttons then work the same way.
   The buttons under the LED rows are not the stock's then (no voice or pattern change). Voice 1-4 and pattern 1-4 = the step (1-8) of the page; the yellow
   VOICE = page 1 / 2 (it alternates, as the stock's voice bank does; from page 3 / 4 it jumps to page 1), the yellow PATTERN = page 3 / 4; in keyboard mode
   voice 5 / pattern 5 = the drum layer 1 / 2 that the pads written go to. With Hold down: voice 5 / pattern 5 = that layer out of every step (keyboard mode), the
   yellow VOICE = every note out (keyboard mode) / every chord step out (chord mode), the yellow PATTERN = every drum (both layers) out (keyboard mode). Any
   press while Hold is down is a combination: Hold then does not clear the step when it is let go. */
static void sq_chords_empty(sq_t *Q) {   /* the chord steps are gone: the memory is a Hold list again */
    if (Q->cm & 2u) sq_chord_off(Q);
    sq_ff(&XQ->cs[0][0], 2u * SQ_CSTEPS); Q->cm = 0; Q->n = 0; Q->pos = 0;
}
static u32 sq_editing(const sq_t *Q) { return LED(0x15) ? Q->se : (Q->rec && (Q->cm & 1u)); }
static u32 sq_pg_data(u32 kbm, u32 pp) {   /* the page has something on it (notes / drums, or chords) */
    for (u32 j = 0; j < 8u; j++) { u32 t = (pp << 3) | j; if (kbm ? (XQ->stn[t & 31u] != 0xFFu || XQ->std[t & 31u] != 0xFFu) : (XQ->cs[t & 63u][0] != 0xFFu)) return 1u; }
    return 0u;
}
static u32 sq_edit_btn(sq_t *Q, u32 row, u32 bits) {
    u32 kbm = LED(0x15) != 0u;
    for (u32 i = 0; i < 12u; i++) {
        u32 k = SQ_SB[i]; if (k / 9u != row) continue;
        u32 b = colbit(k), m = 1u << i, dn = (bits & b) != 0u;
        if (dn && !(Q->sbd & m)) {                                   /* a fresh press */
            u32 ed = sq_editing(Q);
            if (!ed && !kbm && Q->rec && !Q->n && !Q->cv && (i < 8u || i == 9u || i == 11u)) {   /* recording chords, none yet: a step button = the chord STEP sequencer */
                sq_ff(&XQ->cs[0][0], 2u * SQ_CSTEPS); Q->cm |= 1u; ed = 1u;
            }
            if (ed && kbm && Q->pcf) {                                  /* 2.133: the overwrite question: voice 1 (Omni 1) = yes, the yellow VOICE = no; the others do nothing */
                if (i == 0u) { pt_save(Q->pcf); Q->p3cur = Q->pcf; Q->pcf = 0; kb_blink_start(r27()); }
                else if (i == 9u) Q->pcf = 0;
                Q->sbw |= (unsigned short)m;
            } else if (ed && kbm && Q->bbm && ((i >= 4u && i <= 7u) || i == 10u || i == 11u)) {   /* 2.130: Bb minor + PATTERN 1-5: the drums saved in that slot; + the yellow PATTERN: in the slot loaded */
                u32 k = i == 11u ? Q->p3cur : i == 10u ? 5u : i - 3u;
                if (k) {
                    if (pt()->b[(k - 1u) * PT_SLOT + 40u]) Q->pcf = (u8)k;      /* 2.133 (owner): a used slot: asks first (voice 1 = overwrite, yellow VOICE = cancel) */
                    else { pt_save(k); Q->p3cur = (u8)k; kb_blink_start(r27()); }
                }
                Q->sbw |= (unsigned short)m;
            } else if (ed) {
                Q->sbw |= (unsigned short)m;
                u32 hd = Q->hseen, pg = Q->ss >> 3;
                if (hd) Q->hcb = 1u;
                if (i < 8u) { Q->ss = (u8)((Q->ss & (kbm ? 0x18u : 0x38u)) | i); if (kbm) Q->sprv = 1u; }   /* a step of the page; 2.129: heard */
                else if (i == 8u || i == 10u) {                      /* voice 5 / pattern 5: keyboard mode: the drum layer 1 / 2 */
                    u32 l = i == 8u ? 0u : 1u;
                    if (kbm) { if (hd) { for (u32 q = 0; q < 32u; q++) XQ->std[q] |= (u8)(15u << (l * 4u)); } else Q->sl = (u8)l; }
                    else if (!hd) Q->ss = (u8)((Q->ss & 0x1Fu) | (l << 5));      /* chord steps: voice 5 = pages 1-4, pattern 5 = pages 5-8 */
                } else if (i == 9u) {                                /* the yellow VOICE */
                    if (hd) { if (kbm) sq_ff(XQ->stn, 32u); else sq_chords_empty(Q); }
                    else Q->ss = (u8)((kbm ? 0u : (Q->ss & 0x20u)) | ((((kbm ? pg : (pg & 3u)) == 0u) ? 1u : 0u) << 3) | (Q->ss & 7u));   /* page 1 / 2 (of the bank) */
                } else {                                             /* the yellow PATTERN */
                    if (hd) { if (kbm) sq_ff(XQ->std, 32u); }
                    else Q->ss = (u8)((kbm ? 0u : (Q->ss & 0x20u)) | ((((kbm ? pg : (pg & 3u)) == 2u) ? 3u : 2u) << 3) | (Q->ss & 7u));   /* page 3 / 4 (of the bank) */
                }
            }
        }
        if (dn) Q->sbd |= (unsigned short)m; else { Q->sbd &= (unsigned short)~m; Q->sbw &= (unsigned short)~m; }
        if (Q->sbw & m) bits &= ~b;
    }
    return bits;
}
/* row_filter: the drum pads and note keys (rows 0-3): read for the step sequencer, its keys pressed for it, the arpeggio's; rows 4-5: the VOICE / PATTERN
   buttons, KEYBOARD and Hold */
static u32 sq_filter(u32 row, u32 bits) {
    if (!R8(KEYS_EN) || R8(ST) != PH_DONE || !cf(CF_EXTRAS)) return bits;
    sq_t *Q = sq();
    if (row <= 1u) {               /* the drum pads (keys 4-11: row 0 cols 4-8, row 1 cols 0-2): a fresh press is noted (written in edit mode) */
        for (u32 k = 4u; k <= 11u; k++) {
            if (k / 9u != row) continue;
            u32 m = 1u << (k - 4u), dn = (bits & colbit(k)) != 0u;
            if (dn && !(Q->dpd & m)) {
                Q->dnw |= (u8)m;
                if (LED(0x15) && Q->bmn) { Q->pal |= (u8)m; Q->dna |= (u8)m; Q->pfr |= (u8)m; }   /* 2.119: B minor + a pad (keyboard mode): its extra sound */
            }
            if (dn) Q->dpd |= (u8)m; else { Q->dpd &= (u8)~m; Q->pal &= (u8)~m; }
            if (Q->pal & m) bits &= ~colbit(k);             /* the stock does not play its own sound for it */
        }
    }
    if (row >= 1u && row <= 3u) {  /* the note keys (13-35) are rows 1-3: read what is physically down, and while an arpeggio runs the stock sees only its step's key */
        if (row == 1u) { Q->php = Q->ph; Q->ph = 0; Q->bbm = (bits & colbit(15u)) ? 1u : 0u; }
        if (row == 2u) Q->bmn = (bits & colbit(22u)) ? 1u : 0u;      /* 2.119: B minor (key 22; it still reaches the stock) */
        for (u32 c = 0; c < 9u; c++) {
            u32 k = row * 9u + c;
            if (k < 13u || !((NOTE_KEYS >> k) & 1ull)) continue;
            u32 dn = (bits & colbit(k)) != 0u, fresh = dn && !((Q->php >> (k - 12u)) & 1u);
            if (dn) Q->ph |= 1u << (k - 12u);
            if (fresh) Q->pnew |= 1u << (k - 12u);
            if (Q->arp || (Q->aen && Q->php && fresh)) bits &= ~colbit(k);   /* a key pressed while another is down belongs to the arpeggio from its first scan */
            if (Q->pvs && k == R8(SQ_KT + 9u)) continue;                  /* 2.123: the preview's key: pressed by us below */
            if (Q->trx) {                                                   /* 2.114: transposing: the key does not sound; a fresh one sets the transpose */
                bits &= ~colbit(k);
                if (fresh) {
                    int ref = -1, pi = -1;
                    for (u32 q = 0; q < 32u; q++) if (XQ->stn[q] < 20u) { ref = XQ->stn[q]; break; }       /* the first note of the sequence */
                    for (u32 i = 0; i < 20u; i++) if (R8(SQ_KT + i) == k) { pi = (int)i; break; }
                    if (ref >= 0 && pi >= 0) Q->tro = (u8)(signed char)(pi - ref);
                }
            }
        }
        if (Q->arp && Q->ak < 54u && Q->ak / 9u == row) bits |= colbit(Q->ak);
        if (Q->snk < 54u && Q->snk / 9u == row) bits |= colbit(Q->snk);     /* the step sequencer's note */
        if (Q->ekk < 54u && Q->ekk / 9u == row) bits |= colbit(Q->ekk);     /* 2.129: a picked step's note, heard */
        if (Q->pvs) { u32 pk = R8(SQ_KT + 9u); if (pk < 54u && pk / 9u == row) bits |= colbit(pk); }   /* 2.123: the volume preview's key (A; sq_aoff makes it A 440) */
    }
    if (row == 4u) {               /* keyboard mode: Bb minor + a PATTERN button picks the arpeggio (the button is not the stock's then) */
        u32 kbe0 = LED(0x15) != 0u, pp = 0;
        {   /* 2.132 (owner): chord mode, KEYBOARD + yellow VOICE = the scale strum off (neither button reaches the stock) */
            u32 b = colbit(0x27u), dn = (bits & b) != 0u;
            if (dn && !(Q->scy & 2u) && !kbe0 && Q->kst && key_down(0x31u)) { Q->scy |= 1u; Q->scl = 0; Q->kst = 2u; Q->krel = 0; }
            if (dn) Q->scy |= 2u; else Q->scy &= 4u;
            if (Q->scy & 1u) bits &= ~b;
        }
        bits = sq_edit_btn(Q, 4u, bits);
        bits = sq_voicesel(Q, 4u, bits);
        for (u32 i = 0; i < 5u; i++) {
            u32 b = colbit(0x28u + i), dn = (bits & b) != 0u;
            if (dn) pp |= 1u << i;
            if (dn && !(Q->pp & (1u << i)) && kbe0 && Q->bbm && !Q->se) {   /* a fresh press with Bb minor down */
                Q->pk |= (u8)(1u << i);
                Q->am = (u8)(i + 1u);                                     /* rock 1 up, rock 2 down, slow rock up-down, country octaves (up over 2), swing random */
                kb_blink_start(r27());
            } else if (dn && !(Q->pp & (1u << i)) && Q->p3a && !Q->se && !Q->bbm) {   /* 2.130: the PATTERN bank 3: a saved drum pattern into the sequencer */
                Q->pk |= (u8)(1u << i); Q->p3cur = (u8)(i + 1u); pt_loadslot(i + 1u);
            }
            if (!dn) Q->pk &= (u8)~(1u << i);
            if (Q->pk & (1u << i)) bits &= ~b;
        }
        Q->pp = (u8)((Q->pp & 0x20u) | pp);
        return bits;
    }
    if (row <= 3u) return bits;
    if (row != 5u) return bits;
    bits = sq_edit_btn(Q, 5u, bits);
    bits = sq_voicesel(Q, 5u, bits);
    {   /* The Start / Stop button (key 0x2E; its LEDs: 0x13 = running, 0x14 = stopped / sync start). Chord mode (2.111, 2.112; 2.113: Start alone too, when something is
           recorded): the player's own start: the step sequencers play with that rhythm (KEYBOARD + Start still works; KEYBOARD is then no mode change). With nothing
           recorded Start is the stock's; Sync Start (a chord press) is always the stock's, the sequencers silent - EXCEPT while chords are being recorded: then Start alone does nothing when the rhythm is not running, a chord
           press does not start it (the pending start is cancelled) and a rhythm that runs anyway is muted. Start while the player's rhythm runs is the stop gesture. */
        u32 b = colbit(0x2Eu), dn = (bits & b) != 0u, kbm = LED(0x15) != 0u;
        if (dn && !Q->skd) {
            Q->sws = 0;
            if (!kbm) {
                if (bits & colbit(0x31u)) { Q->go = 1u; Q->ust = 250u; if (Q->kst < 2u) { Q->kst = 2u; Q->krel = 0; } }   /* KEYBOARD + Start: the player starts it */
                else if (Q->go && R8(SQ_RUN)) Q->go = 0;                                                                /* the stop gesture */
                else if (Q->rec && !R8(SQ_RUN)) { Q->go = 1u; Q->ust = 250u; }                                         /* 2.124 (owner): recording chords, Start alone = KEYBOARD + Start: the beat with the sequences */
                else if (!R8(SQ_RUN) && sq_has(Q)) { Q->go = 1u; Q->ust = 250u; }                                      /* 2.113: Start alone starts what is recorded */
            }
        }
        Q->skd = (u8)dn; if (!dn) Q->sws = 0;
        if (Q->sws) bits &= ~b;
    }
    {   u32 b = colbit(0x2Du), dn = (bits & b) != 0u;           /* the yellow PATTERN button + Bb minor = the arpeggio off (the default) */
        if (dn && !(Q->pp & 0x20u) && !LED(0x15) && Q->kst && (bits & colbit(0x31u))) { Q->pk |= 0x20u; Q->scl = 1u; Q->kst = 2u; Q->krel = 0; }   /* 2.132 (owner): chord mode, KEYBOARD + yellow PATTERN = the scale strum on */
        else if (dn && !(Q->pp & 0x20u) && LED(0x15) && Q->bbm && !Q->se) { Q->pk |= 0x20u; Q->am = 0; kb_blink_start(r27()); }
        else if (dn && !(Q->pp & 0x20u) && cf(CF_EXTRAS) && !Q->se && !Q->p3inj && !Q->bbm) { Q->yph = 1u; Q->ypu = 0; Q->ypt0 = (unsigned short)TICKS; }   /* 2.130: ours until let go */
        if (dn && Q->yph && !Q->ypu && !Q->p3a && ((TICKS - Q->ypt0) & 0xFFFFu) >= SQ_LONG) { Q->p3a = 1u; Q->ypu = 1u; if (Q->p3cur) pt_loadslot(Q->p3cur); }   /* held: the PATTERN bank 3 */
        if (!dn && Q->yph) {
            Q->yph = 0;
            if (!Q->ypu) { if (Q->p3a) { Q->p3a = 0; sq_ff(XQ->std, 32u); } else Q->p3inj = SQ_INJ; }   /* a tap: out of bank 3 (its drums out of the sequencer: the stock pattern again); else the stock's press (the pattern bank) */
        }
        if (dn) Q->pp |= 0x20u; else { Q->pp &= (u8)~0x20u; Q->pk &= (u8)~0x20u; }
        if ((Q->pk & 0x20u) || Q->yph) bits &= ~b;
        if (Q->p3inj) { Q->p3inj--; bits |= b; }
    }
    u32 bk = colbit(0x31u), bh = colbit(0x30u), now = TICKS & 0xFFFFu;
    u32 kb = LED(0x15) != 0u;
    /* KEYBOARD */
    if (!Q->kinit) {                                       /* a KEYBOARD held from power-on (Jazz) is no press of ours */
        if (bits & bk) Q->kup = 0; else if (++Q->kup >= 20u) Q->kinit = 1u;
    } else if (Q->kst == 0u && (bits & bk) && !Q->injk && R8(KEY(0x31u) + 3u) && !key_down(0x2Fu) && !sq_pat_down()) {
        Q->kst = 1u; Q->k0 = (unsigned short)now; Q->krel = 0;
    }
    if (Q->kst) {
        if (bits & bk) {
            Q->krel = 0;
            if (Q->kst == 1u && ((now - Q->k0) & 0xFFFFu) >= SQ_LONG) {
                Q->kst = 2u;
                if (kb) { Q->se ^= 1u; Q->ss &= 31u; }     /* keyboard mode: a long press = the step sequencer's edit mode on / off (4 pages there) */
                else sq_toggle(Q);                         /* chord mode: the chord recording */
            }
        } else if (++Q->krel >= (Q->kst == 1u ? 3u : 20u)) {
            if (Q->kst == 1u) { if (Q->se) Q->se = 0; else Q->injk = SQ_INJ; }   /* a short press: in edit mode it ends it, else the stock gets it now */
            Q->kst = 0;
        }
        bits &= ~bk;
    }
    if (Q->injk) { Q->injk--; bits |= bk; }
    /* Hold */
    if (bits & bh) {
        Q->hoff = 0;
        if (!Q->hseen) {
            Q->hseen = 1u;
            if (sq_editing(Q)) { Q->hdn = 1u; Q->hcb = 0; }                           /* edit mode: it clears the selected step when it is let go (unless it was a combination) */
            else if (Q->rec || (kb && Q->n)) { Q->hdn = 1u; Q->hreq = 1u; }
        }
    } else if (Q->hseen && ++Q->hoff >= 20u) {
        Q->hseen = 0; Q->hdn = 0;
        if (!Q->hcb) {
            if (kb && Q->se) sq_hold_step(Q->ss & 31u);           /* 2.129: the newest thing written there (note, drum layer 1 / 2), one per press */
            else if (!kb && Q->rec && (Q->cm & 1u)) {                                 /* a chord step */
                XQ->cs[Q->ss & 63u][0] = 0xFFu; XQ->cs[Q->ss & 63u][1] = 0xFFu;
                u32 left = 0; for (u32 q = 0; q < SQ_CSTEPS; q++) left += XQ->cs[q][0] != 0xFFu;
                if (!left) sq_chords_empty(Q);
            }
        }
    }
    if (Q->hdn) bits &= ~bh;
    return bits;
}
/* the recording LED: the keyboard LED's port bit only (as kb_blink_tick), the flag is never touched */
static void sq_port(u32 on) {
    u8 v = R8(LEDSH + 2u), nv = on ? (u8)(v | KB_LEDBIT) : (u8)(v & ~KB_LEDBIT);
    if (nv != v) { R8(LEDSH + 2u) = nv; F_F518(6u, (u32)(u8)~nv); }
}
static void sq_portbit(u32 bit, u32 on) {                 /* any bit of LED port 6 (flag 0x11 = bit 0 .. 0x15 = bit 4, the keyboard LED); the flags are never touched */
    u8 v = R8(LEDSH + 2u), nv = on ? (u8)(v | bit) : (u8)(v & ~bit);
    if (nv != v) { R8(LEDSH + 2u) = nv; F_F518(6u, (u32)(u8)~nv); }
}
/* the next arpeggio step: the position in the held notes (lowest first; the 'octaves' mode runs them again 12 higher), by the mode */
static u32 sq_pick(sq_t *Q, const u8 *L, u32 n, u32 *off) {
    u32 m = Q->am, p = Q->ap, N = (m == 4u) ? 2u * n : n;
    if (p != 0xFFu && p >= N) p = N - 1u;
    if (m == 1u || m == 4u) p = (p == 0xFFu) ? 0u : (p + 1u) % N;
    else if (m == 2u) p = (p == 0xFFu || p == 0u) ? N - 1u : p - 1u;
    else if (m == 3u) {
        if (p == 0xFFu) { p = 0u; Q->ad = 1u; }
        else if (Q->ad) { if (p + 1u >= N) { Q->ad = 0; p -= 1u; } else p++; }
        else { if (p == 0u) { Q->ad = 1u; p = 1u; } else p--; }
    } else {
        if (!Q->rng) Q->rng = 0x2545F491u ^ TICKS;
        Q->rng ^= Q->rng << 13; Q->rng ^= Q->rng >> 17; Q->rng ^= Q->rng << 5;
        u32 q = Q->rng % N; if (q == p && N > 1u) q = (q + 1u) % N;
        p = q;
    }
    Q->ap = (u8)p; *off = (m == 4u && p >= n) ? 12u : 0u;
    return L[p % n];
}
static void sq_tick(void) {        /* control task, each tick (c274_wrap) */
    if (!R8(KEYS_EN) || R8(ST) != PH_DONE) return;
    sq_t *Q = sq();
    u32 now = TICKS & 0xFFFFu, kb = LED(0x15) != 0u;
    Q->rpre = (u8)(R8(SQ_RUN) != 0u);                      /* 2.113: for sq_post, right after the stock button handler */
    if ((now & 31u) == 0u) { kv_apply(Q->kvc); Q->kvc = (u8)(Q->kvc + 1u >= 10u ? 0u : Q->kvc + 1u); }   /* 2.123: one voice every 32 ms: all ten in 0.32 s */
    if (Q->p3boot && cf(CF_EXTRAS) && R8(KEYS_EN)) { Q->p3boot = 0; if (Q->p3a && Q->p3cur) pt_loadslot(Q->p3cur); }   /* 2.130: the saved pattern back in the sequencer */
    dv()->p3s = (u8)((Q->p3a ? 0x80u : 0u) | Q->p3cur);
    if (!cf(CF_EXTRAS)) Q->p3a = 0;
    if (B3_OK && R8(KEYS_EN)) {   /* 2.127: the third voice bank */
        dv_t *D = dv();
        /* 2.128: entering picks the banjo the stock's own way (2.127 called the voice loader 0x6000E7DC alone: it loads layers but sets neither the voice DE89
           nor its lights): a press of the yellow VOICE if bank 1 is shown, then of VOICE 5; then the slot is the third bank's */
        if (Q->b3req == 1u) { D->b3 = Q->b3l ? Q->b3l : 1u; Q->b3st = 1u; Q->b3t = (unsigned short)now; }
        else if (Q->b3req == 2u) { D->b3 = 0; Q->b3st = 0; }
        Q->b3req = 0;
        if (Q->b3st) {
            u32 el = (now - Q->b3t) & 0xFFFFu;
            if (el > 1500u) { Q->b3st = 0; D->b3 = 0; }                                       /* the stock did not follow: give up */
            else if (Q->b3st == 1u) { if (R8(RA(2000DE89)) == 9u) Q->b3st = 0; else if (R8(RA(2000DE88)) != 1u) { if (!Q->yinj) Q->yinj = SQ_INJ; Q->b3st = 2u; } else Q->b3st = 3u; }
            else if (Q->b3st == 2u) { if (!Q->yinj && R8(RA(2000DE88)) == 1u) Q->b3st = 3u; }
            else if (Q->b3st == 3u) { if (!Q->yinj) { Q->vinj = SQ_INJ; Q->b3st = 4u; } }
            else if (Q->b3st == 4u) { if (!Q->vinj && R8(RA(2000DE89)) == 9u) Q->b3st = 0; }
        }
        if (!Q->b3st) {
            if (Q->b3a && R8(RA(2000DE89)) != 9u) D->b3 = 0;              /* another voice was picked (MIDI, the stock): out of the third bank */
            u32 want = (D->b3 <= 5u && R8(RA(2000DE89)) == 9u) ? D->b3 : 0u;
            if (want != Q->b3a) b3_apply(Q, want);
        }
    }
    sq_force(Q, cf(CF_EXTRAS) && Q->rec && !kb);           /* 2.113: recording in chord mode: Manual, Hold off; put back after */
    /* 2.122 (owner): the Normal (108) tone's rhythm bass is cut like the Classic (84) tone's: its gate shut BASS_CUT ms after each note opens it (r27_bass notes
       it), the analog bass then dies away on its own. The Classic tick does this at 50 ms (DE34 = 0xFA); the Normal tick never does, so the bass held for the
       pattern's whole note and sounded far louder than the chords. Only with the stock bass (no pluck style), not in 27 mode. Shut as the stock does (0x6000759A). */
    if (Q->bsn && ((now - Q->bst0) & 0xFFFFu) >= BASS_CUT) {
        Q->bsn = 0;
        if (R8(SQ_RUN) && R8(SQ_TONE) == 0u && R8(RA(2000DE38))) { R32(SQ_GPIO2_CLR) = 0x10u; R8(RA(2000DE38)) = 0; }
    }
    if (!cf(CF_EXTRAS)) { Q->rec = 0; Q->want = 0; Q->arp = 0; Q->ak = 0xFFu; Q->se = 0; Q->sact = 0; Q->snk = 0xFFu; Q->dmute = 0; if (Q->bl) { sq_portbit(Q->bl, LED(Q->bl == 4u ? 0x13 : 0x14) != 0u); Q->bl = 0; Q->bon = 0; } Q->mute = 0; Q->go = 0; Q->ust = 0; return; }
    if (!kb) Q->se = 0;                                    /* the step sequencer's edit mode is for keyboard mode */
    if (Q->pos >= Q->n) Q->pos = 0;
    /* 2.119: a drum pad held in keyboard mode (the first one): the knob sets that pad's drum volume (its extra sound's, if it was pressed with B minor) */
    u32 dvi = 0xFFu;
    if (kb && Q->dpd && SQ_PADT != 0u && R8(SQ_TONE) == 0u) {   /* 2.120: only in the Normal (108) tone */ u32 p = 0; while (!((Q->dpd >> p) & 1u)) p++; dvi = ((Q->pal >> p) & 1u) ? SQ_ALT[p] : R8(SQ_PADT + (R8(SQ_TONE) ? 8u : 0u) + p); if (dvi >= DV_N) dvi = 0xFFu; }
    if (Q->pfr) { if (kb) for (u32 p = 0; p < 8u; p++) if ((Q->pfr >> p) & 1u) F_HIT(SQ_ALT[p], 0); Q->pfr = 0; }   /* 2.119: the extra sounds of the pads just pressed */
    {   u32 kv = R8(SQ_SUSV + 2u), swm = kb && Q->bbm, gbd = kb && R8(KEY(0x32u) + 4u) != 0u, rvh = kb && R8(KEY(12u) + 4u) != 0u, kfh = kb && cf(CF_MOD) && cf(CF_EXTRAS) && (R8(KEY(22u) + 4u) || R8(KEY(19u) + 4u)),
             job = dvi != 0xFFu ? 2u : (kb && Q->yh) ? 3u : swm ? 1u : gbd ? 4u : rvh ? 6u : kfh ? 5u : 0u;   /* 2.141: Gb MINOR = the delay (was Gb 7th), Db minor = the reverb, B / D minor = the vibrato / tremolo's depth (mod2.c ui_tick) */   /* the Sustain knob: note length; with Bb minor held, swing; a pad held: its volume; 2.123: yellow VOICE held: the voice's keyboard volume */
        if (!(Q->nst & 1u)) { Q->nst = (u8)(3u | (job << 2)); Q->nref = (u8)kv; }                 /* first tick: all are their defaults until the knob is moved */
        else if (((Q->nst >> 2) & 7u) != job || (job == 2u && Q->dvi != dvi)) { Q->nst = (u8)(3u | (job << 2)); Q->nref = (u8)kv; }   /* the knob's job changed: locked */
        Q->dvi = (u8)dvi;
        if ((Q->nst & 2u) && (kv > Q->nref ? kv - Q->nref : Q->nref - kv) > KN_TH) Q->nst &= (u8)~2u;   /* moved: it takes over */
        if (!(Q->nst & 2u)) {
            if (job == 2u) { dv_t *D = dv(); if (D->v[dvi] != kv) { D->v[dvi] = (u8)kv; Q->dvp = 1u; } }
            else if (job == 3u) {
                dv_t *D = dv(); u32 vi = R8(RA(2000DE89)); volatile u8 *kp = (vi == 9u && Q->b3a) ? &D->kv3[Q->b3a - 1u] : vi < 10u ? &D->kv[vi] : 0;   /* 2.127 */
                if (kp && *kp != kv) {
                    *kp = (u8)kv; kv_apply(vi); Q->yus = 1u; Q->pvt = (unsigned short)now;
                    if (!Q->pvs) { Q->pvo = (s8)(69 - (0x30 + 9 + 12 * (int)R8(RA(200009DC))) - r27()->ktr); Q->pvs = 1u; Q->pvk = 1u; }   /* A 440 = note 69 */
                }
            }
            else if (job == 4u) Q->dtm = (u8)kv;                                            /* 2.137 / 2.141: Gb minor held + Sustain = the delay time */
            else if (job == 6u) Q->rvm = (u8)kv;                                            /* 2.141: Db minor held + Sustain = the reverb amount */
            else if (job == 5u) { }                                                         /* 2.141: B / D minor: the depth (ui_tick); the note length stays */
            else if (swm) Q->cut = (u8)kv; else Q->gt = (u8)kv;                               /* 2.135: Bb minor + Sustain = the filter's cutoff (the swing is no longer set here) */
        }
        if (job == 2u && Q->dvp && ((now - Q->dvt) & 0xFFFFu) >= 90u) { Q->dvp = 0; Q->dvt = (unsigned short)now; F_HIT(dvi, 0); }   /* the preview: at most every 90 ms */
        if (job != 2u) Q->dvp = 0;
        Q->dvk = (u8)(job == 2u);
    }
    {   /* 2.135: Bb minor + the Tempo knob (sensor 5) = the filter's resonance; the stock tempo frozen while Bb minor is held and after it, until the knob is moved again */
        u32 tv = R8(SQ_SUSV + 5u), jb = (cf(CF_EXTRAS) && kb && Q->bbm) ? 1u : 0u;
        u32 jt = jb ? 1u : (cf(CF_EXTRAS) && kb && R8(KEY(0x32u) + 4u)) ? 2u : (cf(CF_EXTRAS) && kb && R8(KEY(12u) + 4u)) ? 4u
               : (cf(CF_EXTRAS) && cf(CF_MOD) && kb && (R8(KEY(22u) + 4u) || R8(KEY(19u) + 4u))) ? 3u : 0u;     /* 2.141: 2 Gb minor = the delay's repeats, 4 Db minor = the reverb's decay, 3 B / D minor = the speed */
        if (!(Q->tst & 1u)) { Q->tst = (u8)(1u | (jt << 2) | (jt ? 2u : 0u)); Q->tref = (u8)tv; }
        else if (((Q->tst >> 2) & 7u) != jt) { Q->tst = (u8)(1u | (jt << 2) | 2u); Q->tref = (u8)tv; }    /* Bb minor / Gb 7th pressed / let go: locked */
        if ((Q->tst & 2u) && (tv > Q->tref ? tv - Q->tref : Q->tref - tv) > KN_TH) Q->tst &= (u8)~2u;     /* moved: it takes over */
        if (jt == 1u && !(Q->tst & 2u)) Q->res = (u8)tv;
        else if (jt == 2u && !(Q->tst & 2u)) Q->dfb = (u8)tv;
        else if (jt == 4u && !(Q->tst & 2u)) Q->rvd = (u8)tv;
        Q->tfz = (u8)(cf(CF_EXTRAS) && (jt || (Q->tst & 2u)));
        flt_t *F = FLTB;
        u32 on = cf(CF_EXTRAS) && kb && (Q->cut || Q->res);
        F->f = FTAB[Q->cut & 127u]; F->q = 23170u - (u32)(Q->res & 127u) * 172u;
        F->on = on;
        dly_t *L = DLYB; u32 don = cf(CF_EXTRAS) && kb && (Q->dtm || Q->dfb);
        if (L->magic != DLYMAGIC) { L->on = 0; L->wr = 0; L->d = 0; L->dt = 0; L->fb = 0; L->clr = DLYN; L->magic = DLYMAGIC; }   /* first time since power-on (RAM is not zero) */
        L->dt = 2400u + (u32)(Q->dtm & 127u) * 146u; L->fb = (u32)(Q->dfb & 127u) * 7u / 8u;
        if (don && L->on != 1u) { L->d = L->dt; L->wr = 0; L->clr = 0; }   /* on: the buffer is cleared first (the mixer) */
        L->on = don;
        rvb_t *RV = RVBB; u32 ron = cf(CF_EXTRAS) && kb && Q->rvm != 0u;                                          /* 2.141: the reverb (amount 0 = off) */
        if (RV->magic != RVBMAGIC) { RV->on = 0; RV->clr = RVN; RV->amt = 0; RV->fb = 0; RV->prev = 0; for (u32 j = 0; j < 6u; j++) RV->ix[j] = 0; for (u32 j = 0; j < 4u; j++) RV->lp[j] = 0; RV->magic = RVBMAGIC; }   /* (RAM at power-on is not zero) */
        RV->amt = (u32)(Q->rvm & 127u); RV->fb = 22000u + (u32)(Q->rvd & 127u) * 70u;
        if (ron && RV->on != 1u) { RV->clr = 0; RV->prev = 0; for (u32 j = 0; j < 6u; j++) RV->ix[j] = 0; for (u32 j = 0; j < 4u; j++) RV->lp[j] = 0; }   /* on: the buffer is cleared first (the mixer) */
        RV->on = ron;
    }
    if (Q->pvs && (!(kb && Q->yh) || ((now - Q->pvt) & 0xFFFFu) >= 400u)) { Q->pvs = 0; Q->pvtl = (unsigned short)now; }   /* 2.123: the preview stops 0.4 s after the last change */
    if (!Q->pvs && Q->pvk && ((now - Q->pvtl) & 0xFFFFu) >= 80u) Q->pvk = 0;                                       /* ... its note-off has gone with its mark */
    { dv_t *D = dv(); D->ar[0] = Q->am; D->ar[1] = Q->dv; D->ar[2] = Q->gt; D->ar[3] = Q->swv; D->rs = (u8)(127u - (Q->res & 127u)); }   /* 2.142: + the resonance */                 /* 2.123: the arpeggio settings, saved at power-off */
    /* the arpeggio: Omni 1 only, keyboard mode, two or more note keys down, a mode picked (Bb minor + PATTERN): the notes are played one at a time */
    Q->aen = (u8)(kb && R8(DE89) == 0u && Q->am && !Q->se && !Q->trx);   /* 2.114: not while the note sequence is transposed by the keys */
    u32 phe = Q->ph;                                       /* 2.144: the keys the arpeggio plays: the held ones, or with the latch on the last set (new keys after all were let go start a new one) */
    if (Q->lat && kb && cf(CF_EXTRAS)) {
        if (Q->ph) { if (Q->lfz) { Q->lmk = 0; Q->lfz = 0; } Q->lmk |= Q->ph; } else Q->lfz = 1u;
        phe = Q->lmk;
    } else { Q->lmk = 0; Q->lfz = 1u; if (!kb) Q->lat = 0; }
    u32 awant = Q->aen && sq_pop(phe) >= 2u;
    if (awant && !Q->arp) {
        Q->arp = 1u; Q->ak = 0xFFu; Q->ap = 0xFFu; Q->ad = 1u; Q->pend = 0; Q->sph = 0; Q->kcl = 0;
        Q->abeat = R8(SQ_RUN) != 0u; Q->lst = *(volatile unsigned short *)SQ_STEP; Q->at0 = (unsigned short)(now - 1000u); Q->gap = 0;   /* free-running: the first step now; with the beat: the next grid point */
    }
    if (!awant && Q->arp) { Q->arp = 0; Q->ak = 0xFFu; Q->pend = 0; Q->kcl = 1u; Q->kt0 = (unsigned short)now; }
    if (Q->kcl && ((now - Q->kt0) & 0xFFFFu) >= 80u) { Q->koff = 0; Q->kcl = 0; }          /* the last notes' note-offs are done: no octave marks left */
    if (Q->arp) {
        /* the step: the note value (half stock steps) x the tempo knob's step time; with the rhythm running the steps are the rhythm's own grid.
           Swing: pairs of steps, the first gets 2/3 of the pair, the second 1/3. */
        u32 stp = R32(SQ_STEPT); if (stp < 60u || stp > 500u) stp = 300u;
        u32 dv = Q->dv < 5u ? Q->dv : 1u, len = SQ_DIV[dv], swg = SQ_SWG[dv];
        u32 sp = SW_MIN + (Q->swv * (SW_MAX - SW_MIN)) / 127u;                  /* swing: the first note's % of the pair */
        u32 per = stp * len / 10u, gap = swg ? (Q->sph ? per * 2u * (100u - sp) / 100u : per * 2u * sp / 100u) : per;   /* ms to the next step */
        if (gap < ARP_MIN) gap = ARP_MIN;
        u32 gf = Q->gt >= GT_FULL ? 100u : GT_MIN + (Q->gt * (100u - GT_MIN)) / 127u;   /* % of the step its key is down: staccato .. legato */
        u32 on = gf >= 100u ? 0xFFFFu : gap * gf / 100u;                         /* legato: never let go before the next step's key */
        if (on < 12u) on = 12u;
        u32 beat = R8(SQ_RUN) != 0u, st = *(volatile unsigned short *)SQ_STEP, trig = 0u, el = (now - Q->at0) & 0xFFFFu;
        if (beat != Q->abeat) { Q->abeat = (u8)beat; Q->lst = (unsigned short)st; Q->at0 = (unsigned short)now; Q->pend = 0; el = 0; }
        if (beat) {
            if ((unsigned short)st != Q->lst) {                  /* a step edge within half a step of a grid point (the pair's first, or second, note) */
                Q->lst = (unsigned short)st;
                u32 pl = swg ? 2u * len : len, p = (st * 2u) % pl, sw = (len * 2u * sp + 50u) / 100u;   /* a swing pair is 2 steps long, a straight step 1; the second note at sp % */
                if (p <= 1u || (swg && p >= sw && p - sw <= 1u)) Q->pend = 1u;
            }
            trig = Q->pend && el >= ARP_MIN;
        } else trig = el >= (u32)Q->gap || Q->gap == 0u;
        if (trig) {
            Q->pend = 0; Q->at0 = (unsigned short)now; Q->gap = (unsigned short)gap; if (swg) Q->sph ^= 1u; else Q->sph = 0;
            u8 L[20]; u32 n = 0, off = 0;
            for (u32 i = 0; i < 20u; i++) { u32 k = R8(SQ_KT + i); if (k >= 13u && k <= 35u && ((phe >> (k - 12u)) & 1u)) L[n++] = (u8)k; }   /* lowest note first */
            if (n >= 2u) {
                u32 key = sq_pick(Q, L, n, &off);
                Q->ak = (u8)key;
                for (u32 i = 0; i < 20u; i++) if (R8(SQ_KT + i) == key) { if (off) Q->koff |= 1u << i; else Q->koff &= ~(1u << i); }   /* this key's octave, marked as it is pressed */
            }
        } else if (el >= on) Q->ak = 0xFFu;
    }
    /* ---- the step sequencer (2.105) ---- */
    if (Q->se) {                                           /* edit mode: a note key / drum pad played is written at the selected step (the step stays: no advance) */
        if (Q->pnew) for (u32 q = 0; q < 20u; q++) { u32 k = R8(SQ_KT + q); if (k >= 13u && k <= 35u && ((Q->pnew >> (k - 12u)) & 1u)) { XQ->stn[Q->ss & 31u] = (u8)q; XQ->sno[Q->ss & 31u] = (u8)R8(OCT); sq_sto_push(Q->ss & 31u, 1u); break; } }
        if (Q->dnw) for (u32 b = 0; b < 8u; b++) if ((Q->dnw >> b) & 1u) { u32 t = Q->ss & 31u, sh = (u32)(Q->sl & 1u) * 4u, xb = 1u << (Q->sl & 1u); XQ->std[t] = (u8)((XQ->std[t] & ~(15u << sh)) | (b << sh)); XQ->stx[t] = (u8)(((Q->dna >> b) & 1u) ? (XQ->stx[t] | xb) : (XQ->stx[t] & ~xb)); sq_sto_push(t, 2u + (Q->sl & 1u)); break; }   /* into the drum layer being edited; 2.119: the extra sound too */
    }
    if (Q->sprv) {                                         /* 2.129: a step picked in edit mode: what it holds is heard (its drums now, its note for 0.25 s) */
        Q->sprv = 0;
        if (kb && Q->se) {
            u32 t = Q->ss & 31u, nt = XQ->stn[t], d1 = XQ->std[t] & 15u, d2 = XQ->std[t] >> 4, x1 = XQ->stx[t] & 1u, x2 = (XQ->stx[t] >> 1) & 1u;
            if (d1 < 8u) { if (x1) F_HIT(SQ_ALT[d1], 0); else F_BE0C(4u + d1, (const volatile u8 *)SQ_TONE); }
            if (d2 < 8u && (d2 != d1 || x2 != x1)) { if (x2) F_HIT(SQ_ALT[d2], 0); else F_BE0C(4u + d2, (const volatile u8 *)SQ_TONE); }
            Q->ekk = nt < 20u ? (u8)R8(SQ_KT + nt) : 0xFFu; Q->ekt = (unsigned short)now;
            if (nt < 20u) sq_tmark(Q, nt, now, sq_octoff(t));                  /* 2.146: heard in its own octave */
        }
    }
    if (Q->ekk != 0xFFu && (((now - Q->ekt) & 0xFFFFu) >= 250u || !Q->se)) { Q->ekk = 0xFFu; if (!Q->on && Q->tl != 0xFFu) sq_tmark(Q, 0xFFu, now, 0); }   /* 2.146: the preview's octave mark let go (not while the sequencer plays) */
    Q->pnew = 0; Q->dnw = 0; Q->dna = 0;
    /* The two sequencers share one clock (2.108): the arpeggio's note value (quarter, eighth = the default, sixteenth, swing eighth, swing sixteenth: Bb minor +
       VOICE) and its swing (the Sustain knob with Bb minor), on the rhythm's step grid; they play with the rhythm (the stock's Start): the notes and drums (the
       drums in any mode, the notes in keyboard mode only: in chord mode the strum plate plays them), and the chords (any mode). A sequencer is as long as its last
       page with something in it. */
    u32 nn = 0, dd = 0, plen = 0, dplen = 0, cn = 0, cplen = 0;
    for (u32 q = 0; q < 32u; q++) {                        /* 2.129 (owner): the notes and the drums loop on their own lengths (a 1-page beat repeats under 4 pages of notes) */
        nn += XQ->stn[q] != 0xFFu; dd += XQ->std[q] != 0xFFu;
        if (XQ->stn[q] != 0xFFu) plen = (q >> 3) + 1u;
        if (XQ->std[q] != 0xFFu) dplen = (q >> 3) + 1u;
    }
    if (!plen) plen = dplen;                               /* no notes: the shown position is the drums' */
    for (u32 q = 0; q < SQ_CSTEPS; q++) if ((Q->cm & 1u) && XQ->cs[q][0] != 0xFFu) { cn++; cplen = (q >> 3) + 1u; }
    u32 slen = plen << 3, dlen = dplen << 3, clen = cplen << 3;
    Q->dmute = (u8)(dd && !IS27() && (kb || Q->go));        /* the stock pattern's drums are silent (r27_row) while drum steps are there and play */
    {   u32 stp = R32(SQ_STEPT); if (stp < 60u || stp > 500u) stp = 300u;
        /* The sequencers play with the rhythm in keyboard mode, and in chord mode only when the player started it (KEYBOARD + Start; or it was started in keyboard mode
           and he came back to chord mode): `go`. A rhythm started the stock way in chord mode is the stock's alone. While chord steps are being recorded a rhythm that runs
           without `go` (a chord press with Sync Start, the Start button's switch to Sync Start that does not stop it) is muted: its drums and bass rest (r27_row /
           r27_bass); a start still pending is cancelled right after the stock's button handler (sq_post). */
        u32 run0 = R8(SQ_RUN) != 0u, rst = !kb && Q->rec;      /* 2.113: any recording in chord mode (the chord steps or the Hold list) */
        if (Q->p3a && run0 && !Q->go) { Q->go = 1u; Q->ust = 0; }   /* 2.131: the PATTERN bank 3: any start is the player's (Sync Start too): the saved pattern plays */
        if (kb) { Q->go = (u8)run0; Q->ust = 0; }
        else if (Q->go && !run0) { if (Q->ust) Q->ust--; else Q->go = 0; }
        else if (run0) Q->ust = 0;
        Q->mute = (u8)(rst && !Q->go);                          /* 2.113: from the recording's start, so not even a first beat sounds */
        u32 run = run0 && !Q->mute && (kb || Q->go), ok = run && (nn || dd || cn);
        if (!kb) Q->snk = 0xFFu;
        Q->trx = (u8)(ok && kb && nn && !Q->tpoff && !Q->se && !IS27());   /* 2.114: the note keys transpose the note sequence */
        if (!Q->trx) Q->tro = 0;                                       /* stopped (or off): back to the sequence as recorded */
        if (Q->tpi != 0xFFu && ((now - Q->tpt) & 0xFFFFu) >= 80u) { Q->toff[Q->tpi] = 0; Q->tpi = 0xFFu; }
        if (!ok) {
            if (Q->sact) { Q->sact = 0; Q->sp = 0xFFu; Q->sdp = 0xFFu; Q->cp = 0xFFu; if (Q->cm & 2u) sq_chord_off(Q); }
            Q->snk = 0xFFu; Q->spend = 0; Q->cpend = 0;
            if (Q->tl != 0xFFu) sq_tmark(Q, 0xFFu, now, 0);
        } else {
            u32 dv = Q->dv < 5u ? Q->dv : 1u, len = SQ_DIV[dv], swg = SQ_SWG[dv];
            u32 spc = SW_MIN + (Q->swv * (SW_MAX - SW_MIN)) / 127u;               /* swing: the first note's % of the pair */
            u32 st = *(volatile unsigned short *)SQ_STEP;
            if (!Q->sact) { Q->sact = 1u; Q->sp = 0xFFu; Q->sdp = 0xFFu; Q->cp = 0xFFu; Q->ssph = 0; Q->slst = (unsigned short)st; Q->sg0 = (unsigned short)(now - 1000u);   /* the beat just started: step 1 with the stock's first step (2.126): */
                              if (st) { Q->spend = 1u; Q->cpend = 1u; Q->sfst = 0; } else { Q->spend = 0; Q->cpend = 0; Q->sfst = 1u; } }   /* already counted up = it has sounded: now; else at its count */
            else if ((unsigned short)st != Q->slst) {
                /* 2.126: the stock counts a step up right AFTER playing it: the count N+1 appears when step N sounds. The grid is checked on N (it was on
                   N+1: every step one stock step early, so step 1 -> 2 came short); the first change after the start is step 1's own beat (already played). */
                u32 sn = st ? st - 1u : 383u, first = Q->sfst;   /* 2.131: the stock counts 0-383 and wraps to 0: 0 = step 383 just played (2.126 took 65535: a grid point, an extra step every 384 counts) */
                Q->slst = (unsigned short)st; Q->sfst = 0;
                if (first) { Q->spend = 1u; Q->cpend = 1u; }                        /* the stock's first step just sounded: step 1 of both */
                else {
                    if (sn % 12u == 0u) Q->cpend = 1u;                              /* the chord steps: a quarter note, whatever the note value is */
                    u32 pl = swg ? 2u * len : len, p = (sn * 2u) % pl, sw = (len * 2u * spc + 50u) / 100u;   /* a step edge within half a step of a grid point (a swing pair's first or second note) */
                    if (p <= 1u || (swg && p >= sw && p - sw <= 1u)) Q->spend = 1u;
                }
            }
            u32 el = (now - Q->sg0) & 0xFFFFu;
            if (Q->spend && el >= ARP_MIN) {
                u32 per = stp * len / 10u, gp = swg ? (Q->ssph ? per * 2u * (100u - spc) / 100u : per * 2u * spc / 100u) : per;   /* ms to the next step */
                if (gp < ARP_MIN) gp = ARP_MIN;
                Q->spend = 0; Q->sg0 = (unsigned short)now; el = 0; Q->sgap = (unsigned short)gp;
                if (swg) Q->ssph ^= 1u; else Q->ssph = 0;
                if (nn || dd) {
                    Q->sp = (u8)((Q->sp == 0xFFu || Q->sp + 1u >= slen) ? 0u : Q->sp + 1u);
                    Q->sdp = (u8)((Q->sdp == 0xFFu || Q->sdp + 1u >= dlen) ? 0u : Q->sdp + 1u);   /* 2.129 */
                    u32 dq = dlen ? Q->sdp : Q->sp;
                    u32 nt = XQ->stn[Q->sp], d1 = XQ->std[dq] & 15u, d2 = XQ->std[dq] >> 4;
                    Q->snk = (kb && nt < 20u) ? (u8)R8(SQ_KT + nt) : 0xFFu;      /* a step with no note lets the last one go; none in chord mode */
                    if (Q->snk != 0xFFu) sq_tmark(Q, nt, now, sq_octoff(Q->sp));   /* 2.114: its transpose, as the key goes down; 2.146: + its octave */
                    u32 x1 = XQ->stx[dq] & 1u, x2 = (XQ->stx[dq] >> 1) & 1u;
                    if (d1 < 8u) { if (x1) F_HIT(SQ_ALT[d1], 0); else F_BE0C(4u + d1, (const volatile u8 *)SQ_TONE); }   /* the drums: the stock's own pad function, two layers = two drums at once; 2.119: an extra sound directly */
                    if (d2 < 8u && (d2 != d1 || x2 != x1)) { if (x2) F_HIT(SQ_ALT[d2], 0); else F_BE0C(4u + d2, (const volatile u8 *)SQ_TONE); }
                }
            }
            /* The chord steps: a quarter note each (fixed, no swing), 8 pages = 64 steps. */
            if (Q->cpend) {
                Q->cpend = 0;
                if (cn) {
                    Q->cp = (u8)((Q->cp == 0xFFu || Q->cp + 1u >= clen) ? 0u : Q->cp + 1u);
                    if (XQ->cs[Q->cp][0] != 0xFFu) { u8 x[4]; sq_expand(XQ->cs[Q->cp], x); sq_chord_off(Q); sq_chord_on(Q, x); Q->cm |= 2u; }   /* an empty step: the chord goes on */
                }
            }
            u32 gf = Q->gt >= GT_FULL ? 100u : GT_MIN + (Q->gt * (100u - GT_MIN)) / 127u;   /* the Sustain knob: staccato .. legato */
            u32 on = gf >= 100u ? 0xFFFFu : (u32)Q->sgap * gf / 100u; if (on < 12u) on = 12u;
            if (Q->snk != 0xFFu && el >= on) Q->snk = 0xFFu;
        }
    }
    /* The Start LED (flag 0x13 = the rhythm runs) or Sync Start LED (flag 0x14 = it is stopped), whichever the stock lights, is a metronome while the rhythm plays
       (a quarter note = 12 stock steps) or an arpeggio plays without it (the tempo knob's quarter): dark between the beats, lit for BL_MS on each one, all alike
       (2.110: it used to go dark on the beat). Port 6 bit only: the flags are the stock's state and are never touched; the bit goes back when it stops. */
    {   u32 beat = 0, stp = R32(SQ_STEPT); if (stp < 60u || stp > 500u) stp = 300u;
        u32 runb = R8(SQ_RUN) != 0u && !Q->mute;
        if (runb) { u32 st = *(volatile unsigned short *)SQ_STEP; if ((unsigned short)st != Q->bst) { Q->bst = (unsigned short)st; beat = (st % 12u) == 0u; } }
        else { Q->bst = 0xFFFFu; if (Q->arp) { u32 pq = stp * 24u / 10u; if (pq < 100u) pq = 100u; unsigned short qn = (unsigned short)((TICKS / pq) & 0xFFFFu); if (qn != Q->bq) { Q->bq = qn; beat = 1u; } } }
        u32 fl = LED(0x13) ? 0x04u : LED(0x14) ? 0x08u : 0u;
        if ((runb || Q->arp) && fl) {
            if (Q->bl && Q->bl != fl) sq_portbit(Q->bl, LED(Q->bl == 4u ? 0x13 : 0x14) != 0u);   /* the stock changed which one is lit: the old one is given back */
            Q->bl = (u8)fl;
            if (beat) { Q->bon = 1u; Q->bt0 = (unsigned short)now; }
            if (Q->bon && ((now - Q->bt0) & 0xFFFFu) >= BL_MS) Q->bon = 0;
            sq_portbit(fl, Q->bon);
        } else if (Q->bl) { sq_portbit(Q->bl, LED(Q->bl == 4u ? 0x13 : 0x14) != 0u); Q->bl = 0; Q->bon = 0; }
    }
    /* The page that is shown follows the playing step while a step sequencer is being edited, keyboard mode (notes and drums, 4 pages) and chord mode (chord steps, 8 pages):
       when the playing step moves to another page that has something on it, the page shown goes there (your own page choice stays until then). Nothing recorded: no
       playing, the page stays. */
    if (sq_editing(Q) && R8(SQ_RUN) && !Q->mute) {
        u32 ph = kb ? Q->sp : Q->cp;
        if (ph != 0xFFu) { u32 pp = ph >> 3; if (pp != Q->fpg) { Q->fpg = (u8)pp; if (sq_pg_data(kb, pp)) Q->ss = (u8)((Q->ss & 7u) | (pp << 3)); } }
    } else Q->fpg = 0xFFu;
    /* The LEDs under the stock's: with Bb minor held in keyboard mode, the PATTERN LED of the picked arpeggio mode (rock 1 up, rock 2 down, slow rock
       up-down, country octaves, swing random; the yellow PATTERN's lit while the arpeggio is off), the VOICE LED of its note value, the yellow VOICE's off. In the
       step sequencer's edit mode, the 8 steps of the page on the voice 1-4 and pattern 1-4 LEDs (lit = there is a note or drum there; the selected one blinks)
       and the 4 pages on the other four (voice 5, yellow VOICE, pattern 5, yellow PATTERN: the page you are on is lit). The LEDs as they were come back
       when it ends. Not in 27 mode (it draws the PATTERN LEDs itself). */
    {   static const u8 SQ_LED[14] = { 9, 10, 11, 12, 13, 2, 3, 4, 5, 6, 0x0E, 0x0F, 7, 8 };
        static const u8 SQ_SL[8] = { 5, 6, 7, 8, 0, 1, 2, 3 };           /* step 1-8 of a page -> SQ_LED index */
        if (Q->pcf && !(kb && Q->se)) Q->pcf = 0;                       /* 2.133: the question ends with the edit mode */
        u32 mode = IS27() ? 0u : (kb && Q->se && (Q->bbm || Q->pcf)) ? 4u : sq_editing(Q) ? 2u : (kb && Q->bbm) ? 1u : (Q->b3a || Q->p3a) ? 3u : 0u;   /* 2.127: 3 = the third voice bank; 2.130: or the PATTERN bank 3 */
        if (mode) {
            u8 w[14]; for (u32 i = 0; i < 14u; i++) w[i] = 0;
            if (mode == 3u) {      /* the third bank: both yellow VOICE lights and its voice's VOICE light; the PATTERN lights stay the stock's (and are kept fresh for the end) */
                if (!Q->lshow) { Q->lsm = 0; for (u32 i = 0; i < 14u; i++) if (LED(SQ_LED[i])) Q->lsm |= (unsigned short)(1u << i); }
                for (u32 i = 0; i < 14u; i++) {
                    u32 vgrp = (i >= 5u && i < 10u) || i >= 12u;   /* the VOICE lights; else the PATTERN lights */
                    if (vgrp && Q->b3a) w[i] = (u8)(i >= 12u || i - 5u == Q->b3a - 1u);
                    else if (!vgrp && Q->p3a) w[i] = (u8)(i >= 10u || i + 1u == Q->p3cur);
                    else {   /* the lights that are not ours: the stock's (kept fresh); 2.133: coming from our other displays (Bb minor menu, edit mode) they are still ours: the saved ones */
                        w[i] = (Q->lshow && Q->lshow != 3u) ? (u8)((Q->lsm >> i) & 1u) : (u8)(LED(SQ_LED[i]) != 0u);
                        if (w[i]) Q->lsm |= (unsigned short)(1u << i); else Q->lsm &= (unsigned short)~(1u << i);
                    }
                }
            } else if (mode == 4u) {   /* 2.133 (owner): saving a drum pattern: all dark but the slots in use; the question: voice 1 (Omni 1) and the yellow VOICE */
                if (Q->pcf) { w[5] = 1; w[12] = 1; w[13] = 1; }
                else for (u32 k = 0; k < 5u; k++) w[k] = (u8)(pt()->b[k * PT_SLOT + 40u] != 0u);
            } else if (mode == 1u) {
                for (u32 i = 0; i < 14u; i++) w[i] = (u8)(i < 5u ? (Q->am && i == Q->am - 1u) : i < 10u ? ((i - 5u) == Q->dv) : i == 10u ? (Q->am == 0u) : i >= 12u ? (Q->tpoff == 0u) : 0u);   /* 2.114: the yellow VOICE's two = transpose on */
            } else {
                u32 pg = Q->ss >> 3, sel = Q->ss & 7u;
                for (u32 j = 0; j < 8u; j++) { u32 t = (pg << 3) | j, has = kb ? (XQ->stn[t] != 0xFFu || XQ->std[t] != 0xFFu) : (XQ->cs[t][0] != 0xFFu); w[SQ_SL[j]] = (u8)(j == sel ? ((now / SQ_BLINK) & 1u) : has); }
                static const u8 SQ_PG[4] = { 12, 13, 10, 11 };                    /* page 1-4: the yellow VOICE's two LEDs, then the yellow PATTERN's (up, down) */
                w[SQ_PG[pg & 3u]] = 1;
                if (kb) w[Q->sl ? 4 : 9] = 1;                                      /* keyboard mode: the drum layer being edited: pattern 5 (layer 2) / voice 5 (layer 1) */
                else {
                    w[(pg >> 2) ? 4 : 9] = 1;                                      /* chord steps: the bank: voice 5 = pages 1-4, pattern 5 = pages 5-8 */
                }
                { u32 ph = kb ? Q->sp : Q->cp;                                       /* the step that is playing blinks fast, on the page that is shown */
                  if (ph != 0xFFu && R8(SQ_RUN) && !Q->mute && (ph >> 3) == pg) w[SQ_SL[ph & 7u]] = (u8)((now / 64u) & 1u); }
            }
            if (!Q->lshow) { Q->lsm = 0; for (u32 i = 0; i < 14u; i++) if (LED(SQ_LED[i])) Q->lsm |= (unsigned short)(1u << i); }
            Q->lshow = (u8)mode;
            for (u32 i = 0; i < 14u; i++) if ((LED(SQ_LED[i]) != 0u) != (w[i] != 0u)) F_LEDSET((int)SQ_LED[i], (int)w[i]);
        } else if (Q->lshow) {
            for (u32 i = 0; i < 14u; i++) { u32 on = (Q->lsm >> i) & 1u; if ((LED(SQ_LED[i]) != 0u) != on) F_LEDSET((int)SQ_LED[i], (int)on); }
            Q->lshow = 0;
        }
    }
    if (kb && Q->rec) { sq_commit(Q); Q->rec = 0; }        /* keyboard mode ends recording */
    if (!kb && !(Q->cm & 1u)) { Q->want = 0; Q->pos = 0; sq_chord_off(Q); }    /* chord mode: no playback; the sequence starts again from the first chord next time (2.96) */
    if (Q->hreq) {                                         /* Hold */
        Q->hreq = 0;
        if (Q->rec) { if (Q->cv || sq_any()) { Q->cv = 0; Q->ign = 1u; } else if (Q->n) Q->n--; kb_blink_start(r27()); }
        else if (Q->n && kb) Q->want = 1u;
    }
    if (Q->rec && !kb) {                                   /* record: the chord the stock decoded while a button is down */
        if (sq_any()) {
            if (!Q->ign) { for (u32 j = 0; j < 3u; j++) Q->cc[j] = (u8)(R8(SQ_CLS + j) % 12u); Q->ct = R8(SQ_TYPE); if (Q->ct > 8u) Q->ct = 0; Q->cx = (u8)sq_jz_get(); Q->cv = 1u; Q->ign = 0; }
            Q->c0 = (unsigned short)now;
        } else if (Q->ign) Q->ign = 0;
        else if (Q->cv && ((now - Q->c0) & 0xFFFFu) >= SQ_CLOSE) sq_commit(Q);
    }
    if (Q->rec && !(Q->cm & 1u)) sq_chord_off(Q);
    else if (Q->want && !(Q->cm & 1u)) {                                    /* play: the next chord, through the stock's chord start */
        sq_chord_off(Q);
        if (Q->n) { u8 e[4]; sq_expand(XQ->seq[Q->pos], e); sq_chord_on(Q, e); Q->pos = (u8)(Q->pos + 1u >= Q->n ? 0u : Q->pos + 1u); }
        Q->want = 0;
    }
    r27_t *S = r27();                                      /* the LED: slow blink while recording (not over the confirm blink) */
    if (Q->rec) { if (!S->kbl) { sq_port(((now / SQ_BLINK) & 1u) ? 0u : 1u); Q->led = 1u; } }
    else if (Q->led) { if (!S->kbl) { sq_port(LED(0x15) != 0u); Q->led = 0; } }
}
/* c274_wrap, right after the stock button handler. In keyboard mode that handler calls 0x6000819C(0) (bass off) for every press of a key with id 0-11:
   the arrows, T, P and all the drum pads (0x6000C996: the stock has no bass in keyboard mode). While a recorded chord sounds in Auto its flag is
   put back at once, so the rhythm's bass goes on (2.95, the owner's report: the bass vanished with the arrows and the drum buttons, not with the
   plate's drums, which do not go through that code). */
static void sq_post(void) {
    const sq_t *Q = (const sq_t *)SQB;
    if (Q->magic != SQMAGIC) return;
    if (Q->bass && !R8(SQ_BASSF)) F_819C(1);
    if (cf(CF_EXTRAS) && !LED(0x15) && Q->rec && !Q->go) {   /* recording chords: a start nobody asked for (a chord press with Sync Start) is cancelled */
        if (R8(SQ_RUN) && !Q->rpre) { F_8170(0); F_81CC(); }  /* 2.113: the press set the running flag itself: stopped as the Start button stops it */
        if (R8(SQ_STARTP) && !R8(SQ_RUN)) R8(SQ_STARTP) = 0;  /* ... and its pending start, before the rhythm takes it up */
    }
}
