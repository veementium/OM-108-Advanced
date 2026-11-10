"""Checks a regenerated recipe. Usage (from the project folder): python3 firmware_source/verify_recipe.py STOCK_BACKUP NEW_BIN PREVIOUS_BIN
Prints: the new code is in the image / every earlier build still rebuilds / the previous build with custom settings is recognised / no changed sector outside the regions."""
import sys, hashlib
sys.path.insert(0, 'assets')
import om108_patcher as P
stock_p, new_p, prev_p = sys.argv[1:4]
sha = lambda b: hashlib.sha256(bytes(b)).hexdigest()
stock = open(stock_p, 'rb').read(); code = open(new_p, 'rb').read()
rec = P.load_recipe('assets/om108_mod.patch', stock)
img = P.build_image(stock, rec, P.default_settings())
print('new code in image:', img[0x3A000:0x3A000 + len(code)] == code, '| code size', len(code), 'of 53248')
want = {L['name']: L['target_fw_sha256'] for L in rec[0]['legacy']}
print('legacy rebuilt:', all(sha(im[:P.FW_END]) == want[n] for n, im in P.code_variants(stock, rec, img)), len(want))
orig = P.load_recipe('firmware_source/legacy_builds/om108_mod.patch.orig', stock)
d = P.default_settings(); d['kb_transpose'] = not d['kb_transpose']
oldx = bytearray(P.build_image(stock, orig, d)); nb = open(prev_p, 'rb').read(); oldx[0x3A000:0x3A000 + len(nb)] = nb
newx = P.build_image(stock, rec, d)
print('previous build + custom settings recognised:', any(sha(im[:P.FW_END]) == sha(oldx[:P.FW_END]) for n, im in P.code_variants(stock, rec, newx, d)))
regs = P.regions_for(rec); reg = lambda s: any(a <= s < a + n for a, n in regs)
print('changed sectors outside regions:', [hex(s) for s in range(0x2000, P.FW_END, 0x1000) if stock[s:s + 0x1000] != img[s:s + 0x1000] and not reg(s)])
