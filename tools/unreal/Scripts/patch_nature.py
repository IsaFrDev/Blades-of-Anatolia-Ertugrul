# -*- coding: utf-8 -*-
import io, re
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

h = load('ErtProcMesh.h')
h = rep(h, "// personaj/ot: to'qima, charm, metall/zanjir, jun, badan", "// personaj/ot: to'qima, charm, metall/zanjir, jun, badan\n\tconstexpr float StyleFelt = 0.12f, StyleRock = 0.27f, StyleBark = 0.64f, StyleLeaf = 0.97f;   // tabiat: kigiz, qoya, po'stloq, barg")
save('ErtProcMesh.h', h)

c = load('ErtWorldBuilder.cpp')
c = rep(c, "\tconst FLinearColor Felt(0.86f, 0.82f, 0.72f), FeltDark(0.24f, 0.22f, 0.20f), Cream(0.90f, 0.86f, 0.74f);",
        "\tconst FLinearColor Felt = ErtCol::Sty(FLinearColor(0.86f, 0.82f, 0.72f), ErtCol::StyleFelt), FeltDark = ErtCol::Sty(FLinearColor(0.24f, 0.22f, 0.20f), ErtCol::StyleFelt), Cream = ErtCol::Sty(FLinearColor(0.90f, 0.86f, 0.74f), ErtCol::StyleFelt);")
c = c.replace("Basalt * 1.4f", "ErtCol::Sty(Basalt * 1.4f, ErtCol::StyleStone)")
c = c.replace("ErtCol::Vary(H > 150.f ? FLinearColor(0.7f, 0.7f, 0.72f) : Stone, 0.12f, Placed)", "ErtCol::Vary(ErtCol::Sty(H > 150.f ? FLinearColor(0.7f, 0.7f, 0.72f) : FLinearColor(0.43f, 0.41f, 0.38f), ErtCol::StyleRock), 0.12f, Placed)")
c = rep(c, "\tconst FLinearColor Trunk = ErtCol::Vary(FLinearColor(0.30f, 0.20f, 0.11f), 0.15f, S);", "\tconst FLinearColor Trunk = ErtCol::Vary(ErtCol::Sty(FLinearColor(0.30f, 0.20f, 0.11f), ErtCol::StyleBark), 0.15f, S);")
c = rep(c, "\t\tconst FLinearColor Leaf = ErtCol::Vary(FLinearColor(0.09f, 0.24f, 0.10f), 0.18f, S + 7);", "\t\tconst FLinearColor Leaf = ErtCol::Vary(ErtCol::Sty(FLinearColor(0.09f, 0.24f, 0.10f), ErtCol::StyleLeaf), 0.18f, S + 7);")
c = rep(c, "\t\tconst FLinearColor Leaf = ErtCol::Vary(FLinearColor(0.20f, 0.38f, 0.12f), 0.2f, S + 7);", "\t\tconst FLinearColor Leaf = ErtCol::Vary(ErtCol::Sty(FLinearColor(0.20f, 0.38f, 0.12f), ErtCol::StyleLeaf), 0.2f, S + 7);")
c = c.replace("Leaf * 1.1f", "ErtCol::Sty(Leaf * 1.1f, ErtCol::StyleLeaf)")
c = rep(c, "\tconst FLinearColor Trunk(0.42f, 0.32f, 0.18f), Leaf(0.16f, 0.40f, 0.14f);", "\tconst FLinearColor Trunk = ErtCol::Sty(FLinearColor(0.42f, 0.32f, 0.18f), ErtCol::StyleBark), Leaf = ErtCol::Sty(FLinearColor(0.16f, 0.40f, 0.14f), ErtCol::StyleLeaf);")
c = c.replace("AddYurt(M, OE(u), ON(v), Z, 2.5f, 1.8f, 1.7f, Felt, FLinearColor(0.72f, 0.66f, 0.55f), DoorYaw, ++K);", "AddYurt(M, OE(u), ON(v), Z, 2.5f, 1.8f, 1.7f, Felt, ErtCol::Sty(FLinearColor(0.72f, 0.66f, 0.55f), ErtCol::StyleFelt), DoorYaw, ++K);")
c = c.replace("AddYurt(M, KE(0), KN(0), Z, 8.f, 3.f, 4.2f, FLinearColor(0.38f, 0.10f, 0.09f), FLinearColor(0.22f, 0.08f, 0.07f), -90.f, ++K);", "AddYurt(M, KE(0), KN(0), Z, 8.f, 3.f, 4.2f, ErtCol::Sty(FLinearColor(0.38f, 0.10f, 0.09f), ErtCol::StyleFelt), ErtCol::Sty(FLinearColor(0.22f, 0.08f, 0.07f), ErtCol::StyleFelt), -90.f, ++K);")
c = c.replace("FeltDark, FeltDark * 0.85f,", "FeltDark, ErtCol::Sty(FeltDark * 0.85f, ErtCol::StyleFelt),")
c = c.replace("ErtCol::Vary(FLinearColor(0.30f, 0.52f, 0.18f), 0.1f, ++S)", "ErtCol::Vary(ErtCol::Sty(FLinearColor(0.30f, 0.52f, 0.18f), ErtCol::StyleLeaf), 0.1f, ++S)")
# Shahar daraxtlari: e'lon nomlari bo'yicha
leaf = "Leaf Plane Cyp Poplar Pine Fruit WillowL".split(); bark = "Trunk WillowT".split()
style = {n: "StyleLeaf" for n in leaf}; style.update({n: "StyleBark" for n in bark})
names = "|".join(sorted(style, key=len, reverse=True))
pat = re.compile(r"\b(" + names + r")\((\s*[-0-9.f]+\s*,\s*[-0-9.f]+\s*,\s*[-0-9.f]+\s*)\)")
out = []; cnt = 0
for line in c.split("\n"):
    if "const FLinearColor" in line:
        def sub(m):
            global cnt; cnt += 1
            return "%s = ErtCol::Sty(FLinearColor(%s), ErtCol::%s)" % (m.group(1), m.group(2), style[m.group(1)])
        line = pat.sub(sub, line)
    out.append(line)
c = "\n".join(out)
# Trunk * 0.8f (tut) alfani siljitadi
c = c.replace("Trunk * 0.8f", "ErtCol::Sty(Trunk * 0.8f, ErtCol::StyleBark)")
save('ErtWorldBuilder.cpp', c)
print("tree decls", cnt)

# Material: yangi diapazonlar
p = 'D:/temp/claude/ert_make_pbr.py'
m = io.open(p, encoding='utf-8').read()
m = rep(m, "int st = (s < 0.05) ? 0 : (s < 0.15) ? 1 : (s < 0.30) ? 2 : (s < 0.36) ? 3 : (s < 0.46) ? 4 : (s < 0.56) ? 5 : (s < 0.66) ? 6 : (s < 0.76) ? 7 : (s < 0.86) ? 8 : (s < 0.96) ? 9 : 10;   // 0 tuproq,1 mato,2 tosh,3 charm,4 yog'och,5 metall,6 tom,7 jun,8 g'isht,9 badan,10 oddiy",
        "int st = (s < 0.05) ? 0 : (s < 0.108) ? 1 : (s < 0.15) ? 14 : (s < 0.24) ? 2 : (s < 0.30) ? 11 : (s < 0.36) ? 3 : (s < 0.46) ? 4 : (s < 0.56) ? 5 : (s < 0.62) ? 6 : (s < 0.66) ? 12 : (s < 0.76) ? 7 : (s < 0.86) ? 8 : (s < 0.96) ? 9 : (s < 0.99) ? 13 : 10;   // 0 tuproq,1 mato,2 tosh,3 charm,4 yog'och,5 metall,6 tom,7 jun,8 g'isht,9 badan,10 oddiy,11 qoya,12 po'stloq,13 barg,14 kigiz")
m = rep(m, "#define HSKIN(p) (VN((p) * 90.0) * 0.5 + VN((p) * 320.0) * 0.5)",
        "#define HSKIN(p) (VN((p) * 90.0) * 0.5 + VN((p) * 320.0) * 0.5)\n#define HROCK(p) (VN((p) * 1.5) * 0.45 + VN((p) * 6.0) * 0.3 + VN((p) * 24.0) * 0.25 - 0.5 * (1.0 - smoothstep(0.0, 0.04, abs(VN((p) * 3.0) - 0.5))))\n#define HBARK(p) (VN(float2((p).x * 28.0, (p).y * 2.5)) * 0.65 + VN((p) * 45.0) * 0.35)\n#define HLEAF(p) (VN((p) * 22.0) * 0.5 + VN((p) * 85.0) * 0.5)\n#define HFELT(p) (VN((p) * 35.0) * 0.6 + VN((p) * 140.0) * 0.4)")
m = rep(m, "(st == 9) ? HSKIN(p) : HPLAIN(p))", "(st == 9) ? HSKIN(p) : (st == 11) ? HROCK(p) : (st == 12) ? HBARK(p) : (st == 13) ? HLEAF(p) : (st == 14) ? HFELT(p) : HPLAIN(p))")
m = rep(m, "else if (st == 9) { col *= 0.94 + 0.12 * h; }",
        "else if (st == 9) { col *= 0.94 + 0.12 * h; }\nelse if (st == 11) { col *= 0.62 + 0.55 * saturate(h + 0.25) ; }\nelse if (st == 12) { col *= 0.68 + 0.5 * h; }\nelse if (st == 13) { col *= 0.78 + 0.45 * h; }\nelse if (st == 14) { col *= 0.9 + 0.18 * h; }")
m = rep(m, "(st == 9) ? 0.004 : 0.02;", "(st == 9) ? 0.004 : (st == 11) ? 0.16 : (st == 12) ? 0.06 : (st == 13) ? 0.03 : (st == 14) ? 0.01 : 0.02;")
m = rep(m, "float e = (st == 1 || st == 3 || st == 5 || st == 7 || st == 9) ? 0.002 : 0.015;", "float e = (st == 1 || st == 3 || st == 5 || st == 7 || st == 9 || st == 13 || st == 14) ? 0.002 : (st == 11) ? 0.03 : 0.015;")
m = rep(m, "if (s >= 0.66 && s < 0.76) return 0.92;                 // jun", "if (s >= 0.66 && s < 0.76) return 0.92;                 // jun\nif (s >= 0.96 && s < 0.99) return 0.75 + 0.15 * H;      // barg\nif (s >= 0.108 && s < 0.15) return 0.98;                // kigiz")
io.open(p, 'w', encoding='utf-8', newline='\n').write(m)
print('patched')
