#!/usr/bin/env python3
"""Rebuilds assets/om108_mod.patch's 1.0.3.0 variant with the code in mod2_v30.bin (run build.sh 30 first).
   python3 make_patch.py <your om108_backup_v30.bin> ["name=path/to/earlier_mod2_v30.bin" ...]   (from the firmware_source folder; run it on the ORIGINAL om108_mod.patch)
The 1.0.2.9 variant is copied unchanged. The firmware this recipe had before (the tool 1.3 one) and the earlier builds it
knew stay recognised, so the tool can install over a unit that runs any of them. The patch holds only this project's own
bytes (the differences from stock), never Suzuki code."""
import sys, json, lzma, struct, hashlib
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'assets'))
import om108_patcher as P
def sha(b): return hashlib.sha256(bytes(b)).hexdigest()
CODE, CODE_END = 0x3A000, 0x47000
DRUM, DRUM_END = 0x48000, 0x148620          # 2.115: the percussion bank (the LinnDrum kit, drums/make_drums.py)
sys.path.insert(0, str(HERE / 'drums'))
import make_drums
sys.path.insert(0, str(HERE / 'voices'))
import make_voices                                   # 2.127: the one-sample banjo and the third voice bank's sounds
VOICE_AREAS = [(0x918000, 0x980000), (0x149668, 0x149668 + 32), (0x14A2E0, 0x14A2E4)]   # their samples, the banjo record (count + zone 1), sample table entry 93
# 2.118 (owner): in the Normal (108) tone's PATTERNS only, the open hi-hat row plays the cymbal instrument (8, the ride) and the two tom rows the congas.
# The stock row player 0x6000763C (1.0.3.0) picks the instrument per row in a switch used only when the tone (DE87) is Normal (tbb at 0x60007706); its cases set
# r4 = instrument (passed to the Normal hit 0x600187E8), r6 = instrument for the rest path, r9 = the hi-hat choke flag. The Classic tone, the drum keys, the plates and
# the step sequencer use other code / tables: untouched. MIDI keeps each row's note. [(flash offset, stock bytes, new bytes)]
def _bl(frm, to):                                            # Thumb-2 BL encoding
    off = to - (frm + 4); s_ = (off >> 24) & 1; i1 = (off >> 23) & 1; i2 = (off >> 22) & 1; j1 = (1 - i1) ^ s_; j2 = (1 - i2) ^ s_
    h1 = 0xF000 | (s_ << 10) | ((off >> 12) & 0x3FF); h2 = 0xD000 | (j1 << 13) | (j2 << 11) | ((off >> 1) & 0x7FF)
    return struct.pack('<HH', h1, h2).hex()
# r4 only for the toms (r6 / r7 stay: the keyboard-mode pad rolls keep following the toms' pads); row 8 calls entry +0x90 (row8_case: r4 / r9 by pattern)
ROWS = [(0x7852, '0b2416270b264ff00109', _bl(0x60007852, 0x6003A090) + '16270b26' + '00bf'),   # row 8: bl row8_case; movs r7,#0x16; movs r6,#11; nop
        (0x7876, '1024', '0424'),                                     # row 5 (tom 16) -> conga 4 (high)
        (0x7882, '1124', '0524')]                                     # row 4 (tom 17) -> conga 5 (mid)
# 2.121: ROWS's row 8 patch CRASHED the Classic tone (its row 8 case branches to 0x60007854, now the middle of the bl): builds z10-z12 only ('rows').
# From 2.121 ('rows2') only the toms; the open hi-hat -> ride is done in r27_row (mod2.c). The Classic cases enter the tom blocks at 0x7878 / 0x7884, after
# the bytes changed here (they set their own r4 first): the Classic toms stay stock.
ROWS2 = ROWS[1:]
ROWS_AREA = (0x7000, 0x8000)
# 2.119 (owner): a volume per drum instrument: the two veneers to the ITCM drum hits (0x600187C0 -> 0x20200344, 0x600187E8 -> 0x20200488) jump to our
# drum_fix / drum_vel (entry +0x94 / +0x98), which call them at the instrument's volume. Every caller in flash goes through these veneers (checked: no other
# reference to either function, none from the ITCM itself).
VOL = [(0x187C4, struct.pack('<I', 0x20200345).hex(), struct.pack('<I', 0x6003A095).hex()),
       (0x187EC, struct.pack('<I', 0x20200489).hex(), struct.pack('<I', 0x6003A099).hex())]
VOL_AREA = (0x18000, 0x19000)
# 2.135 (owner): the sensor loop's one call of the stock tempo routine -> entry +0x9C (tempo_hook: skipped while Bb minor freezes the tempo)
TEMPO = [(0x3752, _bl(0x60003752, 0x60007554), _bl(0x60003752, 0x6003A09C))]
GROUPS = {'rows': ROWS, 'rows2': ROWS2, 'vol': VOL, 'tempo': TEMPO}
def rows_patch(im, stock, groups=('rows2', 'vol', 'tempo')):
    for g in groups:
        for a, o, n in GROUPS[g]:
            o, n = bytes.fromhex(o), bytes.fromhex(n); assert stock[a:a + len(o)] == o and len(o) == len(n), hex(a); im[a:a + len(n)] = n
# 2.139 (owner): spoken clips only for "84 mode" / "27 mode" (clips 1 = 84, 2 = mode, 8 = 27). The others (108, note off, vibrato, tremolo, auto, manual) are
# dropped: their table entries get length 0 (clip_get fails -> silent) and their bytes go back to blank flash (the blob is repacked, clips keep their ids).
PBLOB, PKEEP = 0xF40000, (1, 2, 8)
def prompts_overlay(img):
    d = bytes(img); assert d[PBLOB:PBLOB + 4] == b'OMPR'
    n = struct.unpack_from('<I', d, PBLOB + 4)[0]; tab = [struct.unpack_from('<II', d, PBLOB + 8 + 8 * i) for i in range(n)]
    hdr = 8 + 8 * n; dec = d[PBLOB + hdr:PBLOB + hdr + 512]; base = hdr + 512; out = b''; nt = []
    for i, (o, l) in enumerate(tab):
        if i in PKEEP: nt.append((base + len(out), l)); out += d[PBLOB + o:PBLOB + o + l]
        else: nt.append((base, 0))
    blob = b'OMPR' + struct.pack('<I', n) + b''.join(struct.pack('<II', *t) for t in nt) + dec + out
    end = max(o + l for o, l in tab); assert len(blob) <= end
    return [(PBLOB, blob + b'\xff' * (end - len(blob)))]
def diff_runs(a, b, lo, hi):
    """[(addr, n)] where a and b differ, lo <= addr < hi (contiguous differing bytes). 2.132: equal 4 KB blocks skipped (same result)."""
    out = []; i = lo; B = 0x1000
    while i < hi:
        e = min(hi, (i // B + 1) * B)
        if a[i:e] == b[i:e]: i = e; continue
        while i < e:
            if a[i] != b[i]:
                j = i + 1
                while j < hi and a[j] != b[j]: j += 1
                if out and out[-1][0] + out[-1][1] == i: out[-1] = (out[-1][0], out[-1][1] + j - i)
                else: out.append((i, j - i))
                i = j
            else: i += 1
    return out
def main():
    stock = Path(sys.argv[1]).read_bytes()
    code = (HERE / 'mod2_v30.bin').read_bytes()
    src = HERE.parent / 'assets' / 'om108_mod.patch'
    rs = P.load_recipes(src)
    k = next(i for i, r in enumerate(rs) if r[0]['stock_fw_sha256'] == sha(stock[:P.FW_END]))
    hdr, runs, data = rs[k]
    old = bytearray(P.build_image(stock, (hdr, runs, data), P.default_settings()))
    assert old[CODE:CODE + len(code)] != code, 'the recipe already holds this code'
    # earlier builds this recipe knows (code only), as images with the default settings
    earlier = [(n, bytearray(im)) for n, im in P.code_variants(stock, (hdr, runs, data), old)] + [('settings build 7 (tool 1.3)', old)]
    # 2.132 (owner): no Electric Harp, no Better Clap: the new image starts from the tool 1.3 one with both stock (their bytes
    # from the backup, as the tool did with them off), its settings block kept (bits 4-5 stay 1: ignored by the firmware).
    d0 = P.default_settings(); d0['electric_harp'] = d0['better_clap'] = False
    base = bytearray(P.build_image(stock, (hdr, runs, data), d0)); base[P.CFG_OFF:P.CFG_OFF + 64] = old[P.CFG_OFF:P.CFG_OFF + 64]
    for arg in sys.argv[2:]:                                  # more earlier builds: "name=path of their mod2_v30.bin" (the code only)
        nm, _, pth = arg.partition('='); pth, _, flag = pth.partition(';'); eb = Path(pth).read_bytes()
        b0 = base if 'nohc' in flag.split(',') else old     # 2.132: ';nohc' = a build without Electric Harp / Better Clap (from z23)
        im = bytearray(b0); im[CODE:CODE + len(eb)] = eb
        for f in flag.split(','):
            if f.startswith('drums'):                         # a build with the LinnDrum kit: "name=file;drums" (z7, 2.115) or ";drumsN" (its version N)
                for a, b in make_drums.overlay(b0, version=int(f[5:] or 1)): im[a:a + len(b)] = b
            if f == 'voices3':                                # 2.139 (z30): the new third-bank sounds (Brassy / Rhodes)
                for a, b in make_voices.overlay(im, ver=1): im[a:a + len(b)] = b
            if f == 'voices4':                                # 2.140 (z31 on): every third-bank sound with a repeating tail
                for a, b in make_voices.overlay(im, ver=2): im[a:a + len(b)] = b
            if f == 'voices5':                                # 2.141 (z32 on): VOICE 1 / 4 with a longer crossfade, VOICE 2 / 3 plain loops
                for a, b in make_voices.overlay(im, ver=3): im[a:a + len(b)] = b
            if f == 'voices6':                                # 2.142 (z33 on): VOICE 2 / 3 at a constant level
                for a, b in make_voices.overlay(im, ver=4): im[a:a + len(b)] = b
            if f == 'prompts':                                # 2.139 (z30 on): only the 84 / 27 spoken clips
                for a, b in prompts_overlay(im): im[a:a + len(b)] = b
            if f == 'voices':                                 # 2.127
                for a, b in make_voices.overlay(im, ver=0): im[a:a + len(b)] = b      # 2.139: the earlier builds keep their own sounds
            if f in GROUPS: rows_patch(im, stock, (f,))       # 2.118: with the 108 pattern rows changed (";drums3,rows"); 2.119: ',vol'
        earlier.append((nm, im))
    new = bytearray(base); assert CODE + len(code) <= CODE_END and len(code) >= 0x6000
    new[CODE:CODE + len(code)] = code
    for a, b in make_voices.overlay(new): new[a:a + len(b)] = b   # 2.127
    for a, b in prompts_overlay(new): new[a:a + len(b)] = b      # 2.139: only the 84 / 27 clips
    pre = bytearray(new)                                     # (without the kit: what LinnDrum off gives back)
    for a, b in make_drums.overlay(base): new[a:a + len(b)] = b   # 2.115: the LinnDrum kit
    rows_patch(new, stock)                                   # 2.118: the 108 patterns' toms -> congas; 2.119: the drum volumes
    # 2.132: the switches' areas: where the kit (+ the tom rows) and the Extras sounds differ from stock; the tool writes the backup's bytes there when off
    linn = diff_runs(pre, new, DRUM, DRUM_END) + [(a, len(bytes.fromhex(o))) for a, o, n in ROWS2]
    extras = [r for lo0, hi0 in VOICE_AREAS for r in diff_runs(stock, new, lo0, hi0)]
    for a, n in linn: assert pre[a:a + n] == stock[a:a + n], ('linn', hex(a))
    for a, n in extras: assert base[a:a + n] == stock[a:a + n], ('extras', hex(a))
    # the new variant's runs: everywhere it differs from stock
    nruns = diff_runs(stock, new, 0, P.FW_END)
    ndata = b''.join(bytes(new[a:a + n]) for a, n in nruns)
    img = bytearray(stock); o = 0
    for a, n in nruns: img[a:a + n] = ndata[o:o + n]; o += n
    assert sha(img[:P.FW_END]) == sha(new[:P.FW_END]), 'runs do not rebuild the image'
    nh = {kk: v for kk, v in hdr.items() if not kk.startswith('_') and kk not in ('clap', 'harp_stock')}
    nh.update(legacy_clap=hdr['clap'], legacy_harp=hdr['harp_stock'], linn_stock=[list(x) for x in linn], extras_stock=[list(x) for x in extras])
    nh.update(runs=len(nruns), data_len=len(ndata), data_sha256=sha(ndata), target_fw_sha256=sha(new[:P.FW_END]))
    secs = set(hdr['sectors']) | {x for a, n in nruns for x in range(a & ~0xFFF, a + n, 0x1000)}
    nh['sectors'] = sorted(secs)
    legacy = []
    for name, im in earlier:
        D = diff_runs(new, im, 0, P.FW_END)                  # where that build differs from the new one
        lruns = []
        for a, n in D: lruns += diff_runs(stock, im, a, a + n)   # ... and from stock there: its own bytes
        ldata = b''.join(bytes(im[a:a + n]) for a, n in lruns)
        legacy.append(dict(code_only=True, name=name, runs=len(lruns), data_len=len(ldata), data_sha256=sha(ldata),
                           target_fw_sha256=sha(im[:P.FW_END]), stock=[list(x) for x in D], _runs=lruns, _data=ldata))
        v = bytearray(new)                                   # check: new build -> that build
        for a, n in D: v[a:a + n] = stock[a:a + n]
        o = 0
        for a, n in lruns: v[a:a + n] = ldata[o:o + n]; o += n
        assert sha(v[:P.FW_END]) == sha(im[:P.FW_END]), name
    nh['legacy'] = [{kk: v for kk, v in L.items() if not kk.startswith('_')} for L in legacy]
    variants = []
    for i, (h, r, d) in enumerate(rs):
        if i == k: variants.append((nh, nruns, ndata, legacy))
        else:
            hh = {kk: v for kk, v in h.items() if not kk.startswith('_')}
            hh['legacy'] = [{kk: v for kk, v in L.items() if not kk.startswith('_')} for L in h['legacy']]
            variants.append((hh, r, d, [dict(L) for L in h['legacy']]))
    body = b''
    for h, r, d, leg in variants:
        body += b''.join(struct.pack('<II', a, n) for a, n in r) + d
        for L in leg: body += b''.join(struct.pack('<II', a, n) for a, n in L['_runs']) + L['_data']
    jl = json.dumps({'format': 3, 'variants': [v[0] for v in variants]}, separators=(',', ':')).encode()
    raw = b'OM108RCP' + struct.pack('<I', len(jl)) + jl + body
    out = HERE.parent / 'assets' / 'om108_mod.patch'
    out.write_bytes(lzma.compress(raw, format=lzma.FORMAT_XZ, preset=6))
    print('wrote', out, out.stat().st_size, 'bytes; new target', nh['target_fw_sha256'][:16], '; earlier builds recognised:', [L['name'] for L in legacy])
main()
