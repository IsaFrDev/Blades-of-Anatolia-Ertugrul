# Ertugrul: M_ErtVertexColor materialini protsedural PBR naqshlar bilan qayta yaratish
# Uslub vertex rangining alfa kanali orqali tanlanadi (dunyo koordinatalari, UV kerak emas):
#   A ~ 0.0 tuproq/o't, 0.2 tosh bloklar, 0.4 yog'och taxta, 0.6 tom cherepitsa, 0.8 g'isht, 1.0 oddiy (yengil don)
import unreal

AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
MAT_DIR = "/Game/Ertugrul/Materials"
P = MAT_DIR + "/M_ErtVertexColor"


def log(s):
    unreal.log("[ErtMakePBR] " + s)


PRE = r"""
float3 Pm = WP / 100.0;
float3 an = abs(N);
int pl = (an.z >= an.x && an.z >= an.y) ? 2 : (an.x >= an.y ? 0 : 1);
float2 uv = (pl == 2) ? Pm.xy : ((pl == 0) ? Pm.yz : Pm.xz);
float s = VC.a;
#define H2(p) frac(sin(dot(floor(p), float2(127.1, 311.7))) * 43758.5453)
#define SM(x) ((x) * (x) * (3.0 - 2.0 * (x)))
#define VN(p) lerp(lerp(H2(p), H2((p) + float2(1, 0)), SM(frac((p).x))), lerp(H2((p) + float2(0, 1)), H2((p) + float2(1, 1)), SM(frac((p).x))), SM(frac((p).y)))
int st = (s < 0.05) ? 0 : (s < 0.108) ? 1 : (s < 0.15) ? 14 : (s < 0.24) ? 2 : (s < 0.30) ? 11 : (s < 0.36) ? 3 : (s < 0.46) ? 4 : (s < 0.56) ? 5 : (s < 0.62) ? 6 : (s < 0.66) ? 12 : (s < 0.76) ? 7 : (s < 0.86) ? 8 : (s < 0.91) ? 9 : (s < 0.935) ? 15 : (s < 0.96) ? 16 : (s < 0.99) ? 13 : 10;   // 0 tuproq,1 mato,2 tosh,3 charm,4 yog'och,5 metall,6 tom,7 jun,8 g'isht,9 badan,10 oddiy,11 qoya,12 po'stloq,13 barg,14 kigiz
float2 bs = (st == 2) ? float2(1.2, 0.6) : (st == 4) ? float2(3.0, 0.24) : (st == 6) ? float2(0.32, 0.36) : (st == 8) ? float2(0.30, 0.10) : float2(1, 1);
float2 mw = (st == 2) ? float2(0.06, 0.10) : (st == 4) ? float2(0.01, 0.07) : (st == 6) ? float2(0.08, 0.05) : float2(0.08, 0.18);
#define CQ(p) float2((p).x + ((frac(floor((p).y / bs.y) * 0.5) > 0.25) ? bs.x * 0.5 : 0.0), (p).y)
#define FR(p) frac(CQ(p) / bs)
#define CH(p) H2(CQ(p) / bs)
#define MORT(p) (((FR(p)).x < mw.x || (FR(p)).y < mw.y) ? 1.0 : 0.0)
#define HGROUND(p) (VN((p) * 3.0) * 0.5 + VN((p) * 11.0) * 0.3 + VN((p) * 40.0) * 0.2)
#define HSTONE(p) ((1.0 - MORT(p)) * (0.65 + 0.35 * CH(p)) + 0.15 * VN((p) * 20.0))
#define HWOOD(p) ((1.0 - MORT(p)) * 0.9 + 0.12 * VN(float2((p).x * 2.0, (p).y * 70.0)))
#define HROOF(p) ((FR(p)).y * 0.75 + 0.25 * cos(((FR(p)).x - 0.5) * 3.1416) * (1.0 - MORT(p)))
#define HPLAIN(p) (0.1 * VN((p) * 30.0))
#define HCLOTH(p) (0.5 + 0.5 * sin((p).x * 400.0) * sin((p).y * 400.0))
#define HLEATHER(p) (VN((p) * 60.0) * 0.6 + VN((p) * 220.0) * 0.4)
#define HMETAL(p) (smoothstep(0.16, 0.10, abs(length(frac((p) * 45.0) - 0.5) - 0.32)))
#define HFUR(p) (VN(float2((p).x * 12.0, (p).y * 90.0)) * 0.5 + VN((p) * 30.0) * 0.5)
#define HSKIN(p) (VN((p) * 90.0) * 0.5 + VN((p) * 320.0) * 0.5)
#define HROCK(p) (VN((p) * 1.5) * 0.45 + VN((p) * 6.0) * 0.3 + VN((p) * 24.0) * 0.25 - 0.5 * (1.0 - smoothstep(0.0, 0.04, abs(VN((p) * 3.0) - 0.5))))
#define HBARK(p) (VN(float2((p).x * 28.0, (p).y * 2.5)) * 0.65 + VN((p) * 45.0) * 0.35)
#define HLEAF(p) (VN((p) * 22.0) * 0.5 + VN((p) * 85.0) * 0.5)
#define HFELT(p) (VN((p) * 35.0) * 0.6 + VN((p) * 140.0) * 0.4)
#define HMUD(p) (VN((p) * 5.0) * 0.5 + VN((p) * 22.0) * 0.3 + VN((p) * 80.0) * 0.2 - 0.35 * (1.0 - smoothstep(0.0, 0.03, abs(VN((p) * 4.0) - 0.5))))
#define CBF(p) (frac((p) / 0.28) - 0.5 + (H2((p) / 0.28) - 0.5) * 0.35)
#define HCOB(p) (smoothstep(0.46, 0.22, length(CBF(p))) * (0.7 + 0.3 * H2((p) / 0.28)) + 0.08 * VN((p) * 60.0))
#define HF(p) ((st == 0) ? HGROUND(p) : (st == 1) ? HCLOTH(p) : (st == 2 || st == 8) ? HSTONE(p) : (st == 3) ? HLEATHER(p) : (st == 4) ? HWOOD(p) : (st == 5) ? HMETAL(p) : (st == 6) ? HROOF(p) : (st == 7) ? HFUR(p) : (st == 9) ? HSKIN(p) : (st == 11) ? HROCK(p) : (st == 12) ? HBARK(p) : (st == 13) ? HLEAF(p) : (st == 14) ? HFELT(p) : (st == 15) ? HMUD(p) : (st == 16) ? HCOB(p) : HPLAIN(p))
"""

CODE_COLOR = PRE + r"""
float h = HF(uv);
float3 col = VC.rgb;
float m;
if (st == 0) { col *= 0.72 + 0.55 * h; }
else if (st == 1) { col *= 0.88 + 0.18 * h + 0.06 * (VN(uv * 25.0) - 0.5); }
else if (st == 2 || st == 8) { m = MORT(uv); col *= lerp(0.78 + 0.4 * CH(uv) + 0.2 * (VN(uv * 20.0) - 0.5), 0.5, m); }
else if (st == 3) { col *= 0.78 + 0.35 * h; }
else if (st == 4) { m = MORT(uv); col *= lerp(0.8 + 0.35 * CH(uv) + 0.2 * (VN(float2(uv.x * 2.0, uv.y * 70.0)) - 0.5), 0.45, m); }
else if (st == 5) { col *= 0.78 + 0.3 * h + 0.1 * (VN(uv * 30.0) - 0.5); }
else if (st == 6) { m = MORT(uv); col *= lerp(0.62 + 0.5 * FR(uv).y + 0.15 * CH(uv), 0.5, m); }
else if (st == 7) { col *= 0.86 + 0.22 * h; }
else if (st == 9) { col *= 0.94 + 0.12 * h; }
else if (st == 11) { col *= 0.62 + 0.55 * saturate(h + 0.25) ; }
else if (st == 12) { col *= 0.68 + 0.5 * h; }
else if (st == 13) { col *= 0.78 + 0.45 * h; }
else if (st == 14) { col *= 0.9 + 0.18 * h; }
else if (st == 15) { col *= 0.72 + 0.4 * saturate(h + 0.2); }
else if (st == 16) { col *= 0.5 + 0.55 * h; }
else { col *= 0.93 + 0.14 * VN(uv * 30.0); }
return float4(saturate(col), h);
"""

CODE_NORMAL = PRE + r"""
float e = (st == 1 || st == 3 || st == 5 || st == 7 || st == 9 || st == 13 || st == 14) ? 0.002 : (st == 11) ? 0.03 : 0.015;
float bump = (st == 0) ? 0.06 : (st == 1) ? 0.004 : (st == 2) ? 0.10 : (st == 3) ? 0.012 : (st == 4) ? 0.05 : (st == 5) ? 0.03 : (st == 6) ? 0.12 : (st == 7) ? 0.02 : (st == 8) ? 0.08 : (st == 9) ? 0.004 : (st == 11) ? 0.16 : (st == 12) ? 0.06 : (st == 13) ? 0.03 : (st == 14) ? 0.01 : (st == 15) ? 0.05 : (st == 16) ? 0.12 : 0.02;
float h0 = HF(uv);
float hx = HF(uv + float2(e, 0));
float hy = HF(uv + float2(0, e));
float dx = (hx - h0) / e * bump, dy = (hy - h0) / e * bump;
float3 t = normalize(float3(-dx, -dy, 1.0));
float3 n;
if (pl == 2) n = float3(t.x, t.y, t.z * sign(N.z));
else if (pl == 0) n = float3(t.z * sign(N.x), t.x, t.y);
else n = float3(t.x, t.z * sign(N.y), t.y);
return normalize(n);
"""


CODE_ROUGH = r"""
float s = VC.a;
if (s >= 0.46 && s < 0.56) return 0.45 + 0.25 * H;      // metall
if (s >= 0.05 && s < 0.15) return 0.97;                 // mato
if (s >= 0.30 && s < 0.36) return 0.55 + 0.25 * H;      // charm
if (s >= 0.86 && s < 0.91) return 0.55 + 0.15 * H;      // badan
if (s >= 0.66 && s < 0.76) return 0.92;                 // jun
if (s >= 0.96 && s < 0.99) return 0.75 + 0.15 * H;      // barg
if (s >= 0.108 && s < 0.15) return 0.98;                // kigiz
return 0.95 - 0.25 * H;
"""

CODE_METAL = r"""
float s = VC.a;
return (s >= 0.46 && s < 0.56) ? 0.7 : 0.0;
"""


def custom(m, code, name, x, y, out_type):
    cu = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    cu.set_editor_property("code", code)
    cu.set_editor_property("description", name)
    cu.set_editor_property("output_type", out_type)
    ins = []
    for nm in ["WP", "N", "VC"]:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", nm)
        ins.append(ci)
    cu.set_editor_property("inputs", ins)
    return cu


def make():
    if EAL.does_asset_exist(P):
        EAL.delete_asset(P)
        log("eski material o'chirildi")
    m = AT.create_asset("M_ErtVertexColor", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("tangent_space_normal", False)
    wp = MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1200, -200)
    nw = MEL.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -1200, 0)
    vc = MEL.create_material_expression(m, unreal.MaterialExpressionVertexColor, -1200, 200)
    ccol = custom(m, CODE_COLOR, "ErtColorHeight", -800, -100, unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    cnrm = custom(m, CODE_NORMAL, "ErtNormal", -800, 300, unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    vc4 = MEL.create_material_expression(m, unreal.MaterialExpressionAppendVector, -1000, 200)
    MEL.connect_material_expressions(vc, "", vc4, "A")
    MEL.connect_material_expressions(vc, "A", vc4, "B")
    for cu in (ccol, cnrm):
        MEL.connect_material_expressions(wp, "", cu, "WP")
        MEL.connect_material_expressions(nw, "", cu, "N")
        MEL.connect_material_expressions(vc4, "", cu, "VC")
    rgb = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -450, -150)
    rgb.set_editor_property("r", True); rgb.set_editor_property("g", True); rgb.set_editor_property("b", True); rgb.set_editor_property("a", False)
    MEL.connect_material_expressions(ccol, "", rgb, "")
    MEL.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
    ha = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -450, 0)
    ha.set_editor_property("r", False); ha.set_editor_property("g", False); ha.set_editor_property("b", False); ha.set_editor_property("a", True)
    MEL.connect_material_expressions(ccol, "", ha, "")
    # Roughness va metallik: uslubga bog'liq
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
    MEL.connect_material_property(cmetal, "", unreal.MaterialProperty.MP_METALLIC)
    spec = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 200)
    spec.set_editor_property("r", 0.35)
    MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    MEL.connect_material_property(cnrm, "", unreal.MaterialProperty.MP_NORMAL)
    MEL.recompile_material(m)
    ok = EAL.save_asset(P)
    log("PBR material yaratildi, saqlandi=%s" % ok)


make()
