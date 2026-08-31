# -*- coding: utf-8 -*-
"""nature/ va town/ modellarining material ranglarini tuzatadi.

MUAMMO: Kenney nature to'plamining bir necha materialida Kd ning R va B
kanallari ALMASHIB ketgan — o't feruza (0.17, 0.85, 0.72), barglar ko'kimtir,
tosh esa och havorang chiqadi. Shu sababli butun daraja ko'k-yashil ko'rinardi.

Nima uchun daraja JSON dagi "tint" bu muammoni yechmaydi: Mesh::draw() har bir
submesh uchun glColor4f(sm.kd) chaqiradi va prop tintini BEKOR qiladi; ustiga
mesh display list ga "pishiriladi", ya'ni ranglar bir marta yozilib qoladi.
Shuning uchun tuzatish material faylining O'ZIDA bo'lishi kerak.

TUZATISH: quyidagi materiallarda R va B almashtiriladi. Iliq materiallar
(yog'och, po'stloq, qizil, sariq) TEGILMAYDI.

Zaxira nusxa: <fayl>.mtl.bak (bir marta, qayta yozilmaydi).
"""
import io, os, glob, shutil

SWAP = {"grass", "leafsGreen", "leafsDark", "stone", "stoneDark"}

roots = ["assets/models/nature", "assets/models/town"]
changed_files = 0
changed_mats = 0
report = {}

for root in roots:
    for path in sorted(glob.glob(os.path.join(root, "*.mtl"))):
        src = io.open(path, encoding="utf-8", errors="ignore").read()
        lines = src.split("\n")
        cur = None
        out = []
        touched = False
        for ln in lines:
            st = ln.strip()
            if st.startswith("newmtl"):
                parts = st.split()
                cur = parts[1] if len(parts) > 1 else None
            elif st.startswith("Kd ") and cur in SWAP:
                p = st.split()
                if len(p) >= 4:
                    r, g, b = float(p[1]), float(p[2]), float(p[3])
                    if b > r:                       # faqat ko'kimtir bo'lsa
                        ln = "Kd %.6f %.6f %.6f" % (b, g, r)
                        touched = True
                        changed_mats += 1
                        report.setdefault(cur, (round(r, 3), round(g, 3), round(b, 3),
                                                round(b, 3), round(g, 3), round(r, 3)))
            out.append(ln)
        if touched:
            bak = path + ".bak"
            if not os.path.exists(bak):
                shutil.copyfile(path, bak)
            io.open(path, "w", encoding="utf-8", newline="\n").write("\n".join(out))
            changed_files += 1

print("tuzatilgan fayllar : %d" % changed_files)
print("tuzatilgan material: %d" % changed_mats)
for k, v in report.items():
    print("  %-12s (%.3f, %.3f, %.3f)  ->  (%.3f, %.3f, %.3f)" % (k,) + "" if False else
          "  %-12s (%.3f, %.3f, %.3f)  ->  (%.3f, %.3f, %.3f)" % (k, v[0], v[1], v[2], v[3], v[4], v[5]))
