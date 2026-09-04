# -*- coding: utf-8 -*-
import io, re
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

# 1) Yangi uslub konstantalari
h = load('ErtProcMesh.h')
h = rep(h, "\tconstexpr float StyleGround = 0.0f, StyleStone = 0.2f, StyleWood = 0.4f, StyleRoof = 0.6f, StyleBrick = 0.8f, StylePlain = 1.0f;",
        "\tconstexpr float StyleGround = 0.0f, StyleStone = 0.2f, StyleWood = 0.4f, StyleRoof = 0.6f, StyleBrick = 0.8f, StylePlain = 1.0f;\n\tconstexpr float StyleCloth = 0.10f, StyleLeather = 0.32f, StyleMetal = 0.50f, StyleFur = 0.70f, StyleSkin = 0.90f;   // personaj/ot: to'qima, charm, metall/zanjir, jun, badan")
save('ErtProcMesh.h', h)

# 2) Qahramon tanasi: a'zo ranglarini uslubli mahalliy nusxalar bilan almashtirish
c = load('ErtHeroBody.cpp')
LOCALS = "\tconst FLinearColor KaftanS = ErtCol::Sty(Kaftan, ErtCol::StyleCloth), TrousersS = ErtCol::Sty(Trousers, ErtCol::StyleCloth), LeatherS = ErtCol::Sty(Leather, ErtCol::StyleLeather), SkinS = ErtCol::Sty(Skin, ErtCol::StyleSkin), SteelS = ErtCol::Sty(Steel, ErtCol::StyleMetal), FurS = ErtCol::Sty(Fur, ErtCol::StyleFur), BeardS = ErtCol::Sty(Beard, ErtCol::StyleFur), TrimS = ErtCol::Sty(Trim, ErtCol::StyleMetal);\n"
start = c.index("void UErtHeroBody::Build(")
end = c.index("void UErtHeroBody::TriggerAttack")
body = c[start:end]
if "KaftanS" not in body:
    lines = body.split("\n")
    out = []
    for ln in lines:
        if ln.strip() == "FErtMeshData M;":
            out.append(ln); out.append(LOCALS.rstrip("\n")); continue
        if "Steel = Tier >= 2" in ln:
            out.append(ln); continue
        for nm in ["Kaftan", "Trousers", "Leather", "Skin", "Steel", "Fur", "Beard", "Trim"]:
            ln = re.sub(r"\b%s\b(?!S\b)" % nm, nm + "S", ln)
        out.append(ln)
    body = "\n".join(out)
    body = body.replace("M.AddCylinder(FVector(0, 0, 0), 22.f, 22.f, 2.5f, 12, FLinearColor(0.35f, 0.22f, 0.10f), true, FRotator(0, 0, 90.f));",
                        "M.AddCylinder(FVector(0, 0, 0), 22.f, 22.f, 2.5f, 12, ErtCol::Sty(FLinearColor(0.35f, 0.22f, 0.10f), ErtCol::StyleWood), true, FRotator(0, 0, 90.f));")
    c = c[:start] + body + c[end:]
save('ErtHeroBody.cpp', c)

# 3) Ot va tuya
c = load('ErtHorse.cpp')
c = rep(c, "\t\tconst FLinearColor Sand = Coat, Dark = Coat * 0.6f, Leather(0.30f, 0.18f, 0.09f), Cloth(0.55f, 0.15f, 0.12f), Tassel(0.85f, 0.7f, 0.2f);",
        "\t\tconst FLinearColor Sand = ErtCol::Sty(Coat, ErtCol::StyleFur), Dark = ErtCol::Sty(Coat * 0.6f, ErtCol::StyleFur), Leather = ErtCol::Sty(FLinearColor(0.30f, 0.18f, 0.09f), ErtCol::StyleLeather), Cloth = ErtCol::Sty(FLinearColor(0.55f, 0.15f, 0.12f), ErtCol::StyleCloth), Tassel(0.85f, 0.7f, 0.2f);")
c = rep(c, "\tconst FLinearColor Dark = bDarkPoints ? FLinearColor(0.08f, 0.06f, 0.05f) : Coat * 0.6f;\n\tconst FLinearColor Mane = bDarkPoints ? FLinearColor(0.07f, 0.05f, 0.04f) : Coat * 0.45f;",
        "\tconst FLinearColor CoatF = ErtCol::Sty(Coat, ErtCol::StyleFur);\n\tconst FLinearColor Dark = ErtCol::Sty(bDarkPoints ? FLinearColor(0.08f, 0.06f, 0.05f) : Coat * 0.6f, ErtCol::StyleFur);\n\tconst FLinearColor Mane = ErtCol::Sty(bDarkPoints ? FLinearColor(0.07f, 0.05f, 0.04f) : Coat * 0.45f, ErtCol::StyleFur);")
c = rep(c, "\tconst FLinearColor Hoof(0.16f, 0.13f, 0.11f), Leather(0.30f, 0.18f, 0.09f), LeatherD(0.20f, 0.12f, 0.06f), Cloth(0.55f, 0.12f, 0.10f), ClothTrim(0.85f, 0.7f, 0.25f), Iron(0.6f, 0.6f, 0.62f), Eye(0.05f, 0.04f, 0.04f), WhiteM(0.92f, 0.9f, 0.86f);",
        "\tconst FLinearColor Hoof(0.16f, 0.13f, 0.11f), Leather = ErtCol::Sty(FLinearColor(0.30f, 0.18f, 0.09f), ErtCol::StyleLeather), LeatherD = ErtCol::Sty(FLinearColor(0.20f, 0.12f, 0.06f), ErtCol::StyleLeather), Cloth = ErtCol::Sty(FLinearColor(0.55f, 0.12f, 0.10f), ErtCol::StyleCloth), ClothTrim = ErtCol::Sty(FLinearColor(0.85f, 0.7f, 0.25f), ErtCol::StyleCloth), Iron = ErtCol::Sty(FLinearColor(0.6f, 0.6f, 0.62f), ErtCol::StyleMetal), Eye(0.05f, 0.04f, 0.04f), WhiteM = ErtCol::Sty(FLinearColor(0.92f, 0.9f, 0.86f), ErtCol::StyleFur);")
# Realistik ot bo'limida Coat -> CoatF (ta'riflardan tashqari)
start = c.index("\tconst FLinearColor CoatF = ErtCol::Sty(Coat, ErtCol::StyleFur);")
end = c.index("void AErtHorse::ApplyDamage(float D)")
body = c[start:end]
lines = body.split("\n"); out = []
for ln in lines:
    if "const FLinearColor CoatF" in ln or "const FLinearColor Dark = ErtCol::Sty(bDarkPoints" in ln or "const FLinearColor Mane = ErtCol::Sty(bDarkPoints" in ln:
        out.append(ln); continue
    out.append(re.sub(r"\bCoat\b(?!F\b)", "CoatF", ln))
c = c[:start] + "\n".join(out) + c[end:]
# Coat * 0.92f (paypoqsiz bilak) alfani siljitadi -> Sty
c = c.replace("(bDarkPoints ? Dark : CoatF * 0.92f)", "(bDarkPoints ? Dark : ErtCol::Sty(CoatF * 0.92f, ErtCol::StyleFur))")
save('ErtHorse.cpp', c)

# 4) Kiyik
c = load('ErtEnemy.cpp')
c = rep(c, "\tconst FLinearColor Coat(0.55f, 0.36f, 0.18f), Belly(0.75f, 0.62f, 0.45f), Dark(0.25f, 0.16f, 0.08f);",
        "\tconst FLinearColor Coat = ErtCol::Sty(FLinearColor(0.55f, 0.36f, 0.18f), ErtCol::StyleFur), Belly = ErtCol::Sty(FLinearColor(0.75f, 0.62f, 0.45f), ErtCol::StyleFur), Dark(0.25f, 0.16f, 0.08f);")
c = c.replace("M.AddBox(FVector(0, 0, -38), FVector(5, 4.5f, 38), i < 2 ? Coat : Coat * 0.95f);", "M.AddBox(FVector(0, 0, -38), FVector(5, 4.5f, 38), i < 2 ? Coat : ErtCol::Sty(Coat * 0.95f, ErtCol::StyleFur));")
if '#include "ErtProcMesh.h"' not in c:
    c = c.replace('#include "ErtEnemy.h"\n', '#include "ErtEnemy.h"\n#include "ErtProcMesh.h"\n', 1)
save('ErtEnemy.cpp', c)

# 5) Material skripti: yangi diapazonlar, metallik va roughness tugunlari
p = 'D:/temp/claude/ert_make_pbr.py'
m = io.open(p, encoding='utf-8').read()
m = rep(m, "float2 bs = (s < 0.1) ? float2(1, 1) : (s < 0.3) ? float2(1.2, 0.6) : (s < 0.5) ? float2(3.0, 0.24) : (s < 0.7) ? float2(0.32, 0.36) : (s < 0.9) ? float2(0.30, 0.10) : float2(1, 1);",
        "int st = (s < 0.05) ? 0 : (s < 0.15) ? 1 : (s < 0.30) ? 2 : (s < 0.36) ? 3 : (s < 0.46) ? 4 : (s < 0.56) ? 5 : (s < 0.66) ? 6 : (s < 0.76) ? 7 : (s < 0.86) ? 8 : (s < 0.96) ? 9 : 10;   // 0 tuproq,1 mato,2 tosh,3 charm,4 yog'och,5 metall,6 tom,7 jun,8 g'isht,9 badan,10 oddiy\nfloat2 bs = (st == 2) ? float2(1.2, 0.6) : (st == 4) ? float2(3.0, 0.24) : (st == 6) ? float2(0.32, 0.36) : (st == 8) ? float2(0.30, 0.10) : float2(1, 1);")
m = rep(m, "float2 mw = (s < 0.3) ? float2(0.06, 0.10) : (s < 0.5) ? float2(0.01, 0.07) : (s < 0.7) ? float2(0.08, 0.05) : float2(0.08, 0.18);",
        "float2 mw = (st == 2) ? float2(0.06, 0.10) : (st == 4) ? float2(0.01, 0.07) : (st == 6) ? float2(0.08, 0.05) : float2(0.08, 0.18);")
m = rep(m, "#define HPLAIN(p) (0.1 * VN((p) * 30.0))\n#define HF(p) ((s < 0.1) ? HGROUND(p) : (s < 0.3) ? HSTONE(p) : (s < 0.5) ? HWOOD(p) : (s < 0.7) ? HROOF(p) : (s < 0.9) ? HSTONE(p) : HPLAIN(p))",
        "#define HPLAIN(p) (0.1 * VN((p) * 30.0))\n#define HCLOTH(p) (0.5 + 0.5 * sin((p).x * 400.0) * sin((p).y * 400.0))\n#define HLEATHER(p) (VN((p) * 60.0) * 0.6 + VN((p) * 220.0) * 0.4)\n#define HMETAL(p) (smoothstep(0.16, 0.10, abs(length(frac((p) * 45.0) - 0.5) - 0.32)))\n#define HFUR(p) (VN(float2((p).x * 9.0, (p).y * 140.0)) * 0.6 + VN((p) * 40.0) * 0.4)\n#define HSKIN(p) (VN((p) * 90.0) * 0.5 + VN((p) * 320.0) * 0.5)\n#define HF(p) ((st == 0) ? HGROUND(p) : (st == 1) ? HCLOTH(p) : (st == 2 || st == 8) ? HSTONE(p) : (st == 3) ? HLEATHER(p) : (st == 4) ? HWOOD(p) : (st == 5) ? HMETAL(p) : (st == 6) ? HROOF(p) : (st == 7) ? HFUR(p) : (st == 9) ? HSKIN(p) : HPLAIN(p))")
m = rep(m, '''if (s < 0.1) { col *= 0.72 + 0.55 * h; }
else if (s < 0.3 || (s >= 0.7 && s < 0.9)) { m = MORT(uv); col *= lerp(0.78 + 0.4 * CH(uv) + 0.2 * (VN(uv * 20.0) - 0.5), 0.5, m); }
else if (s < 0.5) { m = MORT(uv); col *= lerp(0.8 + 0.35 * CH(uv) + 0.2 * (VN(float2(uv.x * 2.0, uv.y * 70.0)) - 0.5), 0.45, m); }
else if (s < 0.7) { m = MORT(uv); col *= lerp(0.62 + 0.5 * FR(uv).y + 0.15 * CH(uv), 0.5, m); }
else { col *= 0.93 + 0.14 * VN(uv * 30.0); }''',
        '''if (st == 0) { col *= 0.72 + 0.55 * h; }
else if (st == 1) { col *= 0.88 + 0.18 * h + 0.06 * (VN(uv * 25.0) - 0.5); }
else if (st == 2 || st == 8) { m = MORT(uv); col *= lerp(0.78 + 0.4 * CH(uv) + 0.2 * (VN(uv * 20.0) - 0.5), 0.5, m); }
else if (st == 3) { col *= 0.78 + 0.35 * h; }
else if (st == 4) { m = MORT(uv); col *= lerp(0.8 + 0.35 * CH(uv) + 0.2 * (VN(float2(uv.x * 2.0, uv.y * 70.0)) - 0.5), 0.45, m); }
else if (st == 5) { col *= 0.55 + 0.6 * h + 0.1 * (VN(uv * 30.0) - 0.5); }
else if (st == 6) { m = MORT(uv); col *= lerp(0.62 + 0.5 * FR(uv).y + 0.15 * CH(uv), 0.5, m); }
else if (st == 7) { col *= 0.7 + 0.5 * h; }
else if (st == 9) { col *= 0.94 + 0.12 * h; }
else { col *= 0.93 + 0.14 * VN(uv * 30.0); }''')
m = rep(m, "float bump = (s < 0.1) ? 0.06 : (s < 0.3) ? 0.10 : (s < 0.5) ? 0.05 : (s < 0.7) ? 0.12 : (s < 0.9) ? 0.08 : 0.02;",
        "float bump = (st == 0) ? 0.06 : (st == 1) ? 0.004 : (st == 2) ? 0.10 : (st == 3) ? 0.012 : (st == 4) ? 0.05 : (st == 5) ? 0.03 : (st == 6) ? 0.12 : (st == 7) ? 0.02 : (st == 8) ? 0.08 : (st == 9) ? 0.004 : 0.02;")
m = rep(m, "float e = 0.015;", "float e = (st == 1 || st == 3 || st == 5 || st == 7 || st == 9) ? 0.002 : 0.015;")
# Roughness/metallik: custom tugunlar (H, VC)
m = rep(m, '''    # Roughness = 0.95 - 0.25 * h
    mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 40)
    k = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -450, 90)
    k.set_editor_property("r", -0.25)
    MEL.connect_material_expressions(ha, "", mul, "A")
    MEL.connect_material_expressions(k, "", mul, "B")
    add = MEL.create_material_expression(m, unreal.MaterialExpressionAdd, -150, 40)
    base = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 120)
    base.set_editor_property("r", 0.95)
    MEL.connect_material_expressions(mul, "", add, "A")
    MEL.connect_material_expressions(base, "", add, "B")
    MEL.connect_material_property(add, "", unreal.MaterialProperty.MP_ROUGHNESS)''',
        '''    # Roughness va metallik: uslubga bog'liq
    def custom2(code, name, x, y, out_type):
        cu = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
        cu.set_editor_property("code", code)
        cu.set_editor_property("description", name)
        cu.set_editor_property("output_type", out_type)
        ins = []
        for nm in ["H", "VC"]:
            ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); ins.append(ci)
        cu.set_editor_property("inputs", ins)
        MEL.connect_material_expressions(ha, "", cu, "H")
        MEL.connect_material_expressions(vc4, "", cu, "VC")
        return cu
    crough = custom2(CODE_ROUGH, "ErtRough", -300, 40, unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    MEL.connect_material_property(crough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    cmetal = custom2(CODE_METAL, "ErtMetal", -300, 140, unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    MEL.connect_material_property(cmetal, "", unreal.MaterialProperty.MP_METALLIC)''')
m = rep(m, 'def custom(m, code, name, x, y, out_type):', '''CODE_ROUGH = r"""
float s = VC.a;
if (s >= 0.46 && s < 0.56) return 0.30 + 0.25 * H;      // metall
if (s >= 0.05 && s < 0.15) return 0.97;                 // mato
if (s >= 0.30 && s < 0.36) return 0.55 + 0.25 * H;      // charm
if (s >= 0.86 && s < 0.96) return 0.55 + 0.15 * H;      // badan
if (s >= 0.66 && s < 0.76) return 0.92;                 // jun
return 0.95 - 0.25 * H;
"""

CODE_METAL = r"""
float s = VC.a;
return (s >= 0.46 && s < 0.56) ? 0.9 : 0.0;
"""


def custom(m, code, name, x, y, out_type):''')
io.open(p, 'w', encoding='utf-8', newline='\n').write(m)
print('patched')
