#!/usr/bin/env python3
"""2.115 (owner, 2026-11-08): the LinnDrum LM-2 kit in the percussion bank (0x600486A0, 1.0.3.0 only).
overlay(img) -> [(flash offset, bytes)] to write over the image (the recipe's own image: the clap the tool 1.3 put there is the reference).
Every replaced sound is scaled so that NEITHER its peak NOR the energy of its first 50 ms is above the sample it replaces (the owner: never louder).
Each sound: mono 16-bit 44.1 kHz (the stock plays drums at a fixed step), a 3 ms fade at the end, then 32 silent frames (the zone's 16-frame loop is silence).
Shorter than the sample it replaces: written in its place (zone end changed). Longer: written in the room the shorter open hi-hat leaves (sample table entry changed).
The Classic (84) tone shares instruments with the Normal (108) one: it gets the shared ones too (the owner: accepted); its own (BD 20, SD 24, HH 23 / 22, claves 21) stay."""
import struct, math, wave
from pathlib import Path
HERE = Path(__file__).resolve().parent
B = 0x486A0                                     # percussion bank (flash offset)
BANK_END = 0x148620                             # the strum bank starts here
MAP = {2: 'kick', 14: 'snare-m', 13: 'stick-h', 6: 'stick-h', 9: 'clap', 10: 'hihat-closed', 11: 'hihat-open', 8: 'ride',
       16: 'tom-hh', 17: 'tom-h', 18: 'tom-m', 4: 'conga-h', 5: 'conga-m', 7: 'cowb', 15: 'tamb', 12: 'cabasa'}
TAIL, FADE = 32, 132                            # silent frames after each sound; 3 ms fade
TRIM = {'ride': -6.0, 'hihat-open': -6.0}       # 2.116 (owner): the cymbals stood out of the kit: 6 dB lower on top of the rule
VERSIONS = {1: {}, 2: TRIM, 3: 'lufs'}          # 1 = z7 (2.115, no trim), 2 = z8 (2.116), 3 = z9 (2.117: every sound at the same loudness)
LUFS = -18.0                                    # 2.117 (owner): one loudness for the whole kit (the kick's at full peak) ...
LUFS_OWN = {'stick-h': -24.0}                   # ... the sidestick lower (owner)
def _biquad(x, b, a):
    y = []; x1 = x2 = y1 = y2 = 0.0; b0, b1, b2 = b[0] / a[0], b[1] / a[0], b[2] / a[0]; a1, a2 = a[1] / a[0], a[2] / a[0]
    for v in x:
        o = b0 * v + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2; x2, x1, y2, y1 = x1, v, y1, o; y.append(o)
    return y
def loudness(x, fs=44100):
    """the highest momentary loudness (ITU-R BS.1770 K-weighting, 400 ms window) of a one-shot, in LUFS"""
    c, sn = math.cos, math.sin
    A = 10 ** (4.0 / 40); w0 = 2 * math.pi * 1500.0 / fs; al = sn(w0) / (2 / math.sqrt(2))
    b = [A * ((A + 1) + (A - 1) * c(w0) + 2 * math.sqrt(A) * al), -2 * A * ((A - 1) + (A + 1) * c(w0)), A * ((A + 1) + (A - 1) * c(w0) - 2 * math.sqrt(A) * al)]
    a = [(A + 1) - (A - 1) * c(w0) + 2 * math.sqrt(A) * al, 2 * ((A - 1) - (A + 1) * c(w0)), (A + 1) - (A - 1) * c(w0) - 2 * math.sqrt(A) * al]
    y = _biquad([v / 32768.0 for v in x] + [0.0] * 17640, b, a)
    w0 = 2 * math.pi * 38.0 / fs; al = sn(w0) / (2 * 0.5)
    y = _biquad(y, [(1 + c(w0)) / 2, -(1 + c(w0)), (1 + c(w0)) / 2], [1 + al, -2 * c(w0), 1 - al])
    n = 17640; e = sum(v * v for v in y[:n]); best = e
    for i in range(n, len(y)): e += y[i] * y[i] - y[i - n] * y[i - n]; best = max(best, e)
    return -0.691 + 10 * math.log10(best / n + 1e-12)
def u32(d, a): return struct.unpack_from('<I', d, a)[0]
def bank(d):
    st = B + 0x20 + u32(d, B + 0x20); base = st - 0x20; n = u32(d, B + 0x24)
    recs = [B + 0x14 + u32(d, B + 0x28 + 4 * i) for i in range(n)]       # rec+4 = the zone: +8 end, +0x10 sample index
    return st, base, recs
def stock_sample(d, i):
    st, base, recs = bank(d); r = recs[i]; end = u32(d, r + 8); si = u32(d, r + 0x10); a = base + u32(d, st + 8 + 4 * si)
    return list(struct.unpack_from('<%dh' % (end + 1), d, a)), si, a
def level(x):
    n = min(len(x), 2205); return max(abs(v) for v in x), math.sqrt(sum(v * v for v in x[:n]) / n)
def wav(name):
    w = wave.open(str(HERE / 'linn' / (name + '.wav'))); assert w.getnchannels() == 1 and w.getsampwidth() == 2 and w.getframerate() == 44100, name
    return list(struct.unpack('<%dh' % w.getnframes(), w.readframes(w.getnframes())))
def overlay(img, report=None, version=3):
    trim = VERSIONS[version]
    d = bytes(img); st, base, recs = bank(d)
    out = []; sounds = {}
    for name in sorted(set(MAP.values())):         # the gain: the lowest any of the samples it replaces asks for
        y = wav(name); py, ry = level(y); g = 10.0
        if trim == 'lufs':                         # version 3: the same loudness for all, never above full peak
            g = min(10 ** ((LUFS_OWN.get(name, LUFS) - loudness(y)) / 20.0), 32767.0 / py)
        else:
            for i, nm in MAP.items():
                if nm != name: continue
                x, _, _ = stock_sample(d, i); px, rx = level(x); g = min(g, px / py, rx / ry)
            g *= 10 ** (trim.get(name, 0.0) / 20.0)
        z = [int(round(v * g)) for v in y]
        for k in range(min(FADE, len(z))): z[len(z) - 1 - k] = int(round(z[len(z) - 1 - k] * k / FADE))
        z += [0] * TAIL
        z = [max(-32768, min(32767, v)) for v in z]
        sounds[name] = (z, g)
    # where each goes: its own slot if it fits, else the free room after the new open hi-hat (slot of instrument 11)
    _, si11, a11 = stock_sample(d, 11); room = a11 + 2 * len(sounds['hihat-open'][0]); room = (room + 3) & ~3
    room_end = a11 + 2 * (u32(d, recs[11] + 8) + 1)
    placed = {}
    for i in sorted(MAP):
        name = MAP[i]; z, g = sounds[name]; x, si, a = stock_sample(d, i)
        if name in placed: addr = placed[name]
        elif len(z) <= len(x): addr = a; placed[name] = a; out.append((addr, struct.pack('<%dh' % len(z), *z)))
        else:
            addr = room; room += 2 * len(z); room = (room + 3) & ~3; assert room <= room_end, 'no room'
            placed[name] = addr; out.append((addr, struct.pack('<%dh' % len(z), *z)))
        out.append((recs[i] + 8, struct.pack('<I', len(z) - 1)))                       # zone end = frames - 1
        if addr != a: out.append((st + 8 + 4 * si, struct.pack('<I', addr - base)))     # sample table entry
        if report is not None: report.append((i, name, len(z), len(x), 20 * math.log10(g), hex(0x60000000 + addr)))
    for a, b in out: assert B <= a and a + len(b) <= BANK_END
    return out
if __name__ == '__main__':
    import sys; sys.path.insert(0, str(HERE.parent.parent / 'assets')); import om108_patcher as P
    stock = Path(sys.argv[1]).read_bytes(); rec = P.load_recipe(str(HERE.parent / 'legacy_builds' / 'om108_mod.patch.orig'), stock)
    img = P.build_image(stock, rec, P.default_settings()); rep = []; ov = overlay(img, rep)
    for r in rep: print('%2d %-13s %6d fr (was %6d) gain %+5.1f dB at %s' % r)
    print(len(ov), 'writes,', sum(len(b) for _, b in ov), 'bytes')
