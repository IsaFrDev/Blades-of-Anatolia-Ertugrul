# -*- coding: utf-8 -*-
"""Kigiz rangli o'tov variantlarini yaratadi.

Kenney chodirlarining materiali `colorRed` (0.88, 0.29, 0.31) — yorqin qizil.
Qayi obasi esa OQ KIGIZdan tikilgan: issiq oq, sarg'ish soya, yog'och karkas.
Prop "tint" bu yerda yordam bermaydi (Mesh::draw material rangini glColor bilan
yozadi va display list ga pishiriladi), shuning uchun ALOHIDA material fayli
bilan yangi variant yaratiladi. Geometriya o'zgarmaydi — faqat .mtl boshqa.
"""
import io, os, shutil

SRC = "assets/models/nature"
PAL = {
    "colorRed":     (0.780, 0.720, 0.625),   # kigiz — issiq oq
    "colorRedDark": (0.615, 0.552, 0.470),   # soyadagi kigiz
    "wood":         (0.560, 0.400, 0.268),   # karkas
    "woodBark":     (0.500, 0.352, 0.232),
    "woodDark":     (0.430, 0.300, 0.198),
    "_defaultMat":  (0.760, 0.700, 0.610),
}

names = ["tent_detailedClosed", "tent_detailedOpen",
         "tent_smallClosed", "tent_smallOpen"]

made = 0
for nm in names:
    obj_in  = os.path.join(SRC, nm + ".obj")
    mtl_in  = os.path.join(SRC, nm + ".mtl")
    if not (os.path.exists(obj_in) and os.path.exists(mtl_in)):
        print("o'tkazib yuborildi:", nm)
        continue
    out_nm  = nm.replace("tent_", "yurt_felt_")
    obj_out = os.path.join(SRC, out_nm + ".obj")
    mtl_out = os.path.join(SRC, out_nm + ".mtl")

    # --- .mtl: faqat ranglar almashadi ---
    cur = None
    lines = []
    for ln in io.open(mtl_in, encoding="utf-8", errors="ignore").read().split("\n"):
        st = ln.strip()
        if st.startswith("newmtl"):
            cur = st.split()[1] if len(st.split()) > 1 else None
        elif st.startswith("Kd ") and cur in PAL:
            c = PAL[cur]
            ln = "Kd %.6f %.6f %.6f" % c
        elif st.startswith("Ks "):
            ln = "Ks 0.030000 0.030000 0.030000"     # kigiz yaltiramaydi
        lines.append(ln)
    header = ("# Qayi o'tovi — kigiz rangli variant.\n"
              "# Manba: %s.mtl (Kenney). Faqat Kd ranglari o'zgartirilgan.\n" % nm)
    io.open(mtl_out, "w", encoding="utf-8", newline="\n").write(header + "\n".join(lines))

    # --- .obj: mtllib satrini yangi faylga qaratamiz ---
    obj = io.open(obj_in, encoding="utf-8", errors="ignore").read()
    out = []
    for ln in obj.split("\n"):
        if ln.strip().startswith("mtllib"):
            ln = "mtllib " + out_nm + ".mtl"
        out.append(ln)
    io.open(obj_out, "w", encoding="utf-8", newline="\n").write("\n".join(out))
    made += 1
    print("yaratildi: %s.obj + .mtl" % out_nm)

print("jami: %d ta o'tov varianti" % made)
