# Ertugrul: M_ErtLeaf - buta barg kartochkalari: protsedural barg to'plami niqobi (Masked), ikki tomonlama folyaj, shamol
import unreal

AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
MAT_DIR = "/Game/Ertugrul/Materials"
P = MAT_DIR + "/M_ErtLeaf"


def log(s):
    unreal.log("[ErtMakeLeaf] " + s)


# Kartochka UV (0..1): markazdan radial silueti barg tishli, ichida teshiklar; natija: rgb = rang, a = niqob
CODE = r"""
float2 u = UV - 0.5;
float r = length(u) * 2.0;
float ang = atan2(u.y, u.x);
float seed = frac(sin(dot(floor(WP.xy / 37.0), float2(127.1, 311.7))) * 43758.5453) * 6.283;
// Barg to'plami silueti: 5-7 bo'lakli, tishli chet
float lobes = 0.72 + 0.16 * sin(ang * 5.0 + seed) + 0.10 * sin(ang * 11.0 - seed * 2.0) + 0.06 * sin(ang * 23.0 + seed);
// Teshiklar (kichik shovqin)
float2 q = UV * 7.0 + seed;
float2 q0 = floor(q), f = frac(q); f = f * f * (3.0 - 2.0 * f);
#define H(v) frac(sin(dot(v, float2(127.1, 311.7))) * 43758.5453)
float n = lerp(lerp(H(q0), H(q0 + float2(1, 0)), f.x), lerp(H(q0 + float2(0, 1)), H(q0 + float2(1, 1)), f.x), f.y);
float mask = (r < lobes && n > 0.16) ? 1.0 : 0.0;
// Rang: markaz qoramtir, chetlar ochroq; tomirlar
float3 c = VC * (0.65 + 0.55 * r) * (0.8 + 0.4 * n);
float vein = 1.0 - 0.15 * saturate(1.0 - abs(sin(ang * 6.0 + seed) * 8.0));
c *= vein;
return float4(c, mask);
"""

CODE_WIND = r"""
float2 p = WP.xy / 100.0;
float w = sin(T * 1.4 + p.x * 0.3 + p.y * 0.2) * 0.6 + sin(T * 2.6 + p.y * 0.6) * 0.3 + sin(T * 4.7 + p.x * 1.5) * 0.1;
float amp = (2.0 + 4.0 * A);
return float3(w * amp, w * amp * 0.5, -abs(w) * amp * 0.2);
"""


def make():
    if EAL.does_asset_exist(P): EAL.delete_asset(P)
    m = AT.create_asset("M_ErtLeaf", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    wp = MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1200, 0)
    tm = MEL.create_material_expression(m, unreal.MaterialExpressionTime, -1200, 150)
    vc = MEL.create_material_expression(m, unreal.MaterialExpressionVertexColor, -1200, 300)
    uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1200, 450)
    def custom(code, name, y, out_type, names):
        cu = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -800, y)
        cu.set_editor_property("code", code); cu.set_editor_property("description", name); cu.set_editor_property("output_type", out_type)
        ins = []
        for nm in names:
            ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); ins.append(ci)
        cu.set_editor_property("inputs", ins)
        return cu
    leaf = custom(CODE, "ErtLeaf", 300, unreal.CustomMaterialOutputType.CMOT_FLOAT4, ["UV", "VC", "WP"])
    MEL.connect_material_expressions(uv, "", leaf, "UV"); MEL.connect_material_expressions(vc, "", leaf, "VC"); MEL.connect_material_expressions(wp, "", leaf, "WP")
    rgb = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -400, 300)
    rgb.set_editor_property("r", True); rgb.set_editor_property("g", True); rgb.set_editor_property("b", True); rgb.set_editor_property("a", False)
    MEL.connect_material_expressions(leaf, "", rgb, "")
    MEL.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
    am = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -400, 420)
    am.set_editor_property("r", False); am.set_editor_property("g", False); am.set_editor_property("b", False); am.set_editor_property("a", True)
    MEL.connect_material_expressions(leaf, "", am, "")
    MEL.connect_material_property(am, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    wind = custom(CODE_WIND, "ErtLeafWind", 0, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["WP", "T", "A"])
    MEL.connect_material_expressions(wp, "", wind, "WP"); MEL.connect_material_expressions(tm, "", wind, "T"); MEL.connect_material_expressions(vc, "A", wind, "A")
    MEL.connect_material_property(wind, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    sss = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -800, 600)
    sss.set_editor_property("constant", unreal.LinearColor(0.3, 0.55, 0.12, 1.0))
    MEL.connect_material_property(sss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -800, 700)
    rough.set_editor_property("r", 0.7)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m)
    EAL.save_asset(P)
    log("barg materiali yaratildi")


make()
