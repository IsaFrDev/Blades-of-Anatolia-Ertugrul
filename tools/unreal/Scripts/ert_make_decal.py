# Ertugrul: dekal materiali (mog'or / yoriq / dog') - M_ErtDecal (Deferred Decal, Translucent)
import unreal

AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
MAT_DIR = "/Game/Ertugrul/Materials"


def log(s):
    unreal.log("[ErtMakeDecal] " + s)


def make_decal():
    p = MAT_DIR + "/M_ErtDecal"
    if EAL.does_asset_exist(p):
        EAL.delete_asset(p)
    m = AT.create_asset("M_ErtDecal", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    kind = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, 0)
    kind.set_editor_property("parameter_name", "Kind")
    kind.set_editor_property("default_value", 0.0)
    seed = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, 120)
    seed.set_editor_property("parameter_name", "Seed")
    seed.set_editor_property("default_value", 0.0)
    uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -900, 240)
    cu = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -500, 0)
    cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    inputs = []
    for nm in ("UV", "Kind", "Seed"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", nm)
        inputs.append(ci)
    cu.set_editor_property("inputs", inputs)
    cu.set_editor_property("code", r"""
float2 u = UV * 2.0 - 1.0;   // -1..1
float2 q = UV * 6.0 + Seed * 7.3;
float h1 = frac(sin(dot(floor(q), float2(127.1, 311.7))) * 43758.5453);
float2 f = frac(q); f = f * f * (3.0 - 2.0 * f);
float2 q0 = floor(q);
float a = frac(sin(dot(q0, float2(127.1, 311.7))) * 43758.5453), b = frac(sin(dot(q0 + float2(1, 0), float2(127.1, 311.7))) * 43758.5453);
float c = frac(sin(dot(q0 + float2(0, 1), float2(127.1, 311.7))) * 43758.5453), d = frac(sin(dot(q0 + float2(1, 1), float2(127.1, 311.7))) * 43758.5453);
float n = lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
float2 q2 = q * 2.7; float2 q20 = floor(q2); float2 f2 = frac(q2); f2 = f2 * f2 * (3.0 - 2.0 * f2);
float a2 = frac(sin(dot(q20, float2(269.5, 183.3))) * 43758.5453), b2 = frac(sin(dot(q20 + float2(1, 0), float2(269.5, 183.3))) * 43758.5453);
float c2 = frac(sin(dot(q20 + float2(0, 1), float2(269.5, 183.3))) * 43758.5453), d2 = frac(sin(dot(q20 + float2(1, 1), float2(269.5, 183.3))) * 43758.5453);
float n2 = lerp(lerp(a2, b2, f2.x), lerp(c2, d2, f2.x), f2.y);
float rad = length(u);
float edge = saturate(1.0 - rad);          // chetlari yumshoq
float3 col; float op;
if (Kind < 0.5) {   // mog'or: yashil-jigarrang dog'lar, pastga qarab zichroq
    float m = saturate((n * 0.6 + n2 * 0.4) - 0.35) * 2.2;
    col = lerp(float3(0.10, 0.14, 0.05), float3(0.22, 0.30, 0.10), n2);
    op = m * edge * saturate(1.0 - u.y * 0.8) * 0.85;
} else if (Kind < 1.5) {   // yoriq: ingichka egri chiziq
    float cr = abs(u.x - 0.35 * sin(u.y * 4.0 + Seed) - 0.25 * (n - 0.5));
    float line1 = 1.0 - smoothstep(0.0, 0.05 + 0.03 * n2, cr);
    col = float3(0.05, 0.045, 0.04);
    op = line1 * edge * 0.9;
} else {   // dog' (is/loy): qora-jigarrang
    float m = saturate((n * 0.5 + n2 * 0.5) - 0.3) * 1.8;
    col = float3(0.12, 0.09, 0.06);
    op = m * edge * 0.6;
}
return float4(col, saturate(op));
""")
    MEL.connect_material_expressions(uv, "", cu, "UV")
    MEL.connect_material_expressions(kind, "", cu, "Kind")
    MEL.connect_material_expressions(seed, "", cu, "Seed")
    mask = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -250, 0)
    mask.set_editor_property("r", True); mask.set_editor_property("g", True); mask.set_editor_property("b", True); mask.set_editor_property("a", False)
    MEL.connect_material_expressions(cu, "", mask, "")
    MEL.connect_material_property(mask, "", unreal.MaterialProperty.MP_BASE_COLOR)
    amask = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -250, 150)
    amask.set_editor_property("r", False); amask.set_editor_property("g", False); amask.set_editor_property("b", False); amask.set_editor_property("a", True)
    MEL.connect_material_expressions(cu, "", amask, "")
    MEL.connect_material_property(amask, "", unreal.MaterialProperty.MP_OPACITY)
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -250, 300)
    rough.set_editor_property("r", 0.9)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m)
    EAL.save_asset(p)
    log("yaratildi: " + p)


make_decal()
