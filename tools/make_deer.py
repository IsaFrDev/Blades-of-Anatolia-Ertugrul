# -*- coding: utf-8 -*-
"""Past-poligonli kiyik modeli (ov missiyasi uchun). Tashqi asset yo'q edi —
qutilardan yig'iladi: tana, bo'yin, bosh, 4 oyoq, 2 shox, dum. ~300 verteks,
shuning uchun Skin.cpp uni "oddiy rig" (faqat tana harakati) bilan chizadi.
Chiqish: assets/models/nature/deer.obj + deer.mtl. Balandligi ~1.5 m, +Z ga qaraydi.
"""
import io, os, math

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "assets", "models", "nature")

verts, norms, faces = [], [], []   # faces: (vi list, ni, mat)

def box(cx, cy, cz, sx, sy, sz, mat, rx=0.0):
    """Markazi (cx,cy,cz), o'lchami (sx,sy,sz); rx — X o'qi atrofida burish (gradus)."""
    hx, hy, hz = sx / 2, sy / 2, sz / 2
    c, s = math.cos(math.radians(rx)), math.sin(math.radians(rx))
    def P(x, y, z):
        y2, z2 = y * c - z * s, y * s + z * c
        return (cx + x, cy + y2, cz + z2)
    corners = [P(-hx, -hy, -hz), P(hx, -hy, -hz), P(hx, hy, -hz), P(-hx, hy, -hz),
               P(-hx, -hy, hz), P(hx, -hy, hz), P(hx, hy, hz), P(-hx, hy, hz)]
    base = len(verts)
    verts.extend(corners)
    quads = [((0, 3, 2, 1), (0, 0, -1)), ((4, 5, 6, 7), (0, 0, 1)),
             ((0, 1, 5, 4), (0, -1, 0)), ((3, 7, 6, 2), (0, 1, 0)),
             ((0, 4, 7, 3), (-1, 0, 0)), ((1, 2, 6, 5), (1, 0, 0))]
    for q, n in quads:
        nx, ny, nz = n[0], n[1] * c - n[2] * s, n[1] * s + n[2] * c
        norms.append((nx, ny, nz)); ni = len(norms)
        faces.append(([base + i + 1 for i in q], ni, mat))

# tana (bo'yi ~0.9 m da), +Z old tomon
box(0.0, 0.95, 0.0, 0.42, 0.46, 1.10, "fur")
# bo'yin — oldinga va yuqoriga
box(0.0, 1.22, 0.62, 0.20, 0.42, 0.22, "fur", rx=-35)
# bosh
box(0.0, 1.42, 0.86, 0.20, 0.20, 0.34, "fur")
# tumshuq
box(0.0, 1.38, 1.06, 0.12, 0.12, 0.12, "dark")
# quloqlar
box(-0.12, 1.55, 0.80, 0.05, 0.12, 0.08, "fur")
box( 0.12, 1.55, 0.80, 0.05, 0.12, 0.08, "fur")
# shoxlar
for sgn in (-1, 1):
    box(sgn * 0.09, 1.66, 0.80, 0.04, 0.26, 0.04, "antler")
    box(sgn * 0.14, 1.78, 0.80, 0.16, 0.04, 0.04, "antler")
    box(sgn * 0.20, 1.86, 0.80, 0.04, 0.16, 0.04, "antler")
# oyoqlar
for x in (-0.14, 0.14):
    for z in (-0.40, 0.40):
        box(x, 0.36, z, 0.10, 0.72, 0.10, "dark")
# dum
box(0.0, 1.05, -0.58, 0.08, 0.10, 0.10, "light")

os.makedirs(OUT, exist_ok=True)
with io.open(os.path.join(OUT, "deer.mtl"), "w", encoding="utf-8") as f:
    f.write("# Kiyik — protsedural\n")
    for name, kd in (("fur", "0.55 0.38 0.22"), ("dark", "0.28 0.20 0.13"),
                     ("antler", "0.78 0.72 0.60"), ("light", "0.85 0.80 0.72")):
        f.write("newmtl %s\nKd %s\nKa 0.2 0.2 0.2\nKs 0.05 0.05 0.05\nNs 8\n\n" % (name, kd))
with io.open(os.path.join(OUT, "deer.obj"), "w", encoding="utf-8") as f:
    f.write("# Kiyik — tools/make_deer.py\nmtllib deer.mtl\n")
    for v in verts: f.write("v %.4f %.4f %.4f\n" % v)
    for n in norms: f.write("vn %.4f %.4f %.4f\n" % n)
    f.write("vt 0 0\n")
    cur = None
    for vi, ni, mat in faces:
        if mat != cur:
            f.write("usemtl %s\n" % mat); cur = mat
        a, b, c, d = vi
        f.write("f %d/1/%d %d/1/%d %d/1/%d\n" % (a, ni, b, ni, c, ni))
        f.write("f %d/1/%d %d/1/%d %d/1/%d\n" % (a, ni, c, ni, d, ni))
print("deer.obj: %d verteks, %d uchburchak" % (len(verts), len(faces) * 2))
