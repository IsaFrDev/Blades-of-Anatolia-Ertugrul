# Ertugrul: osmon materiallari - M_ErtClouds (bulut qatlami), M_ErtStars (yulduzlar gumbazi)
import unreal

AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
MAT_DIR = "/Game/Ertugrul/Materials"


def log(s):
    unreal.log("[ErtMakeSky] " + s)


CODE_CLOUD = r"""
float2 p = WP.xy / 150000.0 + float2(T * 0.004, T * 0.0015);
#define H2(q) frac(sin(dot(floor(q), float2(127.1, 311.7))) * 43758.5453)
#define SM(x) ((x) * (x) * (3.0 - 2.0 * (x)))
#define VN(q) lerp(lerp(H2(q), H2((q) + float2(1, 0)), SM(frac((q).x))), lerp(H2((q) + float2(0, 1)), H2((q) + float2(1, 1)), SM(frac((q).x))), SM(frac((q).y)))
float n = VN(p * 3.0) * 0.55 + VN(p * 7.0 + 3.1) * 0.28 + VN(p * 15.0 + 7.7) * 0.12 + VN(p * 31.0 - T * 0.01) * 0.05;
float d = smoothstep(Cov, Cov + 0.28, n);
float shade = 0.55 + 0.6 * smoothstep(Cov, Cov + 0.5, n + 0.08);
float2 dc = WP.xy - Cam.xy;
float fade = 1.0 - smoothstep(900000.0, 1600000.0, length(dc));
return float3(d * fade, shade, 0.0);
"""

CODE_STARS = r"""
float3 dir = normalize(WP - Cam);
float2 sc = float2(atan2(dir.y, dir.x) / 6.2831853 + 0.5, dir.z * 0.5 + 0.5) * float2(420.0, 210.0);
#define H2(q) frac(sin(dot(floor(q), float2(127.1, 311.7))) * 43758.5453)
float2 f = frac(sc) - 0.5;
float h = H2(sc);
float h2 = frac(h * 91.7);
float star = (h > 0.955) ? smoothstep(0.32, 0.0, length(f)) * (0.4 + 0.6 * h2) : 0.0;
float tw = 0.75 + 0.25 * sin(T * (2.0 + 4.0 * h2) + h * 50.0);
float horizon = smoothstep(0.02, 0.2, dir.z);
float3 col = lerp(float3(1.0, 0.95, 0.85), float3(0.8, 0.88, 1.0), h2);
return col * star * tw * horizon * Vis;
"""


def custom(m, code, name, x, y, out_type, names):
    cu = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    cu.set_editor_property("code", code)
    cu.set_editor_property("description", name)
    cu.set_editor_property("output_type", out_type)
    ins = []
    for nm in names:
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); ins.append(ci)
    cu.set_editor_property("inputs", ins)
    return cu


def make_clouds():
    p = MAT_DIR + "/M_ErtClouds"
    if EAL.does_asset_exist(p): EAL.delete_asset(p)
    m = AT.create_asset("M_ErtClouds", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    wp = MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1200, 0)
    tm = MEL.create_material_expression(m, unreal.MaterialExpressionTime, -1200, 150)
    cam = MEL.create_material_expression(m, unreal.MaterialExpressionCameraPositionWS, -1200, 300)
    cov = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -1200, 450)
    cov.set_editor_property("parameter_name", "Coverage"); cov.set_editor_property("default_value", 0.55)
    cu = custom(m, CODE_CLOUD, "ErtCloud", -900, 100, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["WP", "T", "Cam", "Cov"])
    MEL.connect_material_expressions(wp, "", cu, "WP"); MEL.connect_material_expressions(tm, "", cu, "T")
    MEL.connect_material_expressions(cam, "", cu, "Cam"); MEL.connect_material_expressions(cov, "", cu, "Cov")
    dmask = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -650, 50)
    dmask.set_editor_property("r", True); dmask.set_editor_property("g", False); dmask.set_editor_property("b", False); dmask.set_editor_property("a", False)
    MEL.connect_material_expressions(cu, "", dmask, "")
    smask = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -650, 200)
    smask.set_editor_property("r", False); smask.set_editor_property("g", True); smask.set_editor_property("b", False); smask.set_editor_property("a", False)
    MEL.connect_material_expressions(cu, "", smask, "")
    tint = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -650, 350)
    tint.set_editor_property("parameter_name", "Tint"); tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, 250)
    MEL.connect_material_expressions(tint, "", mul, "A"); MEL.connect_material_expressions(smask, "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    dens = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -650, 500)
    dens.set_editor_property("parameter_name", "Density"); dens.set_editor_property("default_value", 0.9)
    mul2 = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, 450)
    MEL.connect_material_expressions(dmask, "", mul2, "A"); MEL.connect_material_expressions(dens, "", mul2, "B")
    MEL.connect_material_property(mul2, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(m)
    EAL.save_asset(p)
    log("bulut materiali yaratildi")


def make_stars():
    p = MAT_DIR + "/M_ErtStars"
    if EAL.does_asset_exist(p): EAL.delete_asset(p)
    m = AT.create_asset("M_ErtStars", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    wp = MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1200, 0)
    tm = MEL.create_material_expression(m, unreal.MaterialExpressionTime, -1200, 150)
    cam = MEL.create_material_expression(m, unreal.MaterialExpressionCameraPositionWS, -1200, 300)
    vis = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -1200, 450)
    vis.set_editor_property("parameter_name", "Vis"); vis.set_editor_property("default_value", 0.0)
    cu = custom(m, CODE_STARS, "ErtStars", -900, 100, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["WP", "T", "Cam", "Vis"])
    MEL.connect_material_expressions(wp, "", cu, "WP"); MEL.connect_material_expressions(tm, "", cu, "T")
    MEL.connect_material_expressions(cam, "", cu, "Cam"); MEL.connect_material_expressions(vis, "", cu, "Vis")
    MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    EAL.save_asset(p)
    log("yulduz materiali yaratildi")


make_clouds()
make_stars()
