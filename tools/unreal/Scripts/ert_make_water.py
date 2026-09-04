# Ertugrul: M_ErtWater - to'lqinli, Fresnel, qirg'oq ko'pigi, sindirish
import unreal

AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
MAT_DIR = "/Game/Ertugrul/Materials"
P = MAT_DIR + "/M_ErtWater"


def log(s):
    unreal.log("[ErtMakeWater] " + s)


CODE_WAVE = r"""
float2 p = WP.xy / 100.0;
float t = T;
#define H2(q) frac(sin(dot(floor(q), float2(127.1, 311.7))) * 43758.5453)
#define SM(x) ((x) * (x) * (3.0 - 2.0 * (x)))
#define VN(q) lerp(lerp(H2(q), H2((q) + float2(1, 0)), SM(frac((q).x))), lerp(H2((q) + float2(0, 1)), H2((q) + float2(1, 1)), SM(frac((q).x))), SM(frac((q).y)))
#define HW(q) (sin((q).x * 1.3 + t * 1.1) * 0.5 + sin(((q).x * 0.7 + (q).y * 1.1) * 1.7 - t * 1.4) * 0.35 + sin(((q).y * 0.9 - (q).x * 0.5) * 3.1 + t * 2.2) * 0.2 + VN((q) * 2.0 + t * 0.15) * 0.7 + VN((q) * 7.0 - t * 0.3) * 0.25)
float e = 0.05;
float h0 = HW(p), hx = HW(p + float2(e, 0)), hy = HW(p + float2(0, e));
float k = 0.045;
float3 n = normalize(float3(-(hx - h0) / e * k, -(hy - h0) / e * k, 1.0));
return n;
"""


def make():
    if EAL.does_asset_exist(P):
        EAL.delete_asset(P)
        log("eski suv materiali o'chirildi")
    m = AT.create_asset("M_ErtWater", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("tangent_space_normal", False)
    m.set_editor_property("screen_space_reflections", True)
    # To'lqin normali
    wp = MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1200, 300)
    tm = MEL.create_material_expression(m, unreal.MaterialExpressionTime, -1200, 450)
    cw = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -900, 350)
    cw.set_editor_property("code", CODE_WAVE)
    cw.set_editor_property("description", "ErtWaves")
    cw.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ins = []
    for nm in ["WP", "T"]:
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); ins.append(ci)
    cw.set_editor_property("inputs", ins)
    MEL.connect_material_expressions(wp, "", cw, "WP")
    MEL.connect_material_expressions(tm, "", cw, "T")
    MEL.connect_material_property(cw, "", unreal.MaterialProperty.MP_NORMAL)
    # Rang: chuqur/sayoz DepthFade bo'yicha, Fresnel bilan osmon tusi, qirg'oq ko'pigi
    deep = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -900, -300)
    deep.set_editor_property("constant", unreal.LinearColor(0.02, 0.09, 0.16, 1.0))
    shallow = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -900, -150)
    shallow.set_editor_property("constant", unreal.LinearColor(0.10, 0.38, 0.42, 1.0))
    df_col = MEL.create_material_expression(m, unreal.MaterialExpressionDepthFade, -900, 0)
    df_col.set_editor_property("fade_distance_default", 350.0)
    df_col.set_editor_property("opacity_default", 1.0)
    lerp_c = MEL.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -650, -200)
    MEL.connect_material_expressions(shallow, "", lerp_c, "A")
    MEL.connect_material_expressions(deep, "", lerp_c, "B")
    MEL.connect_material_expressions(df_col, "", lerp_c, "Alpha")
    fres = MEL.create_material_expression(m, unreal.MaterialExpressionFresnel, -900, 150)
    fres.set_editor_property("exponent", 5.0)
    fres.set_editor_property("base_reflect_fraction", 0.04)
    sky = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -650, 100)
    sky.set_editor_property("constant", unreal.LinearColor(0.30, 0.44, 0.58, 1.0))
    lerp_f = MEL.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -450, -100)
    MEL.connect_material_expressions(lerp_c, "", lerp_f, "A")
    MEL.connect_material_expressions(sky, "", lerp_f, "B")
    MEL.connect_material_expressions(fres, "", lerp_f, "Alpha")
    # Ko'pik: 1 - DepthFade(90) ; to'lqin bilan modulyatsiya
    df_foam = MEL.create_material_expression(m, unreal.MaterialExpressionDepthFade, -900, 550)
    df_foam.set_editor_property("fade_distance_default", 90.0)
    df_foam.set_editor_property("opacity_default", 1.0)
    om = MEL.create_material_expression(m, unreal.MaterialExpressionOneMinus, -650, 550)
    MEL.connect_material_expressions(df_foam, "", om, "")
    foam_k = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -650, 650)
    foam_k.set_editor_property("r", 0.4)
    foam_m = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -450, 580)
    MEL.connect_material_expressions(om, "", foam_m, "A")
    MEL.connect_material_expressions(foam_k, "", foam_m, "B")
    add_c = MEL.create_material_expression(m, unreal.MaterialExpressionAdd, -250, -50)
    MEL.connect_material_expressions(lerp_f, "", add_c, "A")
    MEL.connect_material_expressions(foam_m, "", add_c, "B")
    MEL.connect_material_property(add_c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    # Shaffoflik: chuqurlik bo'yicha (qirg'oqda 0.35, chuqurda 0.85) + Fresnel
    df_op = MEL.create_material_expression(m, unreal.MaterialExpressionDepthFade, -900, 800)
    df_op.set_editor_property("fade_distance_default", 260.0)
    df_op.set_editor_property("opacity_default", 1.0)
    o_lo = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -650, 780)
    o_lo.set_editor_property("r", 0.35)
    o_hi = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -650, 840)
    o_hi.set_editor_property("r", 0.85)
    lerp_o = MEL.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -450, 800)
    MEL.connect_material_expressions(o_lo, "", lerp_o, "A")
    MEL.connect_material_expressions(o_hi, "", lerp_o, "B")
    MEL.connect_material_expressions(df_op, "", lerp_o, "Alpha")
    one = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -450, 900)
    one.set_editor_property("r", 1.0)
    lerp_o2 = MEL.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -250, 820)
    MEL.connect_material_expressions(lerp_o, "", lerp_o2, "A")
    MEL.connect_material_expressions(one, "", lerp_o2, "B")
    MEL.connect_material_expressions(fres, "", lerp_o2, "Alpha")
    MEL.connect_material_property(lerp_o2, "", unreal.MaterialProperty.MP_OPACITY)
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -250, 200)
    rough.set_editor_property("r", 0.06)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -250, 260)
    spec.set_editor_property("r", 1.0)
    MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    met = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -250, 320)
    met.set_editor_property("r", 0.0)
    MEL.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    ior = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -250, 400)
    ior.set_editor_property("r", 1.2)
    MEL.connect_material_property(ior, "", unreal.MaterialProperty.MP_REFRACTION)
    MEL.recompile_material(m)
    ok = EAL.save_asset(P)
    log("suv materiali yaratildi, saqlandi=%s" % ok)


make()
