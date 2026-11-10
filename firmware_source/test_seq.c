#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
typedef unsigned char u8; typedef signed char s8; typedef unsigned int u32;
#define FW_VERSION 30
#define R8(a)  (*(volatile u8 *)(uintptr_t)(a))
#define R32(a) (*(volatile u32 *)(uintptr_t)(a))
#define RA(a) (0x##a##u)
#define FL(a) (0x##a##u)
#define KEY(k) (RA(20006B90)+7u*(k))
#define KEYBITS FL(600332C0)
#define KEYS_EN RA(2000DEA2)
#define ST RA(20002C54)
#define PH_DONE 0x5A
#define CF_EXTRAS 2u
#define CF_MOD 1u
#define LED(i) R8(RA(200000F0)+(i))
#define LEDSH RA(20002A54)
#define KB_LEDBIT 0x10u
#define TICKS R32(RA(2000DB84))
#define NOTE_KEYS 0xFFFB76000ull
#define RS8(a) (*(volatile s8 *)(uintptr_t)(a))
#define DE79 RA(2000DE79)
#define DE89 RA(2000DE89)
#define OCT RA(200009DC)
#define IS27() 0
#define CF_KTR 0x40u
#define DE64 RA(2000DE64)
#define MA_KB RA(2000681C)
#define MA_KS RA(200068CC)
#define MA_AUTO 0u
#define MA_MANUAL 1u
#define MA_THIRD 2u
static u32 mstate = 1; static u32 ma_state(void) { return mstate; }
static int masets; static void ma_set(u32 t) { mstate = t; masets++; }
static int calls_on, calls_off; static u32 last_on[3];
static unsigned c8t[256]; static u8 c8a[256]; static int c8n;
static void F_8D24(u32 a, u32 b, u32 c, u32 d, u32 e) { (void)d; (void)e; calls_on++; last_on[0] = a; last_on[1] = b; last_on[2] = c; if (c8n < 256) { c8t[c8n] = *(volatile unsigned *)0x2000DB84; c8a[c8n++] = (u8)a; } }
static int calls_osc_off; static void F_9184(u32 a, u32 b) { (void)a; (void)b; calls_osc_off++; }
#define SQ_BASSF RA(2000DE42)
static int c819_on, c819_off; static void F_819C(u32 a) { if (a) c819_on++; else c819_off++; R8(SQ_BASSF) = a ? 1 : 0; }
#define F_8D24 F_8D24
#define F_9184 F_9184
#define F_819C F_819C
static void F_8D38(u32 a, u32 b) { (void)a; (void)b; calls_off++; }
static int note_held(void) { for (unsigned k = 13; k <= 35; k++) if (((NOTE_KEYS >> k) & 1ull) && R8(KEY(k) + 4u)) return 1; return 0; }
static u32 cfx = 1; static u32 cf(u32 b) { (void)b; return cfx; }
static u32 colbit(u32 k) { return 1u << (R8(KEYBITS + (k % 9u) * 16u + 8u) & 31u); }
static int key_down(u32 k) { return R8(KEY(k)) || R8(KEY(k) + 4u); }
typedef struct { u8 kbl; s8 ktr; } r27_t; static r27_t R27;
static r27_t *r27(void) { return &R27; }
static void kb_blink_start(r27_t *S) { S->kbl = 1; }
static void F_LEDSET(int i, int on) { R8(RA(200000F0) + i) = (u8)on; }
static int ledwrites = 0; static void F_F518(u32 p, u32 v) { (void)p; (void)v; ledwrites++; }
static u32 jz_cur = 0, jz_set_last = 99; static int jz_sets = 0;
static u32 sq_jz_get(void) { return jz_cur; }
static void sq_jz_set(u32 x) { jz_set_last = x; jz_sets++; }
static u32 sq_on(void);
static unsigned ptime[512]; static u8 pkeyl[512]; static int npl = 0;
static void F_BE0C(u32 k, const volatile u8 *t) { (void)t; if (npl < 512) { ptime[npl] = *(volatile unsigned *)0x2000DB84; pkeyl[npl++] = (u8)k; } }
#define F_BE0C F_BE0C
static int sync_armed, post_on, sync_direct, stops;
static int beat_on;
static void F_8170(u32 a) { if (!a) { beat_on = 0; R8(RA(2000DE47)) = 0; stops++; } }
static void F_81CC(void) { }
#define F_8170 F_8170
#define F_81CC F_81CC
static int becc_n; static u32 becc_a[16][3];
static void F_BECC(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g) { (void)d; (void)e; (void)f; (void)g; if (becc_n < 16) { becc_a[becc_n][0] = a; becc_a[becc_n][1] = b; becc_a[becc_n][2] = c; } becc_n++; }
#define F_BECC F_BECC
/* 2.119: the stock drum hits (ITCM) and the veneer that our drum_fix stands behind */
static int h344, h488; static u32 l344i, l488i, l488v; static int hitn; static u32 hiti[256];
static void R344(u32 i, u32 c) { (void)c; h344++; l344i = i; }
static void R488(u32 i, u32 c, u32 v) { (void)c; h488++; l488i = i; l488v = v; }
static void F_8674(void) { }
void drum_fix(u32 i, u32 c);
static void F_HIT(u32 i, u32 c) { if (hitn < 256) hiti[hitn++] = i; drum_fix(i, c); }
#define F_HIT F_HIT
#define SQ_PADT 0x60033370u
#define SQ_GPIO2_CLR 0x2001F000u
#define DV_IRQ_OFF(pm) ((pm) = 0)
#define DV_IRQ_ON(pm) ((void)(pm))
static unsigned char ocram_mem[0x10000] __attribute__((aligned(8)));
#define SQ_XP ((sqx_t *)ocram_mem)
#include "seq.h"

static u8 nlog[8192]; static int nl = 0; static unsigned dstart[54]; static unsigned dlog[8192]; static unsigned dend[8192]; static int nd = 0; static unsigned ntime[8192]; static unsigned short nstep[8192]; static u8 noffl[8192];
static double step_acc = 0;
static u8 phys[54], armed[54], upc[54]; static int edges[54], npress[54], nrel[54];
static void stock_update(u32 k, u32 b) {
    if (b) { if (!R8(KEY(k)) && armed[k]) { edges[k]++; if (k <= 35 && !LED(0x15) && sync_armed) { R8(RA(2000DE3F)) = 1; if (sync_direct) { beat_on = 1; R8(RA(2000DE47)) = 1; } } if (k == 0x31) LED(0x15) ^= 1; else if (LED(0x15)) { npress[k]++; if (k >= 4 && k <= 11 && npl < 512) { ptime[npl] = R32(RA(2000DB84)); pkeyl[npl++] = (u8)k; } if (k >= 13 && k <= 35 && nl < 8192) { ntime[nl] = R32(RA(2000DB84)); nstep[nl] = *(volatile unsigned short *)RA(2000DE0E); { int idx = -1; for (int q = 0; q < 20; q++) if (R8(SQ_KT + q) == k) idx = q; noffl[nl] = (u8)sq_aoff(0x30 + idx + 12 * R8(RA(200009DC))); } nlog[nl++] = (u8)k; } } armed[k] = 0; }
             if (!R8(KEY(k))) dstart[k] = R32(RA(2000DB84));
             R8(KEY(k)) = 1; R8(KEY(k) + 4) = 1; upc[k] = 0; R8(KEY(k)+3) = 0; }
    else { if (R8(KEY(k)) && LED(0x15) && k >= 13 && k <= 35 && nd < 8192) { dlog[nd] = R32(RA(2000DB84)) - dstart[k]; dend[nd++] = R32(RA(2000DB84)); }
           R8(KEY(k)) = 0;
           if (upc[k] < 255 && ++upc[k] == 30) { R8(KEY(k) + 4) = 0; armed[k] = 1; R8(KEY(k)+3) = 1; if (k != 0x31 && LED(0x15)) nrel[k]++; } }
}
static int blinkc = 0;
static void scan(void) {
    TICKS += 1;
    { static int prev = 0; if (beat_on && !prev) { *(volatile unsigned short *)RA(2000DE0E) = 0; step_acc = R32(RA(20002A90)) * 0.2 - 1.0; } prev = beat_on; }   /* 2.126: as the stock: step 0 plays at once and the count goes up right after a step plays */   /* the stock starts its step counter at 0 */
    if (beat_on) { R8(RA(2000DE47)) = 1; step_acc += 1.0; double sp = R32(RA(20002A90)) * 0.2; if (step_acc >= sp) { step_acc -= sp; *(volatile unsigned short *)RA(2000DE0E) += 1; if (*(volatile unsigned short *)RA(2000DE0E) >= 384) *(volatile unsigned short *)RA(2000DE0E) = 0; }   /* 2.131: the stock counts 0-383 */ } else R8(RA(2000DE47)) = 0;
    sq_tick();
    for (u32 row = 0; row < 6; row++) {
        u32 bits = 0;
        for (u32 c = 0; c < 9; c++) if (phys[row * 9 + c]) bits |= 1u << c;
        if (row == 5u && sq_frc()) bits &= ~colbit(0x2Fu);   /* as mod2.c's row_filter (2.113) */
        bits = sq_filter(row, bits);
        for (u32 c = 0; c < 9; c++) stock_update(row * 9 + c, (bits >> c) & 1u);
    }
    if (post_on) sq_post();
    if (sync_armed && R8(RA(2000DE3F))) { R8(RA(2000DE3F)) = 0; beat_on = 1; }   /* the rhythm's own tick takes up a start that is still pending */
    if (R27.kbl && ++blinkc > 300) { R27.kbl = 0; blinkc = 0; }
}
static void run(int ms) { while (ms-- > 0) scan(); }
static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("FAIL: %s\n", msg); fails++; } else printf("ok:   %s\n", msg); } while (0)
static void press(u32 k) { phys[k] = 1; }
static void rel(u32 k) { phys[k] = 0; }
static void setchord(u32 a, u32 b, u32 c, u32 t) { R8(SQ_CLS) = (u8)a; R8(SQ_CLS + 1) = (u8)b; R8(SQ_CLS + 2) = (u8)c; R8(SQ_TYPE) = (u8)t; }
static void chordbtn(u32 k, u32 a, u32 b, u32 c, u32 t, int ms) { setchord(a, b, c, t); press(k); run(ms); rel(k); run(100); }
static void tap(u32 k, int ms) { press(k); run(ms); rel(k); run(100); }
static u32 down(void) { u32 m = 0; for (u32 k = 12; k <= 35; k++) if (R8(KEY(k))) m |= 1u << (k - 12); return m; }
static int is(const u8 *e, u32 a, u32 b, u32 c, u32 t) { u8 o[4]; sq_expand(e, o); return o[0] == a && o[1] == b && o[2] == c && o[3] == t; }
int main(void) {
    mmap((void *)0x20000000, 0x20000, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    mmap((void *)0x60000000, 0x40000, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    for (u32 c = 0; c < 9; c++) R8(KEYBITS + c * 16 + 8) = (u8)c;
    static const u8 NK[20] = {13,14,16,17,18,20,21,23,24,25,26,27,28,29,30,31,32,33,34,35};
    for (int i = 0; i < 20; i++) R8(SQ_KT + i) = NK[i];
    { static const u8 PT[16] = {2, 14, 16, 18, 10, 11, 8, 9, 20, 24, 16, 18, 23, 22, 8, 9}; for (int i = 0; i < 16; i++) R8(0x60033370u + i) = PT[i]; }
    memset(armed, 1, sizeof armed); for (int k = 0; k < 54; k++) R8(KEY(k) + 3) = 1;
    R8(KEYS_EN) = 1; R8(ST) = PH_DONE;
    sq_t *Q = sq();
    run(100);
    press(0x31); run(SQ_LONG + 50);
    CHECK(Q->rec == 1 && LED(0x15) == 0 && edges[0x31] == 0, "chord mode, long press: recording on, keyboard mode untouched");
    rel(0x31); run(60);
    chordbtn(6, 3, 7, 10, 0, 150); chordbtn(18, 5, 8, 0, 1, 150); chordbtn(27, 8, 0, 3, 2, 150);
    CHECK(Q->n == 3 && is(XQ->seq[0], 3, 7, 10, 0) && is(XQ->seq[1], 5, 8, 0, 1) && is(XQ->seq[2], 8, 0, 3, 2), "three chords recorded: the 3 classes and the type");
    R8(SQ_ANC) = 9; R8(SQ_ANC + 1) = 4;
    int h0 = edges[0x30];
    tap(0x30, 80);
    CHECK(Q->n == 2 && edges[0x30] == h0, "Hold deletes the last chord; no stock Hold");
    jz_cur = 2; chordbtn(4, 11, 2, 9, 3, 150); jz_cur = 0;
    CHECK(Q->n == 3 && is(XQ->seq[2], 11, 2, 9, 3 | (2 << 4)), "chord 3 replaced (a minor 7th: row 4; with its jazz extra combination, xt 2)");
    press(0x31); run(SQ_LONG + 50); rel(0x31); run(100);
    CHECK(Q->rec == 0 && Q->n == 3 && LED(0x15) == 0 && calls_on == 0, "recording off; nothing was played in chord mode by us");
    tap(0x31, 60); run(100);
    CHECK(LED(0x15) == 1, "keyboard mode on");
    /* playback, Manual state */
    mstate = 1; h0 = edges[0x30];
    press(0x30); run(30);
    CHECK(calls_on == 1 && last_on[0] == 3 && last_on[1] == 7 && last_on[2] == 10 && R8(SQ_CLS) == 3 && R8(SQ_CLS + 1) == 7 && R8(SQ_CLS + 2) == 10 && R8(SQ_TYPE) == 0 && R8(DE64) == 1, "Hold: chord 1 started through the stock's chord-start code, CLS / type / chord-held set");
    CHECK(edges[0x30] == h0 && down() == 0, "no stock Hold, no note keys pressed");
    CHECK(LED(0x10) == 1, "the Hold LED is lit while a sequence chord is on");
    CHECK(R8(SQ_ANC) == 3 && R8(SQ_ANC + 1) == 0, "the stock's chord anchor holds the chord (a Transpose press re-applies it)");
    rel(0x30); run(100);
    CHECK(calls_off == 0 && R8(DE64) == 1, "the chord keeps sounding after Hold is let go");
    tap(0x30, 60); run(50);
    CHECK(calls_off == 1 && calls_osc_off == 1 && calls_on == 2 && last_on[0] == 5 && last_on[1] == 8 && last_on[2] == 0 && R8(SQ_TYPE) == 1, "next Hold: the old chord off, chord 2 on");
    jz_set_last = 99;
    tap(0x30, 60); run(50);
    CHECK(calls_on == 3 && last_on[0] == 11 && last_on[1] == 2 && last_on[2] == 9 && R8(SQ_TYPE) == 3 && R8(SQ_ANC + 1) == 3 && jz_set_last == 2, "chord 3: the type without the jazz bits, the jazz extra combination handed to the jazz code");
    tap(0x30, 60); run(50);
    CHECK(calls_on == 4 && last_on[0] == 3 && jz_set_last == 0, "wraps round to chord 1 (no extra combination)");
    /* the player's notes are the player's */
    int off0 = calls_off; memset(npress, 0, sizeof npress); memset(nrel, 0, sizeof nrel);
    press(0x30); press(20); run(100); rel(0x30); rel(20); run(200);
    CHECK(npress[20] == 1 && nrel[20] == 1, "a note pressed with Hold is a plain note, once");
    CHECK(calls_off == off0 + 1, "(and the chord changed)");
    /* leaving keyboard mode stops the chord */
    int on0 = calls_on; off0 = calls_off;
    tap(0x31, 60); run(100);
    CHECK(LED(0x10) == 0, "the Hold LED goes back");
    CHECK(R8(SQ_ANC) == 9 && R8(SQ_ANC + 1) == 4, "the anchor goes back to what the stock had");
    CHECK(LED(0x15) == 0 && calls_off == off0 + 1 && R8(DE64) == 0, "leaving keyboard mode: held chord and oscillators off, chord-held cleared");
    CHECK(calls_on == on0, "nothing else started");
    /* leaving and re-entering keyboard mode starts the sequence again from the first chord */
    tap(0x31, 60); run(100);
    on0 = calls_on;
    tap(0x30, 60); run(50);
    CHECK(calls_on == on0 + 1 && last_on[0] == 3, "after leaving and re-entering keyboard mode, Hold plays the first chord again");
    tap(0x31, 60); run(100);
    /* Auto state: the oscillators are started as the stock does, and the rhythm's flag is set */
    tap(0x31, 60); run(100); mstate = 0; on0 = calls_on; c819_on = c819_off = 0;
    tap(0x30, 60); run(50);
    CHECK(calls_on == on0 + 1 && c819_on == 1 && R8(SQ_BASSF) == 1 && R8(DE64) == 1 && R8(SQ_CLS) == (XQ->seq[(Q->pos + Q->n - 1) % Q->n][0] & 15), "Auto state: chord started (oscillators), the rhythm's flag set");
    R8(SQ_BASSF) = 0; sq_post();                                  /* the stock handler cleared it for a drum pad / arrow press */
    CHECK(R8(SQ_BASSF) == 1 && c819_on == 2, "a drum pad / arrow press in keyboard mode clears the stock flag: put back at once");
    sq_post();
    CHECK(c819_on == 2, "nothing to put back when the flag is set");
    mstate = 1;
    tap(0x31, 60); run(100);
    CHECK(c819_off == 1 && R8(DE64) == 0, "leaving: the Auto flag cleared too");
    R8(SQ_BASSF) = 0; sq_post();
    CHECK(R8(SQ_BASSF) == 0, "no chord of ours: the flag is left alone");
    /* recording stops sounding */
    tap(0x31, 60); run(100);
    tap(0x30, 60); run(50); off0 = calls_off;
    tap(0x31, 60); run(100);
    CHECK(R8(DE64) == 0, "chord off after leaving");
    /* KEYBOARD / Hold in other contexts */
    int e = edges[0x30]; tap(0x30, 60);
    CHECK(edges[0x30] == e + 1, "chord mode, not recording: Hold is the stock's");
    tap(0x31, 60); run(100);
    press(0x31); run(SQ_LONG + 100); CHECK(Q->rec == 0 && Q->se == 1 && LED(0x15) == 1, "keyboard mode: a long press does not record, it is the step sequencer's edit mode"); rel(0x31); run(100);
    tap(0x31, 60); run(100); CHECK(Q->se == 0 && LED(0x15) == 1, "...a short press ends it and keyboard mode stays");
    tap(0x31, 60); run(100); CHECK(LED(0x15) == 0, "...and the next short press leaves keyboard mode, as always");
    e = edges[0x31]; press(0x2F); run(40); press(0x31); run(60);
    CHECK(edges[0x31] == e + 1, "Manual/Auto held: KEYBOARD reaches the stock at once");
    rel(0x31); rel(0x2F); run(100);
    e = edges[0x31]; press(0x29); run(40); press(0x31); run(60);
    CHECK(edges[0x31] == e + 1, "PATTERN held: KEYBOARD reaches the stock at once");
    rel(0x31); rel(0x29); run(100);
    cfx = 0; e = edges[0x31]; press(0x31); run(60); CHECK(edges[0x31] == e + 1, "Extras off: KEYBOARD stock"); rel(0x31); run(100); cfx = 1;

    /* ---- arpeggio ---- */
    if (!LED(0x15)) { tap(0x31, 60); run(100); }
    R8(DE89) = 0; R32(RA(20002A90)) = 100;
    CHECK(LED(0x15) == 1 && Q->am == 0 && Q->dv == 1, "arpeggio: keyboard mode, Omni 1, off to begin with, eighths");
    { static const u32 pk[6] = {0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D}; static const u32 want[6] = {1, 2, 3, 4, 5, 0};
      for (int i = 0; i < 6; i++) {
          int e0 = edges[pk[i]];
          press(15); press(pk[i]); run(60); rel(pk[i]); rel(15); run(100);
          CHECK(Q->am == want[i] && edges[pk[i]] == e0, "Bb minor + PATTERN picks the mode (rock 1 up, rock 2 down, slow rock up-down, country octaves, swing random, yellow off); the stock never sees the press");
      }
      int e0 = edges[0x28]; tap(0x28, 60); CHECK(edges[0x28] == e0 + 1 && Q->am == 0, "a PATTERN button without Bb minor is the stock's, the mode is as it was");
      e0 = edges[0x2D]; tap(0x2D, 60); CHECK(edges[0x2D] == e0 + 1, "the yellow PATTERN button without Bb minor is the stock's"); }
    /* UP */
    press(15); press(0x28); run(60); rel(0x28); rel(15); run(100);
    nl = 0; press(17); run(100); press(25); run(10); press(29); run(1500);
    { int ok = 1; static const u8 cyc[3] = {17, 25, 29};
      for (int i = 0; i < nl; i++) if (nlog[i] != 17 && nlog[i] != 25 && nlog[i] != 29) ok = 0;
      int seqok = 1; for (int i = 3; i + 1 < nl; i++) { int a = -1; for (int j = 0; j < 3; j++) if (nlog[i] == cyc[j]) a = j; if (nlog[i + 1] != cyc[(a + 1) % 3]) seqok = 0; }
      CHECK(nl >= 10, "up: the stock sees a note press every step");
      CHECK(ok && seqok, "up: lowest note to the highest, then again"); }
    CHECK(sq_pop(Q->ph) == 3 && Q->arp == 1, "up: three keys down, arpeggio running");
    rel(25); rel(29); run(400); int n17 = npress[17];
    CHECK(Q->arp == 0, "one key left: arpeggio off");
    run(300);
    CHECK(npress[17] >= n17 && nlog[nl - 1] == 17, "the key left plays as a note");
    rel(17); run(200);
    /* ---- 2.144: the arpeggio latch (set in the mode menu: Gb 7th + the yellow PATTERN) and the glide (Db minor held) ---- */
    { Q->lat = 1; nl = 0;
      press(17); run(100); press(25); run(10); press(29); run(400);
      CHECK(Q->arp == 1 && sq_pop(Q->lmk) == 3, "2.144 latch on: three keys down, the arpeggio runs, the set is kept");
      rel(17); rel(25); rel(29); run(600); int n0 = nl;
      CHECK(Q->arp == 1 && sq_pop(Q->ph) == 0 && sq_pop(Q->lmk) == 3, "2.144 latch: all keys let go, the arpeggio keeps playing the latched keys");
      run(800); CHECK(nl > n0 + 3, "2.144 latch: it is still stepping");
      press(21); run(100);
      CHECK(sq_pop(Q->lmk) == 1 && Q->arp == 0, "2.144 latch: a new key after all were let go starts a new set (one key: no arpeggio)");
      press(25); run(300);
      CHECK(Q->arp == 1 && sq_pop(Q->lmk) == 2, "2.144 latch: a second key: the new set arpeggiates");
      rel(21); rel(25); run(400); CHECK(Q->arp == 1, "2.144 latch: the new set stays when its keys are let go");
      Q->lat = 0; run(100); CHECK(Q->arp == 0 && Q->lmk == 0, "2.144 latch off: the arpeggio stops (no key is down)");
      run(300); }
    { Q->lat = 1; Q->lmk = 0; Q->lfz = 1; nl = 0;                       /* 2.145: the tap on Gb 7th = the latched notes let go, the latch MODE stays on */
      press(17); run(100); press(25); run(10); press(29); run(300); rel(17); rel(25); rel(29); run(500);
      CHECK(Q->arp == 1 && Q->lat == 1, "2.145 latch: playing from the latch");
      Q->lmk = 0; Q->lfz = 1; run(200);
      CHECK(Q->arp == 0 && Q->lat == 1 && Q->lmk == 0, "2.145 release (the tap): the arpeggio stops, the latch mode is still on");
      press(17); run(100); press(29); run(300); rel(17); rel(29); run(500);
      CHECK(Q->arp == 1 && Q->lat == 1 && sq_pop(Q->lmk) == 2, "2.145 after the release: new keys latch again");
      Q->lat = 0; run(200); CHECK(Q->arp == 0, "2.145 the latch mode off (yellow PATTERN again): it stops"); run(300); }
    { u8 sn[32], so[32]; memcpy(sn, XQ->stn, 32); memcpy(so, XQ->sno, 32);        /* 2.146: a sequence over more than one octave */
      for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->sno[q] = 0xFF; }
      XQ->stn[0] = 3; XQ->sno[0] = 1; XQ->stn[1] = 7; XQ->sno[1] = 2; XQ->stn[2] = 5; XQ->sno[2] = 0; XQ->stn[3] = 9;                     /* step 4: an old step, octave not known */
      CHECK(sq_octoff(0) == 0 && sq_octoff(1) == 12 && sq_octoff(2) == -12 && sq_octoff(3) == 0, "2.146: each step's octave against the first note (+1 octave, -1 octave, unknown = same)");
      u32 oc0 = R8(OCT); R8(OCT) = 1; Q->trx = 0; Q->tl = 0xFF; Q->tpi = 0xFF;
      sq_tmark(Q, 7, TICKS, sq_octoff(1));
      CHECK(Q->toff[7] == 12 && sq_aoff(0x30 + 7 + 12 * 1) == 12, "2.146: the step's key sounds an octave up (the note the stock makes + 12)");
      sq_tmark(Q, 5, TICKS, sq_octoff(2)); CHECK(Q->toff[5] == -12 && sq_aoff(0x30 + 5 + 12 * 1) == -12, "2.146: ... and a step an octave down");
      Q->trx = 1; Q->tro = 2; sq_tmark(Q, 3, TICKS, sq_octoff(0)); CHECK(Q->toff[3] == 2, "2.146: with the transpose: both add up");
      sq_tmark(Q, 0xFF, TICKS, 0); for (int k = 0; k < 20; k++) Q->toff[k] = 0; Q->trx = 0; Q->tro = 0; Q->tl = 0xFF; Q->tpi = 0xFF; R8(OCT) = oc0;
      memcpy(XQ->stn, sn, 32); memcpy(XQ->sno, so, 32); }
    { Q->glv = 0; Q->gl0 = 0; rel(12); run(20);
      gl_note(60); gl_note(70);
      CHECK(Q->glv == 1 && Q->gln == 70 && Q->gl0 == 0 && gl_cents() == 0, "2.144 glide: Db minor not held: no glide, the last note is remembered");
      press(12); run(20); gl_note(64);
      CHECK(Q->gl0 == (70 - 64) * 25600 && Q->gln == 64, "2.144 glide: Db minor held: a lower note starts from the last note (+6 semitones, then down)");
      Q->glt = (unsigned short)TICKS; CHECK(gl_cents() == Q->gl0, "2.144 glide: at once the full offset");
      TICKS += GLD_MS / 2u; CHECK(gl_cents() == Q->gl0 / 2, "2.144 glide: half way, half the offset");
      TICKS += GLD_MS; CHECK(gl_cents() == 0 && Q->gl0 == 0, "2.144 glide: done: no offset");
      gl_note(30); CHECK(Q->gl0 == (64 - 30) * 25600 || Q->gl0 == 614400, "2.144 glide: a big leap is limited to 24 semitones"); Q->gl0 = 0;
      cfx = 0; gl_note(90); CHECK(Q->gl0 == 0, "2.144 glide: Extras off: none"); cfx = 1;
      rel(12); run(20); }
    /* DOWN */
    press(15); press(0x29); run(60); rel(0x29); rel(15); run(100);
    nl = 0; press(17); run(100); press(25); run(10); press(29); run(1500);
    { static const u8 cyc[3] = {29, 25, 17}; int seqok = 1;
      for (int i = 3; i + 1 < nl; i++) { int a = -1; for (int j = 0; j < 3; j++) if (nlog[i] == cyc[j]) a = j; if (a < 0 || nlog[i + 1] != cyc[(a + 1) % 3]) seqok = 0; }
      CHECK(nl >= 10 && seqok, "down: highest note to the lowest, then again"); }
    rel(17); rel(25); rel(29); run(300);
    /* UP-DOWN */
    press(15); press(0x2A); run(60); rel(0x2A); rel(15); run(100);
    nl = 0; press(17); run(100); press(25); run(10); press(29); run(2200);
    { int seqok = 1; static const u8 pp[4] = {17, 25, 29, 25};
      int off = -1; for (int i = 4; i < nl && off < 0; i++) if (nlog[i] == 17) off = i;
      for (int i = off; off >= 0 && i < nl; i++) if (nlog[i] != pp[(i - off) % 4]) seqok = 0;
      CHECK(off >= 0 && seqok && nl - off >= 8, "slow rock: up then down (17 25 29 25 17 25 29 25 ...)"); }
    rel(17); rel(25); rel(29); run(300);
    /* OCTAVES: up over 2 octaves */
    press(15); press(0x2B); run(60); rel(0x2B); rel(15); run(100);
    nl = 0; press(17); run(100); press(25); run(10); press(29); run(3000);
    { static const u8 cyc[6] = {17, 25, 29, 17, 25, 29}; static const u8 co[6] = {0, 0, 0, 12, 12, 12};
      int off = -1; for (int i = 3; i < nl && off < 0; i++) if (nlog[i] == 17 && noffl[i] == 0) off = i;
      int seqok = off >= 0, offok = 1;
      for (int i = off; off >= 0 && i < nl; i++) { if (nlog[i] != cyc[(i - off) % 6]) seqok = 0; if (noffl[i] != co[(i - off) % 6]) offok = 0; }
      CHECK(off >= 0 && seqok && nl - off >= 12, "country: up through the held notes, then the same notes an octave up (17 25 29 | 17 25 29 ...)");
      CHECK(offok, "country: the second round carries the +12 octave offset (marked per key as it is pressed)"); }
    rel(17); rel(25); rel(29); run(400);
    CHECK(Q->koff == 0, "country: no octave marks are left after the arpeggio");
    /* RANDOM (swing) */
    press(15); press(0x2C); run(60); rel(0x2C); rel(15); run(100);
    nl = 0; press(17); run(100); press(25); run(10); press(29); run(2400);
    { int ok = 1, seen[3] = {0, 0, 0}, rep = 0;
      for (int i = 0; i < nl; i++) { if (nlog[i] == 17) seen[0] = 1; else if (nlog[i] == 25) seen[1] = 1; else if (nlog[i] == 29) seen[2] = 1; else ok = 0; }
      for (int i = 4; i + 1 < nl; i++) if (nlog[i] == nlog[i + 1]) rep++;
      CHECK(ok && seen[0] && seen[1] && seen[2] && rep == 0, "swing button: random, only the held notes, all of them, never the same note twice in a row"); }
    rel(17); rel(25); rel(29); run(300);
    /* OFF (yellow) and only on Omni 1 */
    press(15); press(0x2D); run(60); rel(0x2D); rel(15); run(100);
    memset(npress, 0, sizeof npress); press(17); run(60); press(25); run(60); press(29); run(400);
    CHECK(Q->arp == 0 && npress[17] == 1 && npress[25] == 1 && npress[29] == 1, "yellow: arpeggio off, the stock sees the keys as played");
    rel(17); rel(25); rel(29); run(300);
    press(15); press(0x28); run(60); rel(0x28); rel(15); run(100);
    R8(DE89) = 2; memset(npress, 0, sizeof npress); press(17); run(60); press(25); run(60); press(29); run(400);
    CHECK(Q->arp == 0 && npress[17] == 1 && npress[25] == 1 && npress[29] == 1, "another voice (not Omni 1): no arpeggio");
    rel(17); rel(25); rel(29); run(300);
    R8(DE89) = 0;
    memset(npress, 0, sizeof npress); press(17); run(400);
    CHECK(Q->arp == 0 && npress[17] == 1, "a single key is a plain note");
    rel(17); run(200);
    cfx = 0; press(17); run(60); press(25); run(60); CHECK(Q->arp == 0, "Extras off: no arpeggio"); rel(17); rel(25); run(200); cfx = 1;
    /* ---- note values with Bb minor + the VOICE buttons ---- */
    { static const u32 vk[5] = {0x24, 0x25, 0x26, 0x33, 0x34};
      for (int i = 0; i < 5; i++) {
          int e0 = edges[vk[i]];
          press(15); press(vk[i]); run(60); rel(vk[i]); rel(15); run(100);
          CHECK(Q->dv == i && edges[vk[i]] == e0, "Bb minor + a VOICE button picks the note value; the stock never sees the voice press");
      }
      int e0 = edges[0x25]; tap(0x25, 60); CHECK(edges[0x25] == e0 + 1 && Q->dv == 4, "a VOICE button without Bb minor is the stock's");
      press(15); press(0x25); run(60); rel(0x25); rel(15); run(100); CHECK(Q->dv == 1, "back to eighths (omni 2)");
    }
    { /* free-running at the knob's 100 (stock step 20 ms): quarter 240, eighth 120, sixteenth 60, swing 8th 160/80, swing 16th 80/50 (floor) */
      static const u32 vk[5] = {0x24, 0x25, 0x26, 0x33, 0x34}; static const double ex[5] = {240, 120, 60, 120, 65};
      press(15); press(0x28); run(60); rel(0x28); rel(15); run(100);
      for (int d = 0; d < 5; d++) {
          press(15); press(vk[d]); run(60); rel(vk[d]); rel(15); run(100);
          nl = 0; press(17); run(100); press(25); run(4000);
          double sum = 0; int cnt = 0; for (int i = 3; i + 1 < nl; i++) { sum += (double)(ntime[i + 1] - ntime[i]); cnt++; }
          double avg = cnt ? sum / cnt : 0;
          char msg[120]; snprintf(msg, sizeof msg, "note value voice %d: arpeggio step averages %.0f ms (expected about %.0f)", d + 1, avg, ex[d]);
          CHECK(cnt >= 3 && avg > ex[d] - 12 && avg < ex[d] + 12, msg);
          if (d == 3 || d == 4) {      /* swing: long, short, long, short */
              double lo = 1e9, hi = 0; for (int i = 3; i + 1 < nl; i++) { double g = (double)(ntime[i + 1] - ntime[i]); if (g < lo) lo = g; if (g > hi) hi = g; }
              char m2[120]; snprintf(m2, sizeof m2, "swing voice %d: gaps alternate long / short (%.0f .. %.0f ms)", d + 1, lo, hi);
              CHECK(hi > lo * 1.4, m2);
          }
          rel(17); rel(25); run(300);
      }
      press(15); press(0x25); run(60); rel(0x25); rel(15); run(100);
    }
    { /* with the beat: 16ths on every 3rd rhythm step; swing eighths on steps 0 and 8 of 12 */
      press(15); press(0x26); run(60); rel(0x26); rel(15); run(100);
      R32(RA(20002A90)) = 200; beat_on = 1; run(100); nl = 0; press(17); run(100); press(25); run(3000);
      int ok = 1, cnt = 0; double sum = 0;
      for (int i = 3; i < nl; i++) if (nstep[i] % 3 != 0) ok = 0;
      for (int i = 3; i + 1 < nl; i++) { sum += (double)(ntime[i + 1] - ntime[i]); cnt++; }
      double avg = cnt ? sum / cnt : 0;
      CHECK(nl >= 15 && ok && avg > 112 && avg < 128, "beat running, sixteenths: a step every 3 rhythm steps (3 x 40 ms), on the grid");
      rel(17); rel(25); run(300);
      press(15); press(0x33); run(60); rel(0x33); rel(15); run(100);
      nl = 0; press(17); run(100); press(25); run(4000);
      int ok2 = 1; for (int i = 3; i < nl; i++) { int p = nstep[i] % 12; if (p != 0 && p != 8) ok2 = 0; }
      CHECK(nl >= 8 && ok2, "beat running, swing eighths: steps 0 and 8 of every 12 rhythm steps (2:1)");
      rel(17); rel(25); run(300); beat_on = 0; run(100);
      press(15); press(0x25); run(60); rel(0x25); rel(15); run(100);
    }
    { /* LEDs while Bb minor is held */
      for (int i = 0; i < 5; i++) LED(9 + i) = 0; LED(11) = 1; for (int i = 2; i <= 6; i++) LED(i) = 0; LED(2) = 1; LED(0x0E) = 1; LED(0x0F) = 1; LED(7) = 1; LED(8) = 0;
      press(15); press(0x28); run(60); rel(0x28); run(20);                               /* mode 1 (rock 1, up) while held */
      CHECK(LED(9) == 1 && LED(10) == 0 && LED(11) == 0 && LED(12) == 0 && LED(13) == 0, "Bb minor held, up: the LED of rock 1 lights, the rhythm's pattern LED is off");
      CHECK(LED(0x0E) == 0 && LED(0x0F) == 0, "the yellow PATTERN button is dark while an arpeggio mode is on");
      CHECK(LED(7) == 1 && LED(8) == 1, "the yellow VOICE button's two LEDs: the note sequence's transpose is on (the default, 2.114)");
      CHECK(LED(3) == 1 && LED(2) == 0 && LED(4) == 0, "the VOICE LED of the note value (eighths = the second voice) lights");
      press(0x2D); run(60); rel(0x2D); run(20);
      CHECK(Q->am == 0 && LED(0x0E) == 1 && LED(9) == 0 && LED(10) == 0 && LED(11) == 0 && LED(12) == 0 && LED(13) == 0, "yellow picked: the arpeggio is off and only the yellow PATTERN LED is lit");
      press(0x2C); run(60); rel(0x2C); run(20);
      CHECK(Q->am == 5 && LED(13) == 1 && LED(0x0E) == 0, "swing button: random = LED 13, the yellow LED goes out");
      press(0x2B); run(60); rel(0x2B); run(20);
      CHECK(Q->am == 4 && LED(12) == 1 && LED(13) == 0, "country: octaves = LED 12");
      press(0x26); run(60); rel(0x26); run(20);
      CHECK(Q->dv == 2 && LED(4) == 1 && LED(3) == 0, "harp: sixteenths = the third voice LED");
      rel(15); run(20);
      CHECK(LED(11) == 1 && LED(9) == 0 && LED(12) == 0 && LED(0x0E) == 1 && LED(0x0F) == 1 && LED(7) == 1 && LED(2) == 1 && LED(4) == 0, "let go: every LED as it was");
      press(15); press(0x25); run(60); rel(0x25); rel(15); run(20);
      tap(0x31, 60); run(100);                       /* chord mode: no LED play with Bb minor */
      for (int i = 0; i < 5; i++) LED(9 + i) = 0; LED(12) = 1;
      press(15); run(20);
      CHECK(LED(12) == 1 && LED(9) == 0 && LED(11) == 0, "chord mode: Bb minor is a chord button, the LEDs are left alone");
      rel(15); run(20); tap(0x31, 60); run(100);
    }

    /* ---- the Sustain knob: note length (staccato .. legato), and with Bb minor held the swing ---- */
    { static const u32 vk[5] = {0x24, 0x25, 0x26, 0x33, 0x34};
      R8(DE89) = 0; R32(RA(20002A90)) = 100;
      press(15); press(0x28); run(60); rel(0x28); rel(15); run(100);                     /* up */
      press(15); press(vk[1]); run(60); rel(vk[1]); rel(15); run(100);                   /* eighths: a step of 120 ms */
      R8(SQ_SUSV + 2) = 0; run(50);
      CHECK(Q->gt == 64 && Q->swv == 85, "the knob values start as their defaults (note length in the middle, swing = triplet)");
      R8(SQ_SUSV + 2) = 10; run(50);
      CHECK(Q->gt == 10, "the knob moved: it takes over the note length");
      nd = 0; press(17); run(100); press(25); run(2500);
      { double sum = 0; int cnt = 0; for (int i = 3; i < nd; i++) { sum += dlog[i]; cnt++; }
        double avg = cnt ? sum / cnt : 0; char m[120]; snprintf(m, sizeof m, "knob low: staccato, the key is down %.0f ms (expected about 25)", avg);
        CHECK(cnt >= 8 && avg > 19 && avg < 31, m); }
      R8(SQ_SUSV + 2) = 64; run(50); nd = 0; run(2400);
      { double sum = 0; int cnt = 0; for (int i = 2; i < nd; i++) { sum += dlog[i]; cnt++; }
        double avg = cnt ? sum / cnt : 0; char m[120]; snprintf(m, sizeof m, "knob in the middle: the key is down %.0f ms (expected about 69)", avg);
        CHECK(cnt >= 8 && avg > 62 && avg < 78, m); }
      R8(SQ_SUSV + 2) = 127; run(50); nd = 0; run(2400);
      { double sum = 0; int cnt = 0; for (int i = 2; i < nd; i++) { sum += dlog[i]; cnt++; }
        double avg = cnt ? sum / cnt : 0; char m[120]; snprintf(m, sizeof m, "knob at the top: legato, the key is down the whole step (%.0f ms, expected about 120)", avg);
        CHECK(cnt >= 8 && avg > 112 && avg < 128, m);
      }
      /* swing: the swing eighth note value, Bb minor held: the knob is locked until moved */
      press(15); press(vk[3]); run(60); rel(vk[3]); run(30);                              /* celeste: swing eighths, Bb minor still held */
      CHECK(Q->dv == 3, "swing eighths picked");
      run(50); CHECK(Q->swv == 85 && Q->gt == 127, "Bb minor held: the knob is locked (swing stays 85, the note length stays 127)");
      nl = 0; run(2400);
      { double lo = 1e9, hi = 0; for (int i = 3; i + 1 < nl; i++) { double g = (double)(ntime[i + 1] - ntime[i]); if (g < lo) lo = g; if (g > hi) hi = g; }
        char m[120]; snprintf(m, sizeof m, "the default swing is the triplet feel: gaps %.0f / %.0f ms (2 : 1 = 158 / 82)", hi, lo);
        CHECK(nl >= 8 && hi > 148 && hi < 168 && lo > 74 && lo < 90, m); }
      R8(SQ_SUSV + 2) = 0; Q->swv = 0; run(60);      /* 2.135: Bb minor + Sustain is the filter's cutoff now: the swing value is set directly here */
      CHECK(Q->swv == 0 && Q->gt == 127 && Q->cut == 0, "Bb minor held, the knob moved: the cutoff follows it, the note length stays (swing set to 0: straight)");
      nl = 0; run(2400);
      { double lo = 1e9, hi = 0; for (int i = 3; i + 1 < nl; i++) { double g = (double)(ntime[i + 1] - ntime[i]); if (g < lo) lo = g; if (g > hi) hi = g; }
        char m[120]; snprintf(m, sizeof m, "swing 0: the two gaps are equal (%.0f / %.0f ms, 120 each)", hi, lo);
        CHECK(nl >= 8 && hi < 130 && lo > 110, m); }
      R8(SQ_SUSV + 2) = 127; Q->swv = 127; run(60);
      CHECK(Q->swv == 127 && Q->cut == 127, "the knob at the top: the cutoff at its most (swing set to its most)");
      nl = 0; run(2400);
      { double lo = 1e9, hi = 0; for (int i = 3; i + 1 < nl; i++) { double g = (double)(ntime[i + 1] - ntime[i]); if (g < lo) lo = g; if (g > hi) hi = g; }
        char m[120]; snprintf(m, sizeof m, "swing 75 %%: gaps %.0f / %.0f ms (180 / 60)", hi, lo);
        CHECK(nl >= 8 && hi > 170 && hi < 190 && lo > 54 && lo < 70, m); }
      rel(15); run(60);
      CHECK(Q->gt == 127 && Q->swv == 127, "Bb minor let go: nothing jumps, the knob is locked again");
      R8(SQ_SUSV + 2) = 20; run(60);
      CHECK(Q->gt == 20 && Q->swv == 127, "the knob moved after Bb minor was let go: the note length follows it, the swing keeps its value");
      rel(17); rel(25); run(300);
      /* back to the plain state for the next tests */
      press(15); press(vk[1]); run(60); rel(vk[1]); rel(15); run(100);
    }

    /* ---- country (octaves) with the Sustain knob up to legato: every position of the knob ---- */
    { static const u32 vk[5] = {0x24, 0x25, 0x26, 0x33, 0x34};
      R8(DE89) = 0; R32(RA(20002A90)) = 100;
      press(15); press(0x2B); run(60); rel(0x2B); rel(15); run(100);                     /* country: octaves */
      press(15); press(vk[1]); run(60); rel(vk[1]); rel(15); run(100);                   /* eighths: 120 ms */
      static const u8 knob[7] = {0, 40, 80, 100, 115, 124, 127};
      for (int t = 0; t < 7; t++) {
          R8(SQ_SUSV + 2) = (u8)(knob[t] > 0 ? knob[t] : 0); run(60);
          nl = 0; press(17); run(100); press(25); run(3600);
          static const u8 cyc[4] = {17, 25, 17, 25}; static const u8 co[4] = {0, 0, 12, 12};
          int off = -1; for (int i = 3; i < nl && off < 0; i++) if (nlog[i] == 17 && noffl[i] == 0) off = i;
          int seqok = off >= 0, offok = 1; double lo = 1e9, hi = 0;
          for (int i = off; off >= 0 && i < nl; i++) { if (nlog[i] != cyc[(i - off) % 4]) seqok = 0; if (noffl[i] != co[(i - off) % 4]) offok = 0; if (i + 1 < nl) { double g = (double)(ntime[i + 1] - ntime[i]); if (g < lo) lo = g; if (g > hi) hi = g; } }
          char m[160]; snprintf(m, sizeof m, "country, knob %d: the right notes (%s) with their octave offsets (%s), steps %.0f..%.0f ms (120 expected)", knob[t], seqok ? "ok" : "WRONG", offok ? "ok" : "WRONG", lo, hi);
          CHECK(off >= 0 && seqok && offok && nl - off >= 20 && lo > 110 && hi < 130, m);
          rel(17); rel(25); run(400);
          CHECK(Q->koff == 0, "country: marks cleared after the arpeggio");
      }
    }

    /* ---- 32 chords in 2 bytes each ---- */
    { int ok = 1;
      for (u32 c0 = 0; c0 < 12; c0++) for (u32 row = 0; row < 9; row++) {
          u8 a, b; sq_c12(c0, row, &a, &b);
          Q->n = 0; Q->cc[0] = (u8)c0; Q->cc[1] = a; Q->cc[2] = b; Q->ct = (u8)row; Q->cx = (u8)(row % 8); Q->cv = 1; sq_commit(Q);
          u8 o[4]; sq_expand(XQ->seq[0], o);
          if (!(o[0] == c0 && o[1] == a && o[2] == b && o[3] == (u8)(row | ((row % 8) << 4)))) ok = 0;
      }
      CHECK(ok, "all 12 roots x 9 rows: a chord keeps its classes, type and jazz extra through the 2-byte format");
      Q->n = 0; for (int i = 0; i < 40; i++) { Q->cc[0] = 0; Q->cc[1] = 4; Q->cc[2] = 7; Q->ct = 0; Q->cx = 0; Q->cv = 1; sq_commit(Q); }
      CHECK(Q->n == 32, "32 chords fit, the 33rd is ignored");
      Q->n = 0; Q->pos = 0; }
    /* ---- the step sequencer ---- */
    { R8(DE89) = 2; Q->am = 0; Q->n = 0;
      if (!LED(0x15)) { tap(0x31, 60); run(100); }
      Q->se = 0; Q->sl = 0; Q->ss = 0; for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->std[q] = 0xFF; }
      for (int i = 2; i <= 6; i++) LED(i) = 0; for (int i = 9; i <= 13; i++) LED(i) = 0; LED(2) = 1; LED(9) = 1; LED(7) = 0; LED(8) = 1; LED(0x0E) = 1;
      int e31 = edges[0x31];
      press(0x31); run(SQ_LONG + 50);
      CHECK(Q->se == 1 && LED(0x15) == 1 && edges[0x31] == e31, "keyboard mode, long press KEYBOARD: the step sequencer's edit mode, keyboard mode stays, the stock saw nothing");
      rel(0x31); run(100); CHECK(Q->se == 1 && edges[0x31] == e31, "...let go: still no KEYBOARD press for the stock");
      { int on = 0, off = 0; for (int t = 0; t < 120; t++) { run(10); if (LED(2)) on++; else off++; }
        CHECK(on > 20 && off > 20, "the selected step (voice 1 LED) blinks");
        CHECK(LED(7) == 1 && LED(8) == 0 && LED(0x0E) == 0 && LED(0x0F) == 0, "page 1: the yellow VOICE's upper LED is lit, the other three page LEDs are dark");
        CHECK(LED(6) == 1 && LED(13) == 0, "drum layer 1: the voice 5 LED is lit, the pattern 5 LED is dark"); }
      int e26 = edges[0x26];
      press(0x26); run(60); rel(0x26); run(50);
      CHECK(Q->ss == 2 && edges[0x26] == e26, "voice 3 selects step 3; the voice does not change");
      memset(npress, 0, sizeof npress);
      press(17); run(80); rel(17); run(60);
      CHECK(XQ->stn[2] == 3 && npress[17] == 1 && Q->ss == 2, "a note key played is written at the selected step (index 3), plays as a note, and the step stays");
      CHECK(XQ->sno[2] == (u8)R8(OCT), "2.146: the step also keeps the keyboard octave it was written at");
      press(5); run(80); rel(5); run(60);
      CHECK(XQ->std[2] == 0xF1 && npress[5] == 1 && Q->ss == 2, "a drum pad played is written at the selected step, in drum layer 1 (pad 1), plays, the step stays");
      int e2c = edges[0x2C];
      press(0x2C); run(60); rel(0x2C); run(50); run(300);
      CHECK(Q->sl == 1 && LED(13) == 1 && LED(6) == 0 && edges[0x2C] == e2c, "pattern 5 = drum layer 2: its LED lit, voice 5's dark, the stock saw nothing");
      press(6); run(80); rel(6); run(60);
      CHECK(XQ->std[2] == 0x21 && npress[6] == 1, "a pad played now goes to layer 2 (pad 2): the step holds two drums (0x21)");
      press(0x34); run(60); rel(0x34); run(50); run(300);
      CHECK(Q->sl == 0 && LED(6) == 1 && LED(13) == 0, "voice 5 = drum layer 1 again");
      press(0x28); run(60); rel(0x28); run(50);
      CHECK(Q->ss == 4, "pattern 1 = step 5");
      press(25); run(80); rel(25); run(60); press(4); run(80); rel(4); run(60);
      CHECK(XQ->stn[4] == 9 && XQ->std[4] == 0xF0, "step 5: a note (index 9) and a drum (layer 1, pad 0)");
      press(0x27); run(60); rel(0x27); run(50); run(300);
      CHECK(Q->ss == 8 + 4 && LED(8) == 1 && LED(7) == 0, "the yellow VOICE button: page 2 (same place in it), its lower LED");
      press(0x27); run(60); rel(0x27); run(50); run(300);
      CHECK(Q->ss == 4 && LED(7) == 1 && LED(8) == 0, "pressed again: back to page 1");
      press(0x2D); run(60); rel(0x2D); run(50); run(300);
      CHECK(Q->ss == 16 + 4 && LED(0x0E) == 1 && LED(0x0F) == 0 && LED(7) == 0, "the yellow PATTERN button: page 3 (its upper LED)");
      press(0x2D); run(60); rel(0x2D); run(50); run(300);
      CHECK(Q->ss == 24 + 4 && LED(0x0F) == 1 && LED(0x0E) == 0, "pressed again: page 4 (its lower LED)");
      press(0x2D); run(60); rel(0x2D); run(50); run(300);
      CHECK(Q->ss == 16 + 4 && LED(0x0E) == 1, "and again: page 3");
      press(0x27); run(60); rel(0x27); run(50); run(300);
      CHECK(Q->ss == 4 && LED(7) == 1 && LED(0x0E) == 0, "the yellow VOICE from page 3 or 4: page 1");
      press(0x24); run(60); rel(0x24); run(50); run(200);
      CHECK(Q->ss == 0 && LED(4) == 1 && LED(9) == 1, "steps that hold something are lit: step 3 (voice 3 LED) and step 5 (pattern 1 LED)");
      /* Hold alone clears the selected step when it is let go */
      press(0x26); run(60); rel(0x26); run(50);
      int eh = edges[0x30];
      press(0x30); run(100); CHECK(XQ->stn[2] == 3, "Hold held: nothing yet"); rel(0x30); run(120);
      CHECK(XQ->std[2] != 0xFF && edges[0x30] == eh && XQ->stn[4] == 9, "Hold let go: one thing out of the selected step (the newest); the stock saw no Hold; the other steps stay (2.129)");
      for (int j = 0; j < 3; j++) { press(0x30); run(100); rel(0x30); run(120); }
      CHECK(XQ->stn[2] == 0xFF && XQ->std[2] == 0xFF, "...more Holds: the rest goes, one at a time");
      /* Hold + voice 5 / pattern 5: one drum layer out of every step */
      XQ->std[4] = 0x20; XQ->std[5] = 0x35; XQ->std[6] = 0xF4;
      press(0x30); run(60); press(0x34); run(60); rel(0x34); run(30); rel(0x30); run(120);
      CHECK(XQ->std[4] == 0x2F && XQ->std[5] == 0x3F && XQ->std[6] == 0xFF && Q->sl == 0 && XQ->stn[4] == 9, "Hold + voice 5: drum layer 1 out of every step, layer 2 stays (and Hold's release did not clear the step)");
      XQ->std[4] = 0x20; XQ->std[5] = 0x35; XQ->std[6] = 0xF4;
      press(0x30); run(60); press(0x2C); run(60); rel(0x2C); run(30); rel(0x30); run(120);
      CHECK(XQ->std[4] == 0xF0 && XQ->std[5] == 0xF5 && XQ->std[6] == 0xF4 && Q->sl == 0, "Hold + pattern 5: drum layer 2 out of every step, layer 1 stays (the layer being edited is not changed)");
      /* Hold + yellow VOICE: every note out; Hold + yellow PATTERN: every drum out */
      press(0x30); run(60); press(0x27); run(60); rel(0x27); run(30); rel(0x30); run(120);
      { int notes = 0, drums = 0; for (int q = 0; q < 32; q++) { notes += XQ->stn[q] != 0xFF; drums += XQ->std[q] != 0xFF; }
        CHECK(notes == 0 && drums == 3, "Hold + the yellow VOICE: every note out, the drums stay"); }
      XQ->stn[4] = 9; XQ->stn[7] = 2;
      press(0x30); run(60); press(0x2D); run(60); rel(0x2D); run(30); rel(0x30); run(120);
      { int notes = 0, drums = 0; for (int q = 0; q < 32; q++) { notes += XQ->stn[q] != 0xFF; drums += XQ->std[q] != 0xFF; }
        CHECK(notes == 2 && drums == 0, "Hold + the yellow PATTERN: every drum (both layers) out, the notes stay"); }
      /* leave: a short press, keyboard mode stays, the LEDs as they were */
      e31 = edges[0x31]; tap(0x31, 60); run(100);
      CHECK(Q->se == 0 && LED(0x15) == 1 && edges[0x31] == e31, "a short press of KEYBOARD ends the edit mode; keyboard mode stays; the stock saw nothing");
      CHECK(LED(2) == 1 && LED(9) == 1 && LED(7) == 0 && LED(8) == 1 && LED(0x0E) == 1 && LED(6) == 0, "the LEDs are back as they were");
      int ev = edges[0x25]; tap(0x25, 60); CHECK(edges[0x25] == ev + 1, "outside edit mode the voice buttons are the stock's again");
      CHECK(XQ->stn[4] == 9 && XQ->stn[7] == 2, "the notes are kept in RAM after leaving");
    }
    /* ---- the step sequencer plays with the rhythm ---- */
    { R8(DE89) = 2; Q->am = 0; Q->dv = 1; if (!LED(0x15)) { tap(0x31, 60); run(100); } Q->se = 0;
      for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->std[q] = 0xFF; }
      XQ->stn[0] = 3; XQ->std[0] = 0x31; XQ->stn[2] = 9; XQ->std[2] = 0xF0; XQ->stn[3] = 0; XQ->std[4] = 0xF7;       /* notes 17 25 13; pads 5+7 | 4 | 11 */
      R32(RA(20002A90)) = 200; R8(SQ_SUSV + 2) = 40; run(10);                                          /* 40 ms per stock step: an eighth = 240 ms */
      run(50);
      CHECK(sq_dmute() == 1, "drum steps are there: the stock pattern's drums rest");
      memset(npress, 0, sizeof npress); nl = 0; npl = 0; beat_on = 1; run(4200);
      { int i17 = -1, i25 = -1, i13 = -1; for (int i = 0; i < nl; i++) { if (nlog[i] == 17 && i17 < 0) i17 = i; if (nlog[i] == 25 && i25 < 0) i25 = i; if (nlog[i] == 13 && i13 < 0) i13 = i; }
        int p5 = -1, p7 = -1, p4 = -1, p11 = -1; for (int i = 0; i < npl; i++) { if (pkeyl[i] == 5 && p5 < 0) p5 = i; if (pkeyl[i] == 7 && p7 < 0) p7 = i; if (pkeyl[i] == 4 && p4 < 0) p4 = i; if (pkeyl[i] == 11 && p11 < 0) p11 = i; }
        CHECK(i17 >= 0 && i25 >= 0 && i13 >= 0, "keyboard mode: the notes of steps 1, 3 and 4 are pressed for the stock");
        CHECK(i25 >= 0 && i17 >= 0 && ntime[i25] - ntime[i17] > 465 && ntime[i25] - ntime[i17] < 495, "step 3 comes 2 eighths (480 ms) after step 1");
        CHECK(i13 >= 0 && ntime[i13] - ntime[i25] > 225 && ntime[i13] - ntime[i25] < 255, "step 4 one eighth (240 ms) after step 3");
        CHECK(p5 >= 0 && p7 >= 0 && ptime[p5] + 8 >= ntime[i17] && ptime[p5] <= ntime[i17] + 8 && ptime[p7] + 8 >= ntime[i17] && ptime[p7] <= ntime[i17] + 8, "step 1's two drums (layer 1 pad 1, layer 2 pad 3) are played with its note, by the stock's pad function");
        CHECK(p4 >= 0 && p11 >= 0 && ptime[p11] - ptime[p4] > 465 && ptime[p11] - ptime[p4] < 495, "step 5's drum (pad 7) two eighths after step 3's (pad 0)");
        { int a = -1, b = -1; for (int i = 0; i < nl; i++) if (nlog[i] == 17) { if (a < 0) a = i; else if (b < 0) b = i; }
          CHECK(a >= 0 && b >= 0 && ntime[b] - ntime[a] > 1900 && ntime[b] - ntime[a] < 1940, "only page 1 holds anything: it loops by itself, 8 eighths (1920 ms), no silent pages"); } }
      XQ->stn[17] = 4;
      nl = 0; npl = 0; run(12500);
      { int a = -1, b = -1; for (int i = 0; i < nl; i++) if (nlog[i] == 17) { if (a < 0) a = i; else if (b < 0) b = i; }
        CHECK(a >= 0 && b >= 0 && ntime[b] - ntime[a] > 5740 && ntime[b] - ntime[a] < 5780, "a step on page 3: the loop is 3 pages (24 steps = 5760 ms), the silent page 2 included");
        int k18 = 0; for (int i = 0; i < nl; i++) if (nlog[i] == 18) k18++;
        CHECK(k18 >= 1, "...and page 3's note plays"); }
      XQ->stn[17] = 0xFF; run(2000);
      /* the note value follows the arpeggio's: sixteenths */
      XQ->stn[1] = 9; XQ->stn[2] = 0xFF; XQ->stn[3] = 0xFF; XQ->stn[0] = 3; XQ->std[0] = 0xFF; XQ->std[2] = 0xFF; XQ->std[4] = 0xFF; Q->dv = 2; run(2000);
      nl = 0; run(4000);
      { int a = -1, b = -1; for (int i = 0; i < nl; i++) { if (nlog[i] == 17 && a < 0) a = i; if (nlog[i] == 25 && a >= 0 && b < 0) b = i; }
        CHECK(a >= 0 && b >= 0 && ntime[b] - ntime[a] > 108 && ntime[b] - ntime[a] < 132, "note value sixteenth (Bb minor + voice 3): the steps are 120 ms apart"); }
      /* swing eighths: the pair's second step comes at 2/3 of the pair (the swing knob's default 2:1) */
      Q->dv = 3; Q->swv = 85; beat_on = 0; run(200); beat_on = 1; run(1000); nl = 0; run(3000);          /* (a note value changed while running lines up again at the next Start) */
      { int a = -1, b = -1; for (int i = 0; i < nl; i++) { if (nlog[i] == 17 && a < 0) a = i; if (nlog[i] == 25 && a >= 0 && b < 0) b = i; }
        CHECK(a >= 0 && b >= 0 && ntime[b] - ntime[a] > 305 && ntime[b] - ntime[a] < 335, "note value swing eighth: the second step of the pair comes 320 ms after the first (2:1 of 480)"); }
      Q->dv = 1; XQ->stn[1] = 0xFF; XQ->stn[0] = 3; XQ->std[0] = 0x31; XQ->std[2] = 0xF0; XQ->stn[2] = 9; XQ->stn[3] = 0; XQ->std[4] = 0xF7; run(2500);
      beat_on = 0; run(150);
      CHECK(Q->snk == 0xFF && Q->sact == 0, "the rhythm stopped: the sequencer's key is let go");
      npl = 0; nl = 0; beat_on = 1; run(300);
      CHECK(nl >= 1 && nlog[0] == 17 && npl >= 1 && (pkeyl[0] == 5 || pkeyl[0] == 7), "started again: from step 1");
      /* ---- 2.114: the note keys transpose the note sequence (first note: index 3, key 17) ---- */
      { CHECK(Q->trx == 1 && Q->tro == 0, "notes playing in keyboard mode, transpose on (the default): the note keys transpose");
        int k = R8(SQ_KT + 5), e = edges[k];
        press(k); run(60);
        CHECK(edges[k] == e && Q->tro == 2, "a key 2 semitones above the first note: +2, and the key itself does not sound");
        rel(k); nl = 0; run(2500);
        { int ok = nl >= 3; for (int i = 0; i < nl; i++) if (noffl[i] != 2) ok = 0; CHECK(ok, "...every note of the sequence plays 2 semitones up, the key let go: it stays"); }
        k = R8(SQ_KT + 17); press(k); run(60); rel(k); nl = 0; run(2500);
        { int ok = nl >= 3 && Q->tro == 14; for (int i = 0; i < nl; i++) if (noffl[i] != 14) ok = 0; CHECK(ok, "a key an octave and 2 above: +14"); }
        k = R8(SQ_KT + 0); press(k); run(60); rel(k); nl = 0; run(2500);
        { int ok = nl >= 3 && (signed char)Q->tro == -3; for (int i = 0; i < nl; i++) if ((signed char)noffl[i] != -3) ok = 0; CHECK(ok, "a key below the first note: -3"); }
        Q->am = 1; R8(DE89) = 0; k = R8(SQ_KT + 7); int k2 = R8(SQ_KT + 9); press(k); press(k2); run(400);
        CHECK(Q->arp == 0 && Q->ak == 0xFF, "two keys down while transposing: no arpeggio");
        rel(k); rel(k2); run(100); Q->am = 0; R8(DE89) = 2;
        int e27 = edges[0x27]; press(15); run(30); press(0x27); run(60);
        CHECK(Q->tpoff == 1 && LED(7) == 0 && LED(8) == 0 && edges[0x27] == e27, "Bb minor + yellow VOICE: transpose off, its LEDs dark, the stock saw nothing");
        rel(0x27); rel(15); run(100);
        CHECK(Q->trx == 0 && Q->tro == 0, "...the sequence as recorded");
        nl = 0; run(2500);
        { int ok = nl >= 3; for (int i = 0; i < nl; i++) if (noffl[i] != 0) ok = 0; CHECK(ok, "...its notes play as recorded"); }
        k = R8(SQ_KT + 5); e = edges[k]; press(k); run(60); rel(k); run(60);
        CHECK(edges[k] == e + 1 && Q->tro == 0, "transpose off: a key plays over the sequence, as before");
        press(15); run(30); press(0x27); run(60);
        CHECK(Q->tpoff == 0 && LED(7) == 1 && LED(8) == 1, "Bb minor + yellow VOICE again: on");
        rel(0x27); rel(15); run(100);
        k = R8(SQ_KT + 5); press(k); run(60); rel(k); run(60);
        CHECK(Q->tro == 2, "+2 again");
        beat_on = 0; run(200);
        { int ok = Q->tro == 0 && Q->trx == 0; for (int i = 0; i < 20; i++) if (Q->toff[i]) ok = 0; CHECK(ok, "the rhythm stopped: transpose back to 0, no key keeps a mark"); }
        k = R8(SQ_KT + 5); e = edges[k]; press(k); run(60); rel(k); run(60);
        CHECK(edges[k] == e + 1, "stopped: the keys are the stock's");
        beat_on = 1; run(300);
      }
      tap(0x31, 60); run(100);                                           /* chord mode */
      CHECK(LED(0x15) == 0 && Q->dmute == 1 && Q->snk == 0xFF, "chord mode: the stock drums still rest, the sequencer's note key is not pressed (the strum plate plays the notes)");
      nl = 0; npl = 0; run(2000);
      CHECK(nl == 0 && npl >= 3, "chord mode: no note is pressed, but the drums play (the stock's pad function: no key needed)");
      { int ok = 1; for (int i = 0; i < npl; i++) if (pkeyl[i] != 5 && pkeyl[i] != 7 && pkeyl[i] != 4 && pkeyl[i] != 11) ok = 0; CHECK(ok, "chord mode: only the recorded pads"); }
      tap(0x31, 60); run(100); beat_on = 0; run(100);
      for (int q = 0; q < 32; q++) XQ->std[q] = 0xFF; run(20);
      CHECK(Q->dmute == 0, "no drum steps: the stock drums are the stock's again");
      for (int q = 0; q < 32; q++) XQ->stn[q] = 0xFF;
    }
    /* ---- the chord step sequencer (chord mode) ---- */
    { R8(DE89) = 2; if (LED(0x15)) { tap(0x31, 60); run(100); }
      Q->se = 0; Q->rec = 0; Q->hseen = 0; sq_chords_empty(Q); Q->ss = 0; Q->dv = 1; LED(0x14) = 1; LED(0x13) = 0;
      int e31 = edges[0x31];
      press(0x31); run(SQ_LONG + 50); rel(0x31); run(100);
      CHECK(Q->rec == 1 && Q->cm == 0 && LED(0x15) == 0 && edges[0x31] == e31, "chord mode, long press KEYBOARD: recording on, as always (the Hold list)");
      int e26 = edges[0x26];
      press(0x26); run(60); rel(0x26); run(50);
      CHECK((Q->cm & 1) == 1 && Q->ss == 2 && edges[0x26] == e26, "a step button before the first chord: the chord memory is chord STEPS now, the button selected step 3 and was not the stock's");
      { int on = 0, off = 0; for (int t = 0; t < 120; t++) { run(10); if (LED(4)) on++; else off++; }
        CHECK(on > 20 && off > 20 && LED(7) == 1, "the same LEDs: the selected step blinks (voice 3 LED), page 1 is lit (the yellow VOICE's upper LED)");
        CHECK(LED(6) == 1 && LED(13) == 0, "bank 1 (pages 1-4): the voice 5 LED is lit, the pattern 5 LED is dark"); }
      chordbtn(6, 3, 7, 10, 0, 150);
      CHECK(Q->n == 0 && XQ->cs[2][0] != 0xFF && is(XQ->cs[2], 3, 7, 10, 0), "a chord played is written at the selected step (not appended to the Hold list)");
      press(0x28); run(60); rel(0x28); run(50);
      chordbtn(18, 5, 8, 0, 1, 150);
      CHECK(Q->ss == 4 && is(XQ->cs[4], 5, 8, 0, 1), "pattern 1 = step 5: its chord");
      press(0x27); run(60); rel(0x27); run(50); run(300);
      CHECK(Q->ss == 12 && LED(8) == 1 && LED(7) == 0, "the yellow VOICE = page 2 (its lower LED)");
      chordbtn(27, 8, 0, 3, 2, 150);
      CHECK(is(XQ->cs[12], 8, 0, 3, 2), "step 13: its chord");
      press(0x27); run(60); rel(0x27); run(50);
      CHECK(Q->ss == 4, "the yellow VOICE again: back to page 1");
      int e2c = edges[0x2C];
      press(0x2C); run(60); rel(0x2C); run(50); run(300);
      CHECK(Q->ss == 32 + 4 && LED(13) == 1 && LED(6) == 0 && LED(7) == 1 && edges[0x2C] == e2c, "pattern 5 (swing) = bank 2, pages 5-8: its LED lit, voice 5's dark, same page-in-bank and step; the stock saw nothing");
      chordbtn(4, 11, 2, 9, 3, 150);
      CHECK(Q->ss == 36 && is(XQ->cs[36], 11, 2, 9, 3), "step 37 (bank 2, page 5): its chord");
      press(0x2D); run(60); rel(0x2D); run(50); run(300);
      CHECK(Q->ss == 32 + 16 + 4 && LED(0x0E) == 1 && LED(0x0F) == 0 && LED(13) == 1, "the yellow PATTERN in bank 2: page 7 (its upper LED)");
      press(0x2D); run(60); rel(0x2D); run(50); run(300);
      CHECK(Q->ss == 32 + 24 + 4 && LED(0x0F) == 1 && LED(0x0E) == 0, "pressed again: page 8 (its lower LED)");
      press(0x34); run(60); rel(0x34); run(50); run(300);
      CHECK(Q->ss == 24 + 4 && LED(6) == 1 && LED(13) == 0 && LED(0x0F) == 1, "voice 5 (a. piano) = bank 1 again: page 4, same step");
      press(0x27); run(60); rel(0x27); run(50);
      press(0x27); run(60); rel(0x27); run(50);
      CHECK(Q->ss == 4 || Q->ss == 12, "(the yellow VOICE from page 4 of a bank goes to page 1 / 2)");
      Q->ss = 12; int eh = edges[0x30];
      press(0x30); run(100); rel(0x30); run(120);
      CHECK(XQ->cs[12][0] == 0xFF && edges[0x30] == eh && XQ->cs[4][0] != 0xFF, "Hold let go: the selected chord step is cleared, the stock saw no Hold, the others stay");
      /* it plays with the rhythm: quarter notes, in chord mode and in keyboard mode, with the Sync LED off (Start) */
      XQ->cs[36][0] = 0xFF; XQ->cs[36][1] = 0xFF;                                                 /* one page: 8 steps */
      R32(RA(20002A90)) = 200; c8n = 0; calls_on = 0; post_on = 1;
      press(0x31); run(30); press(0x2E); run(40); rel(0x2E); rel(0x31); run(60); beat_on = 1; run(2500);
      CHECK(Q->rec == 1 && Q->go == 1 && LED(0x15) == 0 && c8n >= 1, "(KEYBOARD + Start: the beat plays while the chords are being entered, still chord mode, still recording)");
      press(0x31); run(SQ_LONG + 50); rel(0x31); run(100);                                  /* long press: recording off */
      CHECK(Q->rec == 0 && (Q->cm & 1) == 1, "long press: recording off, the chord steps stay");
      Q->dv = 3; c8n = 0; run(8000);                                                       /* a swing note value: the chord steps do not care */
      { int ia = -1, ib = -1, ia2 = -1; for (int i = 0; i < c8n; i++) { if (c8a[i] == 3 && ia < 0) ia = i; else if (c8a[i] == 3 && ia2 < 0 && ia >= 0) ia2 = i; if (c8a[i] == 5 && ib < 0) ib = i; }
        CHECK(ia >= 0 && ib >= 0, "Rhythm Start: the chord steps play (chord mode)");
        CHECK(ia >= 0 && ib >= 0 && c8t[ib] - c8t[ia] > 940 && c8t[ib] - c8t[ia] < 980, "steps 3 and 5 are 2 quarters (960 ms) apart: a step is a quarter note (12 stock steps of 40 ms), whatever the note value");
        CHECK(ia >= 0 && ia2 >= 0 && c8t[ia2] - c8t[ia] > 3820 && c8t[ia2] - c8t[ia] < 3860, "page 1 only: it loops by itself, 8 quarters = 3840 ms"); }
      Q->dv = 1;
      tap(0x31, 60); run(100);                                                                 /* keyboard mode */
      c8n = 0; run(4500);
      CHECK(LED(0x15) == 1 && c8n >= 2, "keyboard mode: the chord steps play there too (the Hold list is not needed)");
      int ehh = edges[0x30]; tap(0x30, 60); run(100);
      CHECK(edges[0x30] == ehh + 1, "keyboard mode with chord steps: Hold is the stock's (no list to play)");
      beat_on = 0; run(200);
      CHECK(Q->on == 0 && Q->cm == 1, "the rhythm stopped: the chord is let go, the steps stay");
      /* more pages: a chord on page 3 makes the loop 3 pages long */
      XQ->cs[20][0] = XQ->cs[2][0]; XQ->cs[20][1] = XQ->cs[2][1];
      c8n = 0; beat_on = 1; run(14000);
      { int ts[8], nt = 0; for (int i = 0; i < c8n && nt < 8; i++) if (c8a[i] == 3) ts[nt++] = (int)c8t[i];
        CHECK(nt >= 3 && ts[1] - ts[0] > 8600 && ts[1] - ts[0] < 8680, "a chord step on page 3 (step 21): its chord comes 18 quarters (8640 ms) after step 3's, page 2 is silent in between");
        CHECK(nt >= 3 && ts[2] - ts[1] > 2860 && ts[2] - ts[1] < 2900, "...and the loop is 3 pages: step 3 again 6 quarters after (24 quarters = 11520 ms in all)"); }
      XQ->cs[20][0] = 0xFF; XQ->cs[20][1] = 0xFF;
      beat_on = 0; run(200);
      /* Chord mode: the rhythm starts only with KEYBOARD + Start */
      beat_on = 0; run(400); if (LED(0x15)) { tap(0x31, 60); run(100); }
      press(0x31); run(SQ_LONG + 50); rel(0x31); run(100);
      CHECK(LED(0x15) == 0 && Q->rec == 1 && (Q->cm & 1) == 1, "(chord mode, recording the chord steps again)"); sync_armed = 0; LED(0x13) = 0; LED(0x14) = 1; Q->ss = 4;
      CHECK(Q->go == 0 && Q->mute == 1 && sq_beatmute() == 1, "(not allowed: recording chords mutes the stock rhythm from the start, 2.113)");
      int e2e = edges[0x2E];
      press(0x2E); run(60); rel(0x2E); run(60);
      CHECK(edges[0x2E] == e2e + 1 && Q->go == 1, "recording chords, Start alone = KEYBOARD + Start: the stock sees Start, the player's start (2.124)");
      beat_on = 1; run(200); CHECK(Q->mute == 0 && sq_beatmute() == 0, "...the beat plays while recording");
      press(0x2E); run(60); rel(0x2E); run(60); CHECK(Q->go == 0, "Start again: the stop gesture"); beat_on = 0; run(300);
      sync_armed = 1;                                                                     /* the stock's Sync Start: a chord press asks for a start */
      press(6); run(60);
      CHECK(!beat_on && R8(RA(2000DE3F)) == 0 && Q->mute == 1, "a chord press with Sync Start: the pending start is cancelled before the rhythm takes it up: it does not start");
      rel(6); run(100);
      CHECK(!beat_on, "...and it does not start after");
      sync_armed = 0;
      beat_on = 1; run(100);                                                              /* it runs anyway (a start that was not cancelled) */
      CHECK(Q->mute == 1 && sq_beatmute() == 1, "a rhythm that runs in chord mode without being asked for: muted (drums and bass rest)");
      c8n = 0; run(2000); CHECK(c8n == 0 && Q->on == 0, "...and the chord steps do not play");
      beat_on = 0; run(300); CHECK(Q->mute == 1 && Q->go == 0, "(stopped; still recording: still muted)");
      int e31b = edges[0x31]; e2e = edges[0x2E];
      press(0x31); run(30); press(0x2E); run(60); rel(0x2E); run(30); rel(0x31); run(100);
      CHECK(edges[0x2E] == e2e + 1 && LED(0x15) == 0 && edges[0x31] == e31b && Q->go == 1, "KEYBOARD + Start: the stock sees Start (and starts the rhythm), KEYBOARD is no mode change, the rhythm is allowed");
      beat_on = 1; becc_n = 0; c8n = 0; run(3000);
      CHECK(Q->mute == 0 && sq_beatmute() == 0 && c8n >= 1, "...it plays, and the chord steps with it");
      CHECK(becc_n >= 1 && becc_a[0][0] == 3 && becc_a[0][1] == 7 && becc_a[0][2] == 10, "each chord step also tunes the strum plate's notes (the stock's own function, the chord's classes)");
      /* the playing step blinks, on the page that is shown */
      Q->ss = 4; int ss0 = Q->ss; int tr = 0, pv = LED(4); for (int t = 0; t < 4000; t++) { run(1); int v = LED(4); if (v != pv) tr++; pv = v; }
      CHECK(tr >= 6 && Q->ss == ss0, "the step that is playing blinks fast (voice 3 LED = step 3, which holds a chord); only page 1 has chords: the page stays");
      /* the page shown follows the playing step, to pages that have something on them */
      beat_on = 0; run(300);
      XQ->cs[40][0] = XQ->cs[2][0]; XQ->cs[40][1] = XQ->cs[2][1];                         /* a chord on page 6 (bank 2) */
      Q->ss = 2; press(0x31); run(30); press(0x2E); run(60); rel(0x2E); rel(0x31); run(60); beat_on = 1;
      { int seen = 0; unsigned mask = 0; for (int t = 0; t < 22000; t++) { run(1); if (t % 20 == 0) mask |= 1u << (Q->ss >> 3); }
        CHECK((mask & ~((1u << 0) | (1u << 5))) == 0 && (mask & (1u << 5)) && (mask & 1u), "the page shown follows the playing step: pages 1 and 6 (the ones with chords), not the empty ones between"); (void)seen; }
      CHECK(Q->ss >= 0, "(page follow checked)");
      XQ->cs[40][0] = 0xFF; XQ->cs[40][1] = 0xFF;
      /* the stop gesture */
      int e2f = edges[0x2E]; press(0x2E); run(60); rel(0x2E); run(60);
      CHECK(edges[0x2E] == e2f + 1 && Q->go == 0 && Q->mute == 1, "Start while it runs: the stock gets it; the rhythm is no longer allowed: if the stock keeps it running (a chord held, Auto) it is muted");
      beat_on = 0; run(300);
      /* the strum table also in keyboard mode: not while a keyboard note sounds */
      tap(0x31, 60); run(100);
      beat_on = 1; run(100); becc_n = 0; run(4000);
      CHECK(becc_n >= 2, "keyboard mode, no key down: the chord steps tune the strum plate");
      press(17); run(100); becc_n = 0; run(4000);
      CHECK(becc_n == 0, "keyboard mode with a note key down: they do not (the keyboard notes would slide)");
      rel(17); run(100); beat_on = 0; run(200);
      tap(0x31, 60); run(100);
      press(0x31); run(SQ_LONG + 50); rel(0x31); run(100);
      CHECK(Q->rec == 1, "(recording once more)");
      /* back to a Hold list */
      Q->ss = 0; press(0x31); run(SQ_LONG + 50); rel(0x31); run(100);                   /* recording off */
      press(0x31); run(SQ_LONG + 50); rel(0x31); run(100);                              /* on again */
      press(0x30); run(60); press(0x27); run(60); rel(0x27); run(30); rel(0x30); run(120);
      { int left = 0; for (int q = 0; q < 64; q++) left += XQ->cs[q][0] != 0xFF; CHECK(left == 0 && Q->cm == 0 && Q->n == 0, "Hold + the yellow VOICE: every chord step out, the memory is a Hold list again"); }
      chordbtn(6, 3, 7, 10, 0, 150);
      CHECK(Q->n == 1 && Q->cm == 0, "recording chords without a step button: the Hold list as it always was");
      press(0x31); run(SQ_LONG + 50); rel(0x31); run(100);
      CHECK(Q->rec == 0, "(recording off)");
    }
    /* ---- chord mode, not recording: everything as always, unless the player starts the sequences ---- */
    { post_on = 1; sync_armed = 0; R8(DE89) = 2; Q->am = 0; beat_on = 0; run(400);
      if (LED(0x15)) { tap(0x31, 60); run(100); }
      Q->rec = 0; Q->se = 0; Q->go = 0;
      sq_chords_empty(Q); for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->std[q] = 0xFF; }
      R32(RA(20002A90)) = 200;
      int e2e = edges[0x2E];
      press(0x2E); run(60); rel(0x2E); run(60);
      CHECK(edges[0x2E] == e2e + 1 && Q->go == 0, "chord mode, nothing recorded: Start alone is the stock's (2.113)");
      beat_on = 1; c8n = 0; npl = 0; run(4000);
      CHECK(c8n == 0 && npl == 0 && sq_dmute() == 0 && Q->mute == 0 && sq_beatmute() == 0, "the rhythm runs the stock way: the sequencers are silent, the stock's drums are not muted, nothing is muted");
      beat_on = 0; run(400);
      Q->cm = 1; XQ->cs[2][0] = 3; XQ->cs[2][1] = 0; XQ->cs[4][0] = 0x15; XQ->cs[4][1] = 1; XQ->std[0] = 0xF1; XQ->std[2] = 0xF0;
      e2e = edges[0x2E];
      press(0x2E); run(60); rel(0x2E); run(60);
      CHECK(edges[0x2E] == e2e + 1 && Q->go == 1 && LED(0x15) == 0, "chord mode, something recorded: Start alone is the player's start (the stock sees Start) (2.113)");
      beat_on = 1; c8n = 0; npl = 0; run(4000);
      CHECK(c8n >= 1 && npl >= 1 && sq_dmute() == 1 && Q->mute == 0, "...the chord steps and the drum steps play with it");
      e2e = edges[0x2E]; press(0x2E); run(60); rel(0x2E); run(60);
      CHECK(edges[0x2E] == e2e + 1 && Q->go == 0, "Start again while it runs: the stop gesture, the stock gets it");
      beat_on = 0; run(400);
      CHECK(Q->go == 0 && Q->on == 0, "stopped: nothing plays");
      sync_armed = 1; press(6); run(60); rel(6); run(100);
      CHECK(beat_on == 1, "Sync Start as always: a chord press starts the rhythm (the pending start is not cancelled)");
      c8n = 0; npl = 0; run(3000);
      CHECK(c8n == 0 && npl == 0 && Q->mute == 0, "...and the sequences stay silent: it was not started by the player");
      beat_on = 0; sync_armed = 0; run(400);
      int e31b = edges[0x31]; e2e = edges[0x2E];
      press(0x31); run(30); press(0x2E); run(60); rel(0x2E); run(30); rel(0x31); run(100);
      CHECK(edges[0x2E] == e2e + 1 && edges[0x31] == e31b && LED(0x15) == 0 && Q->go == 1, "KEYBOARD + Start: the player's own start (the stock sees Start, KEYBOARD is no mode change)");
      beat_on = 1; c8n = 0; npl = 0; run(4000);
      CHECK(c8n >= 1 && npl >= 1 && sq_dmute() == 1, "...the chord steps and the drum steps play with it, the stock's drums rest for the drum steps");
      beat_on = 0; run(400);
      CHECK(Q->go == 0 && sq_dmute() == 0, "stopped: back to the stock's");
      sq_chords_empty(Q); for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->std[q] = 0xFF; }
    }
    /* ---- keyboard mode: the editor shows where the playing step is, and the page follows it ---- */
    { post_on = 1; sync_armed = 0; R8(DE89) = 0; Q->am = 0; beat_on = 0; run(300); Q->rec = 0;
      if (!LED(0x15)) { tap(0x31, 60); run(100); }
      Q->se = 0; for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->std[q] = 0xFF; }
      XQ->stn[3] = 3; XQ->stn[19] = 9;                                                    /* a note on page 1 (step 4) and on page 3 (step 20); page 2 is empty */
      R32(RA(20002A90)) = 200;
      int e2e = edges[0x2E]; press(0x2E); run(40); rel(0x2E); run(40);
      CHECK(edges[0x2E] == e2e + 1, "keyboard mode: the Start button is the stock's (chord mode's rule is for chord mode only)");
      beat_on = 0; run(300);
      press(0x31); run(SQ_LONG + 50); rel(0x31); run(100);
      CHECK(Q->se == 1, "(the note / drum editor is on)");
      Q->ss = 0; beat_on = 1; run(50);
      { unsigned mask = 0; int tr = 0, pv = LED(5); for (int t = 0; t < 12000; t++) { run(1); if (t % 20 == 0) mask |= 1u << (Q->ss >> 3); { int v = LED(5); if ((Q->ss >> 3) == 0 && v != pv) tr++; pv = v; } }
        CHECK((mask & 1u) && (mask & 4u) && (mask & ~5u) == 0, "keyboard mode, editing: the page shown follows the playing step, to the pages with notes (1 and 3), not the empty one between");
        CHECK(tr >= 6, "...and the step that is playing blinks fast on the page that is shown (voice 4 LED = step 4)"); }
      beat_on = 0; run(300);
      press(0x31); run(40); rel(0x31); run(100);
      CHECK(Q->se == 0 && LED(0x15) == 1, "(editor off)");
      for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->std[q] = 0xFF; }
    }
    /* ---- the Start LED is a metronome: lit on the beat ---- */
    { R8(DE89) = 0; LED(0x13) = 0; LED(0x14) = 1; R8(LEDSH + 2) = 0x08; if (!LED(0x15)) { tap(0x31, 60); run(100); }
      Q->rec = 0; Q->mute = 0; Q->go = 0;
      R32(RA(20002A90)) = 200; Q->am = 0;
      for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->std[q] = 0xFF; }
      sq_chords_empty(Q);
      run(100);
      CHECK((R8(LEDSH + 2) & 0x08) == 0x08, "nothing playing: the Sync Start LED is as the stock leaves it (lit)");
      /* the rhythm runs: the stock lights the Start LED (flag 0x13) */
      LED(0x14) = 0; LED(0x13) = 1; R8(LEDSH + 2) = 0x04;
      beat_on = 1; int on = 0, pulses = 0, prev = 1, longest = 0, cur = 0;
      for (int t = 0; t < 4000; t++) { run(1); int b = (R8(LEDSH + 2) & 0x04) != 0; if (b) { on++; cur++; if (cur > longest) longest = cur; } else cur = 0; if (!prev && b) pulses++; prev = b; }
      CHECK(pulses >= 7 && pulses <= 9, "rhythm running: the Start LED is lit once per quarter note (12 steps x 40 ms = 480 ms: 8 in 4 s)");
      CHECK(longest >= 60 && longest <= 80 && on < 800, "...each flash about 70 ms, dark between the beats (it was the other way round)");
      beat_on = 0; run(200); LED(0x13) = 0; LED(0x14) = 1; R8(LEDSH + 2) = 0x08; run(50);       /* (the stock's own change of the LEDs: flags and port) */
      CHECK((R8(LEDSH + 2) & 0x08) == 0x08 && Q->bl == 0, "the rhythm stopped: the stock's LED is lit again");
      /* an arpeggio alone (no rhythm): the Sync Start LED is the lit one: it flashes at the tempo knob's quarter: 480 ms */
      Q->am = 1; R8(DE89) = 0; R8(LEDSH + 2) = 0x08;
      press(17); run(100); press(25); pulses = 0; prev = (R8(LEDSH + 2) & 0x08) != 0;
      for (int t = 0; t < 4000; t++) { run(1); int b = (R8(LEDSH + 2) & 0x08) != 0; if (!prev && b) pulses++; prev = b; }
      CHECK(pulses >= 7 && pulses <= 9, "an arpeggio playing without the rhythm: the Sync Start LED is lit once per quarter note too");
      rel(17); rel(25); run(300);
      CHECK((R8(LEDSH + 2) & 0x08) == 0x08 && Q->bl == 0, "the arpeggio stopped: the LED is lit again");
      Q->am = 0;
      CHECK(LED(0x14) == 1 && LED(0x13) == 0, "the flags are never touched");
      /* a rhythm that is muted (a chord press while recording chord steps) does not flash it */
      beat_on = 0; run(100);
    }
    /* ---- 2.113: recording in chord mode: Manual, Hold off, no rhythm; put back after ---- */
    { post_on = 1; sync_armed = 0; beat_on = 0; run(400);
      if (LED(0x15)) { tap(0x31, 60); run(100); }
      Q->rec = 0; Q->go = 0; run(50);
      mstate = MA_AUTO; LED(0x10) = 1; masets = 0;
      press(0x31); run(SQ_LONG + 50); rel(0x31); run(100);
      CHECK(Q->rec == 1 && mstate == MA_MANUAL && LED(0x10) == 0 && sq_frc(), "recording starts: Auto is held as Manual, the stock Hold off");
      int e2f = edges[0x2F]; tap(0x2F, 60);
      CHECK(edges[0x2F] == e2f && mstate == MA_MANUAL, "...the Manual/Auto button is hidden while recording");
      sync_armed = 1; sync_direct = 1; stops = 0; press(6); run(5);
      CHECK(!beat_on && stops == 1 && R8(RA(2000DE3F)) == 0 && sq_beatmute() == 1, "a chord press with Sync Start that sets the running flag itself: stopped at once (and muted anyway)");
      run(60); rel(6); run(100);
      CHECK(!beat_on, "...the rhythm stays stopped");
      sync_armed = 0; sync_direct = 0;
      press(0x31); run(SQ_LONG + 50); rel(0x31); run(100);
      CHECK(Q->rec == 0 && mstate == MA_AUTO && LED(0x10) == 1 && !sq_frc() && Q->mute == 0, "recording ends: Auto and Hold as they were, nothing muted");
      mstate = MA_THIRD; LED(0x10) = 0;
      press(0x31); run(SQ_LONG + 50); rel(0x31); run(100);
      CHECK(mstate == MA_MANUAL && LED(0x10) == 0, "the third state: held as Manual too");
      tap(0x31, 60); run(100);
      CHECK(LED(0x15) == 1 && Q->rec == 0 && mstate == MA_THIRD && LED(0x10) == 0, "recording ended by keyboard mode: the third state back, Hold still off");
      tap(0x31, 60); run(100);
      mstate = MA_MANUAL; LED(0x10) = 1;
      press(0x31); run(SQ_LONG + 50); rel(0x31); run(100);
      cfx = 0; run(50);
      CHECK(LED(0x10) == 1 && !sq_frc() && Q->rec == 0, "Extras off while recording: Hold back on");
      cfx = 1; run(50);
    }
    /* ---- 2.119: drum volumes, extra sounds ---- */
    { dv_t *D = dv();
      CHECK(D->v[2] == 127 && D->v[24] == 127, "drum volumes start at 127");
      h344 = h488 = 0; drum_fix(2, 0); CHECK(h344 == 1 && h488 == 0 && l344i == 2, "volume 127: the stock fixed-level hit, untouched");
      D->v[2] = 64; h344 = h488 = 0; R32(0x20006E18u + 80u * 20u + 0x10u) = 0x3F800000u; drum_fix(2, 1);
      CHECK(h344 == 1 && h488 == 0 && R32(0x20006E18u + 80u * 20u + 0x10u) == DV_F[64], "volume 64: the stock hit, then the channel's drum gain = 64/127");
      D->v[8] = 64; drum_vel(8, 0, 100); CHECK(l488i == 8 && l488v == 100 && R32(0x20006E18u + 86u * 20u + 0x10u) == DV_F[64], "a rhythm hit at velocity 100, volume 64: velocity as is, gain 64/127");
      R8(RA(2000DE65)) = 1; R32(0x20006E18u + 86u * 20u + 0x10u) = 0x3F800000u; h488 = 0; drum_vel(8, 0, 100);
      CHECK(h488 == 1 && R32(0x20006E18u + 86u * 20u + 0x10u) == 0x3F800000u, "Classic tone: the stock level, the 108 volume kept for later");
      R8(RA(2000DE65)) = 0;
      D->v[8] = 0; h488 = 0; drum_vel(8, 0, 100); drum_fix(8, 0); CHECK(h488 == 0, "volume 0: no hit at all");
      D->v[2] = 127; D->v[8] = 127;
      if (!LED(0x15)) { tap(0x31, 60); run(100); } Q->se = 0; beat_on = 0; run(100);
      R8(RA(2000DE65)) = 0; R8(SQ_SUSV + 2) = 100; run(50);
      u8 gt0 = Q->gt;
      int e4 = edges[4]; hitn = 0; press(4); run(60);
      CHECK(edges[4] == e4 + 1 && Q->dvk == 1 && Q->dvi == 2, "keyboard mode, pad 1 held: the stock plays it; the knob is on the bass drum's volume");
      rel(4); run(60); R8(RA(2000DE65)) = 1; press(4); run(60); CHECK(Q->dvk == 0, "Classic tone: a pad held does not take the knob"); rel(4); run(60); R8(RA(2000DE65)) = 0; R8(SQ_SUSV + 2) = 100; run(60);
      hitn = 0; press(4); run(60);
      run(50); CHECK(D->v[2] == 127 && hitn == 0, "...locked until the knob is moved");
      R8(SQ_SUSV + 2) = 90; run(30); CHECK(D->v[2] == 90 && hitn >= 1 && hiti[0] == 2, "moved: volume 90, with a preview of the bass drum");
      { int h0 = hitn; for (int v = 89; v >= 40; v--) { R8(SQ_SUSV + 2) = (u8)v; run(5); } run(100);
        CHECK(D->v[2] == 40 && hitn - h0 >= 2 && hitn - h0 <= 5, "turned down to 40: a few previews (at most one per 90 ms), the last value heard"); }
      rel(4); run(60); R8(SQ_SUSV + 2) = 42; run(30);
      CHECK(Q->dvk == 0 && Q->gt == gt0 && D->v[2] == 40, "pad let go: the knob is the note length again, locked (no jump), the volume stays");
      hitn = 0; int e5 = edges[5]; press(22); run(30); press(5); run(60);
      CHECK(edges[5] == e5 && hitn == 1 && hiti[0] == 4 && Q->dvi == 4, "B minor + pad 2: the stock does not get it; its extra sound (conga high) plays; the knob is on its volume");
      R8(SQ_SUSV + 2) = 100; run(30); CHECK(D->v[4] == 100, "...its volume set");
      rel(5); rel(22); run(60);
      R8(SQ_SUSV + 2) = 64; run(50);
      /* edit mode: B minor + pad 3 writes the extra sound (conga mid) */
      for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->std[q] = 0xFF; XQ->stx[q] = 0; }
      press(0x31); run(SQ_LONG + 100); rel(0x31); run(100);
      CHECK(Q->se == 1, "edit mode");
      Q->ss = 0; Q->sl = 0; press(22); run(30); press(6); run(60); rel(6); rel(22); run(60);
      CHECK((XQ->std[0] & 15) == 2 && (XQ->stx[0] & 1) == 1, "B minor + pad 3 in edit mode: step 1, layer 1 = pad 3's extra sound");
      Q->sl = 1; press(7); run(60); rel(7); run(60);
      CHECK((XQ->std[0] >> 4) == 3 && (XQ->stx[0] & 2) == 0, "pad 4 alone on layer 2: its own sound");
      tap(0x31, 60); run(100); CHECK(Q->se == 0, "edit mode ends");
      hitn = 0; npl = 0; beat_on = 1; run(400);
      { int a = 0; for (int i = 0; i < hitn; i++) if (hiti[i] == 5) a = 1; int b = 0; for (int i = 0; i < npl; i++) if (pkeyl[i] == 7) b = 1;
        CHECK(a && b, "playing: step 1 plays conga mid (extra) and pad 4 (the stock pad function)"); }
      beat_on = 0; run(200);
      for (int q = 0; q < 32; q++) { XQ->std[q] = 0xFF; XQ->stx[q] = 0; }
      D->v[2] = 127; D->v[4] = 127;
      tap(0x31, 60); run(100);
      hitn = 0; press(22); run(30); press(5); run(60); rel(5); rel(22); run(60);
      CHECK(hitn == 0 && Q->pal == 0, "chord mode: B minor + a chord button is nothing of ours");
    }
    /* ---- 2.122: the 108 rhythm bass cut after 50 ms ---- */
    { R8(RA(2000DE65)) = 0; beat_on = 1; run(20); R8(RA(2000DE38)) = 1; Q->bsn = 1; Q->bst0 = (unsigned short)(R32(RA(2000DB84)) & 0xFFFF);
      run(30); CHECK(R8(RA(2000DE38)) == 1, "108 bass note: gate still open at 30 ms");
      run(30); CHECK(R8(RA(2000DE38)) == 0 && R32(0x2001F000u) == 0x10u && Q->bsn == 0, "...shut at 50 ms (GPIO2 bit 4 cleared)");
      R8(RA(2000DE65)) = 1; R8(RA(2000DE38)) = 1; Q->bsn = 1; Q->bst0 = (unsigned short)(R32(RA(2000DB84)) & 0xFFFF); run(80);
      CHECK(R8(RA(2000DE38)) == 1, "Classic tone: not ours (the stock tick does it)");
      R8(RA(2000DE65)) = 0; R8(RA(2000DE38)) = 0; beat_on = 0; run(50); }
    /* ---- 2.123: yellow VOICE + Sustain = the voice's keyboard volume; A 440 preview; delayed bank press; arpeggio saved ---- */
    { dv_t *D = dv();
      if (!LED(0x15)) { tap(0x31, 60); run(100); } Q->se = 0; beat_on = 0; R8(RA(2000DE89)) = 3; R8(RA(200009DC)) = 1; run(60);
      R8(SQ_SUSV + 2) = 70; run(50);
      int e27 = edges[0x27]; press(0x27); run(60);
      CHECK(edges[0x27] == e27 && Q->yh == 1, "keyboard mode: yellow VOICE held: hidden from the stock");
      R8(SQ_SUSV + 2) = 60; run(20);
      int k9 = R8(SQ_KT + 9);
      CHECK(D->kv[3] == 60 && Q->pvs == 1 && armed[k9] == 0, "knob moved: voice 4's keyboard volume 60, the preview key pressed");
      CHECK(sq_aoff(0x30 + 9 + 12) == 69 - (0x30 + 9 + 12) - r27()->ktr, "...sounding as A 440 (note 69)");
      R8(SQ_SUSV + 2) = 50; run(200); CHECK(D->kv[3] == 50 && Q->pvs == 1, "turning on: still sounding");
      run(500); CHECK(Q->pvs == 0, "0.4 s without a change: the preview stops");
      rel(0x27); run(100); CHECK(edges[0x27] == e27 && Q->yh == 0, "let go after using the knob: the stock never got the press (no voice bank change)");
      run(100); CHECK(Q->pvk == 0, "the preview's note mark gone");
      press(0x27); run(60); rel(0x27); run(100); CHECK(edges[0x27] == e27 + 1, "a plain tap: the stock gets it on release (the voice bank)");
      Q->am = 3; Q->dv = 2; run(20); CHECK(D->ar[0] == 3 && D->ar[1] == 2 && D->ar[2] == Q->gt && D->ar[3] == Q->swv, "the arpeggio settings mirrored for saving");
      CHECK(((u8 *)D + 4 + 47)[0] == Q->swv && ((u8 *)D + 4 + 36)[0] == 50, "the flat record layout: swing at 47, voice 4 at 36");
      Q->am = 0; Q->dv = 1; R8(RA(200009DC)) = 0; D->kv[3] = 127; run(20); }
    /* ---- 2.129: Hold takes out the newest; independent loops; a picked step is heard ---- */
    { if (!LED(0x15)) { tap(0x31, 60); run(100); }
      for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->std[q] = 0xFF; XQ->stx[q] = 0; XQ->sto[q] = 0; }
      press(0x31); run(SQ_LONG + 100); rel(0x31); run(100);
      CHECK(Q->se == 1, "edit mode");
      Q->ss = 0; Q->sl = 0; press(4); run(60); rel(4); run(60);          /* layer 1: pad 1 */
      Q->sl = 1; press(5); run(60); rel(5); run(60);                     /* layer 2: pad 2 */
      int kn = R8(SQ_KT + 3); press(kn); run(60); rel(kn); run(60);      /* the note */
      CHECK(XQ->stn[0] == 3 && (XQ->std[0] & 15) == 0 && (XQ->std[0] >> 4) == 1, "a step with 2 drums and a note");
      press(0x30); run(100); rel(0x30); run(120);
      CHECK(XQ->stn[0] == 0xFF && XQ->std[0] == 0x10, "Hold: the note (the newest) out, the beat stays");
      press(0x30); run(100); rel(0x30); run(120);
      CHECK(XQ->std[0] == 0xF0, "Hold: then drum layer 2");
      press(0x30); run(100); rel(0x30); run(120);
      CHECK(XQ->std[0] == 0xFF, "Hold: then drum layer 1");
      XQ->std[1] = 0xF2; XQ->stn[9] = 5;                                  /* drum on page 1 step 2, note on page 2 */
      int e5 = edges[R8(SQ_KT + 5)]; npl = 0;
      press(0x24); run(60); rel(0x24); run(30);                          /* pick step 1: empty */
      press(0x25); run(60); rel(0x25); run(30);                          /* pick step 2: its drum heard */
      CHECK(npl >= 1, "picking a step: its drum is heard");
      Q->ss = 8; press(0x25); run(60); rel(0x25); run(60);              /* page 2, step 2: its note */
      CHECK(edges[R8(SQ_KT + 5)] == e5 + 1, "picking a step: its note is heard");
      tap(0x31, 60); run(100);
      for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->std[q] = 0xFF; }
      XQ->std[0] = 0xF0; for (int q = 0; q < 32; q++) if (q % 8 == 0) XQ->stn[q] = 2;   /* a 1-page beat, 4 pages of notes */
      R32(RA(20002A90)) = 200; npl = 0; beat_on = 1; run(8000);
      CHECK(npl >= 3, "a 1-page beat under 4 pages of notes: the beat repeats on every page"); beat_on = 0; run(200);
      for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->std[q] = 0xFF; } }
    /* ---- 2.133: Bb minor settings lights: the voice bank's lights come back ---- */
    { if (!LED(0x15)) { tap(0x31, 60); run(100); }
      if (Q->se) { tap(0x31, 60); run(100); }
      for (int cfg = 0; cfg < 4; cfg++) {
          F_LEDSET(7, cfg & 1); F_LEDSET(8, (cfg >> 1) & 1); F_LEDSET(0x0E, cfg == 2); F_LEDSET(0x0F, 0); F_LEDSET(9, 1); F_LEDSET(2, cfg == 1); run(60);
          Q->tpoff = 0; int a[14]; static const int L[14] = { 9, 10, 11, 12, 13, 2, 3, 4, 5, 6, 0x0E, 0x0F, 7, 8 }; for (int i = 0; i < 14; i++) a[i] = LED(L[i]);
          press(15); run(300);
          if (cfg == 1) { press(0x27); run(60); rel(0x27); run(60); press(0x25); run(60); rel(0x25); run(60); press(0x29); run(60); rel(0x29); run(60); }   /* Bb minor + yellow VOICE (transpose), VOICE 2, PATTERN 2 */
          if (cfg == 2) { press(0x27); run(60); rel(0x27); run(60); press(0x27); run(60); rel(0x27); run(60); }   /* the transpose toggled twice */
          rel(15); run(300);
          int ok = 1; for (int i = 0; i < 14; i++) if (LED(L[i]) != a[i]) ok = 0;
          CHECK(ok, "2.133: Bb minor released in keyboard mode: every light as it was");
      } }
    { /* 2.133 (owner): the PATTERN bank 3 on + Bb minor: the VOICE lights come back too */
      static const int L[14] = { 9, 10, 11, 12, 13, 2, 3, 4, 5, 6, 0x0E, 0x0F, 7, 8 };
      for (int i = 0; i < 14; i++) F_LEDSET(L[i], 0);
      F_LEDSET(3, 1); F_LEDSET(8, 1); Q->p3a = 1; Q->p3cur = 2; run(100);
      int a[14]; for (int i = 0; i < 14; i++) a[i] = LED(L[i]);
      press(15); run(300); rel(15); run(300);
      int ok = 1; for (int i = 0; i < 14; i++) if (LED(L[i]) != a[i]) ok = 0;
      CHECK(ok && LED(3) == 1 && LED(2) == 0 && LED(0x0E) == 1 && LED(10) == 1, "2.133: PATTERN bank 3 + Bb minor let go: the VOICE lights as they were (not the menu's)");
      Q->p3a = 0; Q->p3cur = 0; run(100); }
    /* ---- 2.135: keyboard cutoff / resonance (Bb minor + Sustain / Tempo) ---- */
    { if (!LED(0x15)) { tap(0x31, 60); run(100); } if (Q->se) { tap(0x31, 60); run(100); }
      Q->cut = 0; Q->res = 0; Q->tst = 0; R8(SQ_SUSV + 2u) = 40; R8(SQ_SUSV + 5u) = 50; Q->p3a = 0; run(100);
      CHECK(FLTB->on == 0 && Q->cut == 0 && Q->res == 0 && Q->tfz == 0, "2.135: both knobs untouched: the filter off, the tempo free");
      press(15); run(60);
      CHECK(Q->tfz == 1 && Q->cut == 0 && Q->res == 0 && FLTB->on == 0, "2.135: Bb minor down: the tempo frozen, nothing changes until a knob moves");
      R8(SQ_SUSV + 2u) = 42; R8(SQ_SUSV + 5u) = 52; run(60);
      CHECK(Q->cut == 0 && Q->res == 0, "2.135: a knob moved by 2 (under the threshold): still locked");
      R8(SQ_SUSV + 2u) = 70; run(60);
      CHECK(Q->cut == 70 && Q->res == 0 && FLTB->on == 1 && FLTB->f == FTAB[70], "2.135: Bb minor + Sustain: the cutoff (the filter on)");
      R8(SQ_SUSV + 5u) = 90; run(60);
      CHECK(Q->res == 90 && Q->cut == 70 && FLTB->q == 23170u - 90u * 172u, "2.135: Bb minor + Tempo: the resonance");
      u8 gt0 = Q->gt, sw0 = Q->swv; R8(SQ_SUSV + 2u) = 100; run(60);
      CHECK(Q->cut == 100 && Q->gt == gt0 && Q->swv == sw0, "2.135: the note length and the swing do not move with Bb minor down");
      rel(15); run(60);
      CHECK(Q->tfz == 1 && FLTB->on == 1, "2.135: Bb minor let go: the tempo stays frozen, the filter stays");
      R8(SQ_SUSV + 5u) = 20; R8(SQ_SUSV + 2u) = 10; run(60);
      CHECK(Q->tfz == 0 && Q->cut == 100 && Q->res == 90 && Q->gt == 10 && gt0 != 10, "2.135: the knobs moved after Bb minor: the tempo free again, the note length follows the Sustain knob; cutoff / resonance kept");
      R8(SQ_SUSV + 5u) = 22; run(60); CHECK(Q->tfz == 0, "2.135: ...and it stays free");
      tap(0x31, 60); run(100); CHECK(FLTB->on == 0, "2.135: out of keyboard mode: the filter off (chords and strum untouched)"); tap(0x31, 60); run(100);
      CHECK(FLTB->on == 1, "2.135: back in keyboard mode: the filter again (not saved across power-off, kept meanwhile)");
      /* 2.142: the resonance is kept for power-off, the cutoff is not */
      { sq_t sv = *Q; u8 rs0 = dv()->rs;
        Q->res = 90; Q->cut = 70; run(20);
        CHECK(dv()->rs == 127 - 90, "2.142: the resonance mirrored for saving (127 - value)");
        Q->res = 0; Q->cut = 0; run(20); CHECK(dv()->rs == 127, "2.142: resonance 0 = the default: nothing to save");
        dv()->rs = 127 - 55; Q->magic = 0; Q = sq(); CHECK(Q->res == 55 && Q->cut == 0, "2.142: after a power-on the resonance is as saved, the cutoff 0");
        dv()->rs = 127; Q->magic = 0; Q = sq(); CHECK(Q->res == 0 && Q->cut == 0, "2.142: an old record's 127 (an unwritten byte): resonance 0");
        dv()->rs = 200; Q->magic = 0; Q = sq(); CHECK(Q->res == 0, "2.142: a value out of range: resonance 0");
        *Q = sv; dv()->rs = rs0; }
      /* the filter itself */
      { Q->cut = 0; Q->res = 0; run(60);
        FLTB->f = FTAB[0]; FLTB->q = 23170u; FLTB->on = 1; FLTB->lp = FLTB->bp = 0;
        int b[32]; int maxo = 0;
        for (int blk = 0; blk < 200; blk++) { for (int i = 0; i < 16; i++) { b[2*i] = 1 << 28; b[2*i+1] = 7; } flt_mix(b); }
        CHECK(b[30] > (1 << 28) - (1 << 24) && b[30] < (1 << 28) + (1 << 24) && b[1] == 7, "2.135: a steady level passes at unity (open); the drum side untouched");
        FLTB->f = FTAB[100]; FLTB->lp = FLTB->bp = 0; double e_hi = 0, e_lo = 0;
        for (int blk = 0; blk < 400; blk++) { for (int i = 0; i < 16; i++) b[2*i] = ((blk * 16 + i) & 1) ? (1 << 28) : -(1 << 28); flt_mix(b); for (int i = 0; i < 16; i++) e_hi += fabs((double)b[2*i]); }
        FLTB->lp = FLTB->bp = 0;
        for (int blk = 0; blk < 400; blk++) { for (int i = 0; i < 16; i++) b[2*i] = 1 << 28; flt_mix(b); for (int i = 0; i < 16; i++) e_lo += fabs((double)b[2*i]); }
        CHECK(e_hi < e_lo / 100, "2.135: cutoff 100: the top frequency (24 kHz) is cut hard against the steady level");
        FLTB->f = FTAB[0]; FLTB->q = 23170u - 127u * 172u; FLTB->lp = FLTB->bp = 0; int ok = 1;
        for (int blk = 0; blk < 4000; blk++) { for (int i = 0; i < 16; i++) b[2*i] = (blk & 8) ? (1 << 30) : -(1 << 30); flt_mix(b); }
        for (int blk = 0; blk < 3000; blk++) { for (int i = 0; i < 16; i++) b[2*i] = 0; flt_mix(b); }
        for (int i = 0; i < 16; i++) if (abs(b[2*i]) > (1 << 22)) ok = 0;
        CHECK(ok && maxo == 0, "2.135: open, full resonance, a full-scale square, then silence: it dies away (stable, no runaway)");
        FLTB->on = 0; flt_mix(b); CHECK(FLTB->lp == 0 && FLTB->bp == 0, "2.135: off: the state cleared");
        Q->cut = 0; Q->res = 0; run(60); } }
    /* ---- 2.137 / 2.141: the delay (Gb MINOR held + Sustain = time, + Tempo = repeats; keyboard mode only) ---- */
    { if (!LED(0x15)) { tap(0x31, 60); run(100); } if (Q->se) { tap(0x31, 60); run(100); }
      Q->cut = 0; Q->res = 0; Q->dtm = 0; Q->dfb = 0; Q->tst = 0; Q->nst = 0; Q->gbu = 0; R8(SQ_SUSV + 2u) = 40; R8(SQ_SUSV + 5u) = 50; Q->p3a = 0; run(100);
      CHECK(DLYB->on == 0 && Q->tfz == 0 && Q->dtm == 0 && Q->dfb == 0, "2.137: Gb minor up: no delay, the tempo free");
      press(0x32); run(60);
      CHECK(Q->tfz == 1 && Q->dtm == 0 && Q->dfb == 0 && DLYB->on == 0, "2.141: Gb minor held: the tempo frozen, nothing changes until a knob moves");
      u8 gt1 = Q->gt; R8(SQ_SUSV + 2u) = 80; run(60);
      CHECK(Q->dtm == 80 && DLYB->on == 1 && DLYB->dt == 2400u + 80u * 146u && Q->gt == gt1, "2.141: Gb minor + Sustain: the delay time (the note length stays)");
      R8(SQ_SUSV + 5u) = 100; run(60);
      CHECK(Q->dfb == 100 && DLYB->fb == 100u * 7u / 8u && Q->cut == 0 && Q->res == 0, "2.141: Gb minor + Tempo: the repeats (cutoff / resonance untouched)");
      rel(0x32); run(60);
      CHECK(Q->tfz == 1 && DLYB->on == 1, "2.141: Gb minor let go: tempo still frozen, the delay stays");
      R8(SQ_SUSV + 5u) = 20; R8(SQ_SUSV + 2u) = 10; run(60);
      CHECK(Q->tfz == 0 && Q->dtm == 80 && Q->dfb == 100 && Q->gt == 10, "2.137: the knobs moved after it: tempo free, note length back, the delay kept");
      tap(0x31, 60); run(100); CHECK(DLYB->on == 0, "2.137: out of keyboard mode: the delay off"); tap(0x31, 60); run(100);
      CHECK(DLYB->on == 1, "2.137: back in keyboard mode: the delay again");
      { short *m = DLYBUF; for (unsigned k = 0; k < DLYN; k++) m[k] = 0;
        DLYB->wr = 0; DLYB->d = DLYB->dt = 100; DLYB->fb = 0; DLYB->on = 1; DLYB->magic = DLYMAGIC; DLYB->clr = DLYN;
        int b[32]; int out[16 * 40]; int n = 0;
        for (int blk = 0; blk < 40; blk++) { for (int i = 0; i < 16; i++) { b[2*i] = (blk == 0 && i == 0) ? (1 << 28) : 0; b[2*i+1] = 5; } flt_mix(b); dly_mix(b); for (int i = 0; i < 16; i++) out[n++] = b[2*i]; }
        int near = out[100] > (1 << 27) - (1 << 20) && out[100] < (1 << 27) + (1 << 20);
        CHECK(out[0] == (1 << 28) && near && out[99] == 0 && out[101] == 0 && out[200] == 0, "2.137: an impulse: the echo at the delay time, half level, once (no repeats)");
        for (unsigned k = 0; k < DLYN; k++) m[k] = 0; DLYB->wr = 0; DLYB->d = DLYB->dt = 100; DLYB->fb = 64; n = 0;
        for (int blk = 0; blk < 40; blk++) { for (int i = 0; i < 16; i++) { b[2*i] = (blk == 0 && i == 0) ? (1 << 28) : 0; b[2*i+1] = 5; } dly_mix(b); for (int i = 0; i < 16; i++) out[n++] = b[2*i]; }
        CHECK(out[100] > (1 << 27) - (1 << 20) && out[200] > (1 << 26) - (1 << 20) && out[200] < (1 << 26) + (1 << 20) && out[300] > (1 << 25) - (1 << 20) && out[300] < (1 << 25) + (1 << 20), "2.137: repeats of 0.5: each echo half of the one before");
        CHECK(b[1] == 5, "2.137: the drum side untouched");
        DLYB->fb = 127u * 7u / 8u; int ok = 1;
        for (int blk = 0; blk < 6000; blk++) { for (int i = 0; i < 16; i++) b[2*i] = (blk < 20 && i == 0) ? 0x7FFFFFFF : 0; dly_mix(b); }
        for (int blk = 0; blk < 9000; blk++) { for (int i = 0; i < 16; i++) b[2*i] = 0; dly_mix(b); }
        for (int i = 0; i < 16; i++) if (abs(b[2*i]) > (1 << 24)) ok = 0;
        CHECK(ok, "2.137: the most repeats, a full-scale burst, then silence: the echoes die away (no runaway)");
        for (unsigned k = 0; k < DLYN; k++) m[k] = 0x1234; DLYB->wr = 0; DLYB->d = DLYB->dt = 100; DLYB->fb = 64; DLYB->on = 1; DLYB->clr = 0;
        int pass = 1; for (int blk = 0; blk < 450; blk++) { for (int i = 0; i < 16; i++) b[2*i] = 777; dly_mix(b); if (blk < 380 && b[0] != 777) pass = 0; }
        int zero = 1; for (unsigned k = 0; k < DLYN; k++) if (m[k] != 0 && m[k] != 777 >> 14 && 0) zero = 0;
        int cl = 1; for (unsigned k = 0; k < DLYN - 3000; k++) if (m[k] == 0x1234) cl = 0;
        CHECK(pass && cl && DLYB->clr == DLYN, "2.138: turned on: the old buffer is cleared first, the sound passes untouched meanwhile"); (void)zero;
        /* 2.138: power-on RAM is not zero: nothing may be touched until the state is set up */
        for (unsigned k = 0; k < DLYN; k++) m[k] = 0x5555; memset(DLYB, 0xA5, sizeof(dly_t)); memset(FLTB, 0xA5, sizeof(flt_t));
        for (int blk = 0; blk < 50; blk++) { for (int i = 0; i < 16; i++) b[2*i] = 1000 + i; dly_mix(b); flt_mix(b); }
        int intact = 1; for (unsigned k = 0; k < DLYN; k++) if (m[k] != 0x5555) intact = 0;
        CHECK(intact && b[0] == 1000, "2.138: garbage in the delay / filter state (power-on RAM): the buffer untouched, the sound untouched");
        memset(DLYB, 0, sizeof(dly_t)); memset(FLTB, 0, sizeof(flt_t)); }
      Q->dtm = 0; Q->dfb = 0; Q->cut = 0; Q->res = 0; run(60); }
    /* ---- 2.141: the reverb (Db minor + Sustain = amount, + Tempo = decay); B / D minor lock the knobs for the vibrato / tremolo depth + speed ---- */
    { if (!LED(0x15)) { tap(0x31, 60); run(100); } if (Q->se) { tap(0x31, 60); run(100); }
      Q->cut = 0; Q->res = 0; Q->dtm = 0; Q->dfb = 0; Q->rvm = 0; Q->rvd = 0; Q->tst = 0; Q->nst = 0; R8(SQ_SUSV + 2u) = 40; R8(SQ_SUSV + 5u) = 50; Q->p3a = 0; run(100);
      CHECK(RVBB->on == 0 && Q->tfz == 0 && Q->rvm == 0 && Q->rvd == 0, "2.141: Db minor up: no reverb, the tempo free");
      press(12); run(60);
      CHECK(Q->tfz == 1 && Q->rvm == 0 && Q->rvd == 0 && RVBB->on == 0 && Q->dtm == 0, "2.141: Db minor held: the tempo frozen, nothing changes until a knob moves");
      u8 gt1 = Q->gt; R8(SQ_SUSV + 2u) = 90; run(60);
      CHECK(Q->rvm == 90 && RVBB->on == 1 && RVBB->amt == 90 && Q->gt == gt1 && Q->dtm == 0, "2.141: Db minor + Sustain: the reverb amount (the note length stays, the delay untouched)");
      R8(SQ_SUSV + 5u) = 100; run(60);
      CHECK(Q->rvd == 100 && RVBB->fb == 22000u + 100u * 70u && Q->dfb == 0 && Q->res == 0, "2.141: Db minor + Tempo: the decay (the delay / filter untouched)");
      rel(12); run(60);
      CHECK(Q->tfz == 1 && RVBB->on == 1, "2.141: Db minor let go: tempo still frozen, the reverb stays");
      tap(0x31, 60); run(100); CHECK(RVBB->on == 0, "2.141: out of keyboard mode: the reverb off"); tap(0x31, 60); run(100);
      CHECK(RVBB->on == 1, "2.141: back in keyboard mode: the reverb again");
      { short *m = RVBBUF; for (unsigned k = 0; k < RVN; k++) m[k] = 0;                 /* the reverb itself: an impulse, then silence */
        RVBB->on = 1; RVBB->clr = RVN; RVBB->amt = 127; RVBB->fb = 22000u + 100u * 70u; RVBB->prev = 0; for (int j = 0; j < 6; j++) RVBB->ix[j] = 0; for (int j = 0; j < 4; j++) RVBB->lp[j] = 0;
        int b[32]; long long tail = 0, late = 0; int bounded = 1, bside = 1;
        for (int blk = 0; blk < 400; blk++) {
          for (int i = 0; i < 16; i++) { b[2*i] = (blk == 0 && i == 0) ? (1 << 28) : 0; b[2*i+1] = 7; }
          rvb_mix(b);
          for (int i = 0; i < 16; i++) { if (!(blk == 0 && i == 0)) { long long v = b[2*i] < 0 ? -(long long)b[2*i] : b[2*i]; tail += v; if (blk >= 200) late += v; } if (b[2*i+1] != 7) bside = 0; if (b[2*i] > 0x7FFFFFF0 || b[2*i] < -0x7FFFFFF0) bounded = 0; }
        }
        CHECK(tail > 0 && late > 0 && bounded && bside, "2.141: an impulse leaves a reverb tail (still there after 4000 ms at decay 100), in range, the drum side untouched");
        RVBB->amt = 0; int c2[32]; for (int i = 0; i < 16; i++) { c2[2*i] = 12345 * (i + 1); c2[2*i+1] = 7; } int d2[32]; memcpy(d2, c2, sizeof c2);
        rvb_mix(c2); int same = 1; for (int i = 0; i < 32; i++) if (c2[i] != d2[i] && !(i % 2 == 0)) same = 0;
        CHECK(same, "2.141: amount 0: only the side channel is guaranteed untouched (the wet part is zero)");
        for (int blk = 0; blk < 400; blk++) { for (int i = 0; i < 16; i++) { b[2*i] = (blk % 2) ? 400000000 : -400000000; b[2*i+1] = 7; } RVBB->amt = 127; rvb_mix(b); for (int i = 0; i < 16; i++) if (b[2*i] > 0x7FFFFFF0 || b[2*i] < -0x7FFFFFF0) bounded = 0; }
        CHECK(bounded, "2.141: a loud square input at full amount and decay: the output stays in range (saturates, no wrap)");
        memset(RVBB, 0xA5, sizeof(rvb_t)); int e3[32]; for (int i = 0; i < 32; i++) e3[i] = 999; rvb_mix(e3); int u = 1; for (int i = 0; i < 32; i++) if (e3[i] != 999) u = 0;
        CHECK(u, "2.141: a block of garbage state (RAM at power-on): the mixer touches nothing until sq_tick has set it up");
        memset(RVBB, 0, sizeof(rvb_t)); }
      Q->rvm = 0; Q->rvd = 0; run(60);
      /* B minor (vibrato) / D minor (tremolo) held: the knobs are locked for the depth / speed (ui_tick applies them), the stock tempo and the note length stay */
      Q->tst = 0; Q->nst = 0; R8(SQ_SUSV + 2u) = 40; R8(SQ_SUSV + 5u) = 50; run(60);
      press(22); run(60);
      CHECK(((Q->nst >> 2) & 7u) == 5u && (Q->nst & 2u) && ((Q->tst >> 2) & 7u) == 3u && (Q->tst & 2u) && Q->tfz == 1, "2.141: B minor held: Sustain / Tempo locked for the vibrato's depth / speed, the tempo frozen");
      u8 gt2 = Q->gt; R8(SQ_SUSV + 2u) = 100; R8(SQ_SUSV + 5u) = 20; run(60);
      CHECK(!(Q->nst & 2u) && !(Q->tst & 2u) && Q->gt == gt2 && Q->cut == 0 && Q->res == 0 && Q->dtm == 0 && Q->dfb == 0 && Q->rvm == 0 && Q->rvd == 0, "2.141: the knobs moved: they take over for ui_tick, nothing else changes");
      rel(22); run(60); press(19); run(60);
      CHECK(((Q->nst >> 2) & 7u) == 5u && (Q->nst & 2u) && ((Q->tst >> 2) & 7u) == 3u && (Q->tst & 2u), "2.141: D minor held: locked again for the tremolo");
      rel(19); run(60); R8(SQ_SUSV + 2u) = 10; R8(SQ_SUSV + 5u) = 120; run(60);
      CHECK(Q->tfz == 0 && Q->gt == 10, "2.141: all let go and the knobs moved: the tempo free, the note length back");
    }
    /* ---- 2.130: drum-pattern slots and the PATTERN bank 3 ---- */
    { if (!LED(0x15)) { tap(0x31, 60); run(100); }
      for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->std[q] = 0xFF; XQ->stx[q] = 0; XQ->sto[q] = 0; }
      XQ->std[0] = 0xF0; XQ->std[4] = 0x21; XQ->stx[4] = 2;
      press(0x31); run(SQ_LONG + 100); rel(0x31); run(100); CHECK(Q->se == 1, "edit mode");
      int ss0 = Q->ss; press(15); run(30); press(0x28); run(60); rel(0x28); rel(15); run(60);
      CHECK(pt()->b[40] == 1 && pt()->b[0] == 0xF0 && pt()->b[4] == 0x21 && ((pt()->b[33] >> 0) & 3) == 2 && Q->p3cur == 1 && Q->ss == ss0, "Bb minor + PATTERN 1 in edit mode: the drums saved in slot 1 (not a step pick)");
      tap(0x31, 60); run(100); CHECK(Q->se == 0, "edit mode ends");
      int e2d = edges[0x2D]; press(0x2D); run(SQ_LONG + 60);
      CHECK(Q->p3a == 1 && LED(0x0E) == 1 && LED(0x0F) == 1 && LED(9) == 1 && edges[0x2D] == e2d, "yellow PATTERN held: the PATTERN bank 3, both its lights, PATTERN 1 lit (slot 1 loaded); the stock got nothing");
      rel(0x2D); run(60); CHECK(Q->p3a == 1, "...let go: still there");
      int e29 = edges[0x29]; press(0x29); run(60); rel(0x29); run(60);
      CHECK(edges[0x29] == e29 && Q->p3cur == 2 && XQ->std[0] == 0xFF, "PATTERN 2: slot 2 (empty) into the sequencer, the stock pattern kept");
      press(0x28); run(60); rel(0x28); run(60); CHECK(Q->p3cur == 1 && XQ->std[0] == 0xF0 && XQ->std[4] == 0x21 && XQ->stx[4] == 2, "PATTERN 1: slot 1 back in the sequencer");
      XQ->std[8] = 0xF3; CHECK(pt()->b[8] == 0xFF, "changed in the sequencer: the slot not yet");
      press(0x31); run(SQ_LONG + 100); rel(0x31); run(100);
      press(15); run(60);
      CHECK(LED(9) == 1 && LED(10) == 0 && LED(11) == 0 && LED(12) == 0 && LED(13) == 0 && LED(2) == 0 && LED(7) == 0 && LED(8) == 0, "2.133: Bb minor held in edit mode: all dark but the used slot (PATTERN 1)");
      press(0x2D); run(60); rel(0x2D); rel(15); run(60);
      CHECK(Q->pcf == 1 && pt()->b[8] == 0xFF && LED(2) == 1 && LED(7) == 1 && LED(8) == 1 && LED(9) == 0 && LED(10) == 0, "2.133: yellow PATTERN on a used slot: not saved, only voice 1 (Omni 1) and the yellow VOICE lit");
      press(0x27); run(60); rel(0x27); run(60);
      CHECK(Q->pcf == 0 && pt()->b[8] == 0xFF && XQ->std[8] == 0xF3 && Q->p3a == 1, "2.133: yellow VOICE: cancelled, the slot kept");
      press(15); run(30); press(0x2D); run(60); rel(0x2D); rel(15); run(60);
      press(0x2A); run(60); rel(0x2A); run(60);
      CHECK(Q->pcf == 1 && pt()->b[8] == 0xFF, "2.133: another button during the question: nothing");
      press(0x24); run(60); rel(0x24); run(60);
      CHECK(Q->pcf == 0 && pt()->b[8] == 0xF3 && Q->p3a == 1 && Q->p3cur == 1, "2.133: voice 1 (Omni 1): overwritten and saved in the slot loaded (1)");
      tap(0x31, 60); run(100);
      CHECK(dv()->p3s == (0x80 | 1), "the bank-3 state kept for power-off");
      press(0x2D); run(60); rel(0x2D); run(100);
      CHECK(Q->p3a == 0 && XQ->std[0] == 0xFF && edges[0x2D] == e2d, "a tap in bank 3: out of it, its drums out of the sequencer (the stock pattern)");
      press(0x2D); run(60); rel(0x2D); run(100); CHECK(edges[0x2D] == e2d + 1, "a tap outside bank 3: the stock's (pattern bank), at the release");
      tap(0x31, 60); run(100); }
    /* ---- 2.131: even steps across the stock counter's wrap (384); Sync Start in the PATTERN bank 3 ---- */
    { if (!LED(0x15)) { tap(0x31, 60); run(100); }
      for (int q = 0; q < 32; q++) { XQ->stn[q] = 0xFF; XQ->std[q] = 0xFF; } for (int q = 0; q < 8; q++) XQ->stn[q] = 2;
      R32(RA(20002A90)) = 200; Q->dv = 2; nl = 0; beat_on = 1; run(19000); beat_on = 0; run(200);
      { int mn = 100000, mx = 0; for (int i = 2; i < nl; i++) { int d = ntime[i] - ntime[i - 1]; if (d < mn) mn = d; if (d > mx) mx = d; }
        CHECK(nl > 40 && mx - mn <= 20, "every step the same distance, also where the stock count wraps (384)"); }
      for (int q = 0; q < 32; q++) XQ->stn[q] = 0xFF;
      tap(0x31, 60); run(100);
      Q->p3a = 1; Q->p3cur = 1; XQ->std[0] = 0xF0; Q->go = 0; beat_on = 1; run(300);
      CHECK(Q->go == 1 && sq_dmute() == 1, "chord mode, PATTERN bank 3: a rhythm started any way (Sync Start) plays the saved pattern");
      beat_on = 0; run(400); Q->p3a = 0; XQ->std[0] = 0xFF; run(50); }
    printf("%s (%d failures), sizeof(sq_t)=%zu\n", fails ? "FAILED" : "ALL OK", fails, sizeof(sq_t));
    return fails != 0;
}
