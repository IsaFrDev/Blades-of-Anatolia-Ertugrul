# Ertugrul: M_ErtGrass - o't tolalari (ikki tomonlama, shamolda tebranadi: alfa=1 uchi, 0 tagi)
import unreal

AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
MAT_DIR = "/Game/Ertugrul/Materials"
P = MAT_DIR + "/M_ErtGrass"


def log(s):
    unreal.log("[ErtMakeGrass] " + s)


CODE_WIND = r"""
float2 p = WP.xy / 100.0;
float w = sin(T * 1.7 + p.x * 0.35 + p.y * 0.2) * 0.6 + sin(T * 2.9 + p.y * 0.7 - p.x * 0.4) * 0.3 + sin(T * 5.3 + p.x * 1.9) * 0.1;
float g = 0.5 + 0.5 * sin(T * 0.37 + p.x * 0.05);   // shamol kuchi sekin o'zgaradi
float amp = (6.0 + 10.0 * g) * A;
return float3(w * amp, w * amp * 0.4, -abs(w) * amp * 0.25);
"""

CODE_COL = r"""
float2 p = WP.xy / 100.0;
#define H2(q) frac(sin(dot(floor(q), float2(127.1, 311.7))) * 43758.5453)
float v = H2(p * 1.7);
float3 c = VC * (0.75 + 0.5 * v);
c = lerp(c * float3(0.55, 0.6, 0.35), c, A);   // tagi qoramtir, uchi ochroq
return c;
"""


def make():
    if EAL.does_asset_exist(P): EAL.delete_asset(P)
    m = AT.create_asset("M_ErtGrass", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("two_sided", True)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    wp = MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1200, 0)
    tm = MEL.create_material_expression(m, unreal.MaterialExpressionTime, -1200, 150)
    vc = MEL.create_material_expression(m, unreal.MaterialExpressionVertexColor, -1200, 300)
    def custom(code, name, y, out_type, names):
        cu = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -800, y)
        cu.set_editor_property("code", code); cu.set_editor_property("description", name); cu.set_editor_property("output_type", out_type)
        ins = []
        for nm in names:
            ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); ins.append(ci)
        cu.set_editor_property("inputs", ins)
        return cu
    wind = custom(CODE_WIND, "ErtWind", 0, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["WP", "T", "A"])
    MEL.connect_material_expressions(wp, "", wind, "WP"); MEL.connect_material_expressions(tm, "", wind, "T"); MEL.connect_material_expressions(vc, "A", wind, "A")
    MEL.connect_material_property(wind, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    col = custom(CODE_COL, "ErtGrassColor", 300, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["WP", "VC", "A"])
    MEL.connect_material_expressions(wp, "", col, "WP"); MEL.connect_material_expressions(vc, "", col, "VC"); MEL.connect_material_expressions(vc, "A", col, "A")
    MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    sss = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -800, 550)
    sss.set_editor_property("constant", unreal.LinearColor(0.35, 0.6, 0.15, 1.0))
    MEL.connect_material_property(sss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -800, 650)
    rough.set_editor_property("r", 0.8)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m)
    EAL.save_asset(P)
    log("o't materiali yaratildi")


make()
