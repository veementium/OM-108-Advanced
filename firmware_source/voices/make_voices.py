#!/usr/bin/env python3
"""2.127 (owner): the banjo becomes a one-sample instrument, and 5 more one-sample voices (the third voice bank) share its instrument.
The owner's WAV (voices/cripacio.wav) holds 6 sounds: 1 = the banjo's place, 2-6 = third-bank VOICE 1-5. Each is played with C4 = its own pitch (the owner).
All 6 have the SAME length and loop (one zone, the banjo's: the mod swaps only the sample address in the engine's RAM table): N frames at 22.05 kHz, the last
L frames are the loop (a sustained sound: a crossfaded loop; a decaying one: silence), + 1 guard frame. Level: momentary loudness = the old banjo's C4 sample
(-14.2 LUFS), never above full peak. Written over the old banjo samples (0x60918AD4, nothing else uses them); the banjo record (0x60149668) gets 1 zone
(keys 0-127, sample 93, root 72 = C4 at 22.05 kHz), sample table entry 93 -> sound 1. 1.0.3.0 only. Keep N / L / BASE in step with seq.h (B3_*)."""
import struct, math, wave
from pathlib import Path
HERE = Path(__file__).resolve().parent
import sys; sys.path.insert(0, str(HERE.parent / 'drums')); import make_drums as M
FS = 22050
N, L = 33075, 5513                     # 1.5 s, the last 0.25 s looped
STRIDE = (N + 1) * 2                    # + the guard frame
BASE = 0x918AD4                         # the old banjo sample data (flash offset)
REGION_END = 0x97F810
BANK, REC, SMP = 0x148620, 0x149668, 93
TARGET = -14.2
# Versions of the sounds (the earlier builds stay recognisable): sound i = third-bank VOICE i (0 = the banjo). SUS = the sounds that hold (a loop in the last L
# frames); EXT = sounds rebuilt as "attack, then a repeating cycle" (file or None = the owner's cripacio.wav segment, loop start in 22.05 kHz frames after the trim).
#   0: builds z23-z29.  1: z30 (2.139): VOICE 3 = Alesis S4 Plus "Brassy 5th" C4 (holds), VOICE 5 = Kawai K1 II "Rhodes" C4 (decays); stereo 44.1 kHz WAVs summed to mono.
#   2: z31 (2.140, owner): every third-bank sound gets a repeating tail (a loop from a steady part of the sound, crossfaded), so the Sustain knob sets how long it
#      rings, as for the stock sounds; VOICE 2 and 3 are made constant while held by the firmware (seq.h b3_dec), not here.
VERS = {0: dict(ext={}, sus={0, 2}),
        1: dict(ext={3: ('brassy_c4.wav', 12115), 5: ('rhodes_c4.wav', None)}, sus={0, 2, 3}),
        # 'x' = crossfaded cycle; 'p' = pitch-synchronous cycle (a whole number of periods, resampled to L frames: at most about 13 cents off), lag range for the period
        2: dict(ext={1: (None, 31376, 'p', (30, 360)), 3: ('brassy_c4.wav', 12115, 'x', None), 4: (None, 26302, 'x', None), 5: ('rhodes_c4.wav', 25142, 'p', (70, 100))},
                sus={0, 1, 2, 3, 4, 5}),
        # 3: z32 (2.141, owner): VOICE 1 and 4 tails had an audible repeat -> a loop from a steadier part and a crossfade of half the loop (F = L / 2, as the sounds that hold);
        #    VOICE 2 and 3 are plain looped sounds again (the layer byte [3] of z31 was the layer's octave shift, not a decay: removed from the firmware).
        3: dict(ext={1: (None, 24993, 'x', None, 2756), 3: ('brassy_c4.wav', 12115, 'x', None), 4: (None, 27312, 'x', None, 2756),
                     5: ('rhodes_c4.wav', 25142, 'p', (70, 100))}, sus={0, 1, 2, 3, 4, 5}),
        # 4: z33 (2.142, owner): keyboard mode: VOICE 2 and 3 stay at a constant level while the key is held (as Omni 1). The samples themselves decayed (VOICE 2 to 30 % in 1.5 s,
        #    VOICE 3 to half after 0.5 s) under the loop: `flatten` evens the level out after the attack and the loop is taken from that even part.
        4: dict(ext={1: (None, 24993, 'x', None, 2756), 2: (None, 10531, 'p', (30, 360), 2756, True), 3: ('brassy_c4.wav', 13181, 'x', None, 2756, True),
                     4: (None, 27312, 'x', None, 2756), 5: ('rhodes_c4.wav', 25142, 'p', (70, 100))}, sus={0, 1, 2, 3, 4, 5})}
SEGS = [(0.00, 3.17), (4.01, 6.67), (7.99, 11.99), (11.99, 13.33), (15.99, 18.15), (19.99, 23.00)]   # seconds in the owner's file
def load():
    w = wave.open(str(HERE / 'cripacio.wav')); n, ch, sw, fs = w.getnframes(), w.getnchannels(), w.getsampwidth(), w.getframerate()
    assert sw == 2 and fs == 44100
    buf = w.readframes(n); n = len(buf) // (2 * ch); a = struct.unpack('<%dh' % (n * ch), buf[:n * ch * 2])
    return [sum(a[i * ch:(i + 1) * ch]) / ch for i in range(n)], fs
def down(x):                            # 44.1 -> 22.05 kHz: a 31-tap windowed-sinc low-pass, every second frame
    h = []
    for k in range(-15, 16):
        s = 0.5 if k == 0 else math.sin(math.pi * 0.5 * k) / (math.pi * k)
        h.append(s * (0.54 + 0.46 * math.cos(math.pi * k / 15)))
    return [sum(h[k + 15] * x[i + k] for k in range(-15, 16) if 0 <= i + k < len(x)) for i in range(0, len(x), 2)]
def pitch_cycle(y, a, lags):
    """One loop cycle of exactly L frames from y[a:]: P whole periods of the sound's pitch (autocorrelation, smallest lag within 10 % of the best), resampled."""
    lo, hi = lags; seg = y[a:a + 3 * L // 2]; mean = sum(seg) / len(seg); seg = [v - mean for v in seg]
    ac = [sum(seg[j] * seg[j + k] for j in range(len(seg) - k)) for k in range(lo - 1, hi + 2)]
    m = max(ac[1:-1]); k = next(j for j in range(1, len(ac) - 1) if ac[j] >= 0.9 * m)
    while ac[k + 1] > ac[k] and k < len(ac) - 2: k += 1          # up to the peak
    A, B, C = ac[k - 1], ac[k], ac[k + 1]; T = (lo - 1 + k) + ((A - C) / (2 * (A - 2 * B + C)) if A - 2 * B + C else 0.0)
    P = max(1, round(L / T)); span = P * T; F = 300; out = []
    for j in range(-F, L):                                         # Catmull-Rom resampling (the F frames before the loop start too)
        p = a + j * span / L; i0 = math.floor(p); f = p - i0; p0, p1, p2, p3 = y[i0 - 1], y[i0], y[i0 + 1], y[i0 + 2]
        out.append(p1 + 0.5 * f * (p2 - p0 + f * (2 * p0 - 5 * p1 + 4 * p2 - p3 + f * (3 * (p1 - p2) + p3 - p0))))
    pre, cyc = out[:F], out[F:]
    for j in range(F): t = j / F; cyc[L - F + j] = cyc[L - F + j] * (1 - t) + pre[j] * t      # the end blends into what precedes the start: the wrap is continuous
    return cyc
def flatten(y, W=2205, t0=0.05, t1=0.15, lo=0.3, hi=5.0):
    """2.142: a constant level after the attack. gain = (mean level 0.15-0.5 s) / (level over a 0.1 s window), limited to lo..hi, faded in from 0.05 to 0.15 s."""
    n = len(y); c = [0.0]
    for v in y: c.append(c[-1] + v * v)
    h = W // 2; e = []
    for i in range(n):
        a, b = max(0, i - h), min(n, i + h); e.append(math.sqrt((c[b] - c[a]) / max(b - a, 1)) + 1.0)
    i0, i1 = int(0.15 * FS), int(0.5 * FS); R = sum(e[i0:i1]) / (i1 - i0); out = []
    for i in range(n):
        g = min(hi, max(lo, R / e[i])); w = min(1.0, max(0.0, (i / FS - t0) / (t1 - t0))); out.append(y[i] * (1 + (g - 1) * w))
    return out
def load_ext(name):
    w = wave.open(str(HERE / name)); n, ch, sw, fs = w.getnframes(), w.getnchannels(), w.getsampwidth(), w.getframerate()
    assert sw == 2 and fs == 44100
    buf = w.readframes(n); a = struct.unpack('<%dh' % (n * ch), buf)
    return [sum(a[i * ch:(i + 1) * ch]) / ch for i in range(n)]
def sound(x, fs, i, ver=4):
    SUS, EXT = VERS[ver]['sus'], VERS[ver]['ext']
    if i in EXT and EXT[i][0]:
        name = EXT[i][0]; s = load_ext(name)
    else:
        a, b = int(SEGS[i][0] * fs), int(SEGS[i][1] * fs); s = x[a:b]
    th = max(abs(v) for v in s) * 0.02
    k = next(j for j, v in enumerate(s) if abs(v) > th); s = s[max(0, k - 44):]   # the attack, 1 ms before it
    y = down(s)
    if i in EXT and len(EXT[i]) > 5 and EXT[i][5]: y = flatten(y[:44100])      # 2.142: constant level after the attack
    if i in EXT and EXT[i][1] is not None:  # a one-shot made to hold: attack up to the loop start, then its 0.25 s cycle repeated (crossfaded: its end -> what precedes its start)
        a = EXT[i][1]; F = EXT[i][4] if len(EXT[i]) > 4 else 2000
        assert ver < 3 or a <= N - L, 'the loop must start before the stored loop region (N - L): the wrap has to land inside the repeating part'   # (z31's VOICE 1 broke this: a click at every wrap)
        if len(EXT[i]) > 2 and EXT[i][2] == 'p': cyc = pitch_cycle(y, a, EXT[i][3])
        else:
            cyc = y[a:a + L]
            for j in range(F): t = j / F; cyc[L - F + j] = cyc[L - F + j] * (1 - t) + y[a - F + j] * t
        y = y[:a]; 
        while len(y) < N: y += cyc
        y = y[:N]
    elif i in SUS:
        y = (y + [0.0] * N)[:N]; a0 = N - L; F = L // 2
        for j in range(F):             # crossfade the loop's end with what comes before its start: seamless jump back
            t = j / F; y[a0 + L - F + j] = y[a0 + L - F + j] * (1 - t) + y[a0 - F + j] * t
    else:
        body = N - L; y = (y + [0.0] * N)[:body]; fd = int(0.1 * FS)
        for j in range(fd): y[body - fd + j] *= 1 - j / fd
        y += [0.0] * L
    g = min(10 ** ((TARGET - M.loudness([int(v) for v in y], FS)) / 20), 32000.0 / max(1.0, max(abs(v) for v in y)))
    z = [max(-32768, min(32767, int(round(v * g)))) for v in y]
    z.append(z[N - L])                  # the guard frame: the loop start
    return z, 20 * math.log10(g)
def overlay(img, report=None, ver=4):
    d = bytes(img); u32 = lambda a: struct.unpack_from('<I', d, a)[0]
    st = BANK + 0x20 + u32(BANK + 0x20); base = st - 0x20
    assert u32(st + 8 + 4 * SMP) + base == BASE and u32(REC) == 19, 'the banjo is not where it was'
    x, fs = load(); out = []
    for i in range(6):
        z, gdb = sound(x, fs, i, ver); a = BASE + i * STRIDE; assert a + STRIDE <= REGION_END
        out.append((a, struct.pack('<%dh' % len(z), *z)))
        if report is not None: report.append((i + 1, hex(0x60000000 + a), round(gdb, 1)))
    z0 = REC + 4; zone = bytearray(d[z0:z0 + 28])
    struct.pack_into('<I', zone, 4, N - 1); struct.pack_into('<I', zone, 8, L); struct.pack_into('<I', zone, 0xC, SMP)
    struct.pack_into('<h', zone, 0x10, 0); zone[0x14] = 0; zone[0x15] = 127; zone[0x1B] = 72
    out.append((REC, struct.pack('<I', 1))); out.append((z0, bytes(zone)))
    out.append((st + 8 + 4 * SMP, struct.pack('<I', BASE - base)))
    return out
if __name__ == '__main__':
    d = open(sys.argv[1], 'rb').read(); rep = []; ov = overlay(d, rep)
    for r in rep: print('sound %d at %s gain %+.1f dB' % r)
    print(len(ov), 'writes', sum(len(b) for _, b in ov), 'bytes')
