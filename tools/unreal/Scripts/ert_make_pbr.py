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
float2 bs = (s < 0.1) ? float2(1, 1) : (s < 0.3) ? float2(1.2, 0.6) : (s < 0.5) ? float2(3.0, 0.24) : (s < 0.7) ? float2(0.32, 0.36) : (s < 0.9) ? float2(0.30, 0.10) : float2(1, 1);
float2 mw = (s < 0.3) ? float2(0.06, 0.10) : (s < 0.5) ? float2(0.01, 0.07) : (s < 0.7) ? float2(0.08, 0.05) : float2(0.08, 0.18);
#define CQ(p) float2((p).x + ((frac(floor((p).y / bs.y) * 0.5) > 0.25) ? bs.x * 0.5 : 0.0), (p).y)
#define FR(p) frac(CQ(p) / bs)
#define CH(p) H2(CQ(p) / bs)
#define MORT(p) (((FR(p)).x < mw.x || (FR(p)).y < mw.y) ? 1.0 : 0.0)
#define HGROUND(p) (VN((p) * 3.0) * 0.5 + VN((p) * 11.0) * 0.3 + VN((p) * 40.0) * 0.2)
#define HSTONE(p) ((1.0 - MORT(p)) * (0.65 + 0.35 * CH(p)) + 0.15 * VN((p) * 20.0))
#define HWOOD(p) ((1.0 - MORT(p)) * 0.9 + 0.12 * VN(float2((p).x * 2.0, (p).y * 70.0)))
#define HROOF(p) ((FR(p)).y * 0.75 + 0.25 * cos(((FR(p)).x - 0.5) * 3.1416) * (1.0 - MORT(p)))
#define HPLAIN(p) (0.1 * VN((p) * 30.0))
#define HF(p) ((s < 0.1) ? HGROUND(p) : (s < 0.3) ? HSTONE(p) : (s < 0.5) ? HWOOD(p) : (s < 0.7) ? HROOF(p) : (s < 0.9) ? HSTONE(p) : HPLAIN(p))
"""

CODE_COLOR = PRE + r"""
float h = HF(uv);
float3 col = VC.rgb;
float m;
if (s < 0.1) { col *= 0.72 + 0.55 * h; }
else if (s < 0.3 || (s >= 0.7 && s < 0.9)) { m = MORT(uv); col *= lerp(0.78 + 0.4 * CH(uv) + 0.2 * (VN(uv * 20.0) - 0.5), 0.5, m); }
else if (s < 0.5) { m = MORT(uv); col *= lerp(0.8 + 0.35 * CH(uv) + 0.2 * (VN(float2(uv.x * 2.0, uv.y * 70.0)) - 0.5), 0.45, m); }
else if (s < 0.7) { m = MORT(uv); col *= lerp(0.62 + 0.5 * FR(uv).y + 0.15 * CH(uv), 0.5, m); }
else { col *= 0.93 + 0.14 * VN(uv * 30.0); }
return float4(saturate(col), h);
"""

CODE_NORMAL = PRE + r"""
float e = 0.015;
float bump = (s < 0.1) ? 0.06 : (s < 0.3) ? 0.10 : (s < 0.5) ? 0.05 : (s < 0.7) ? 0.12 : (s < 0.9) ? 0.08 : 0.02;
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
    # Roughness = 0.95 - 0.25 * h
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
    MEL.connect_material_property(add, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 200)
    spec.set_editor_property("r", 0.35)
    MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    MEL.connect_material_property(cnrm, "", unreal.MaterialProperty.MP_NORMAL)
    MEL.recompile_material(m)
    ok = EAL.save_asset(P)
    log("PBR material yaratildi, saqlandi=%s" % ok)


make()
