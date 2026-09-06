# AssetHub/Tripo GLB importlari uchun to'g'ri PBR material: Color (sRGB) -> BaseColor, NormalGL -> Normal, ORM (R=AO, G=Roughness, B=Metallic*0)
import unreal
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
ROOT = "/Game/ErtAssets/Gen"


def log(s):
    unreal.log("[ErtGenMat] " + s)


def find_tex(folder, key):
    for p in EAL.list_assets(folder, recursive=True, include_folder=False):
        n = p.split("/")[-1].split(".")[0]
        if n.lower().startswith(key.lower()):
            t = EAL.load_asset(p)
            if isinstance(t, unreal.Texture2D):
                return t
    return None


done = 0
for p in EAL.list_assets(ROOT, recursive=True, include_folder=False):
    a = EAL.load_asset(p)
    if not isinstance(a, unreal.StaticMesh):
        continue
    name = a.get_name()
    folder = ROOT + "/" + name
    color = find_tex(folder, "Color")
    normal = find_tex(folder, "NormalGL")
    orm = find_tex(folder, "ORM")
    if not color:
        log("rang teksturasi yo'q: " + name)
        continue
    color.set_editor_property("srgb", True)
    color.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
    if normal:
        normal.set_editor_property("srgb", False)
        normal.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        normal.set_editor_property("flip_green_channel", True)   # GL normal -> DX (UE)
    if orm:
        orm.set_editor_property("srgb", False)
        orm.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    mp = folder + "/M_" + name
    if EAL.does_asset_exist(mp):
        EAL.delete_asset(mp)
    m = AT.create_asset("M_" + name, folder, unreal.Material, unreal.MaterialFactoryNew())
    tc = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSample, -600, 0)
    tc.set_editor_property("texture", color)
    tc.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    MEL.connect_material_property(tc, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    if normal:
        tn = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSample, -600, 300)
        tn.set_editor_property("texture", normal)
        tn.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        MEL.connect_material_property(tn, "RGB", unreal.MaterialProperty.MP_NORMAL)
    if orm:
        to = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSample, -600, 600)
        to.set_editor_property("texture", orm)
        to.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        MEL.connect_material_property(to, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
        MEL.connect_material_property(to, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    else:
        r = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -600, 600)
        r.set_editor_property("r", 0.8)
        MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    met = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -600, 800)
    met.set_editor_property("r", 0.0)
    MEL.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    if "Bush" in name or "Juniper" in name or "Tree" in name:
        m.set_editor_property("two_sided", True)
    MEL.recompile_material(m)
    EAL.save_asset(mp)
    mats = a.static_materials
    for i in range(len(mats)):
        sm = mats[i]
        sm.set_editor_property("material_interface", m)
        mats[i] = sm
    a.set_editor_property("static_materials", mats)
    EAL.save_asset(p)
    for t in (color, normal, orm):
        if t:
            EAL.save_asset(t.get_path_name())
    done += 1
    log("material: " + name)
log("tayyor: %d mesh" % done)


# Blender fabrikasi elementlari: har slot uchun o'z teksturasi (M_wood/M_cloth/M_hay/M_rock -> T_<key>_D/N)
BLENDER_PROPS = ["SM_TetherPost_Wood", "SM_Trough_Wood", "SM_HayBale_01", "SM_Woodpile_01", "SM_Banner_Kayi", "SM_WeaponRack_Wood", "SM_FirePit_Stone"]
for name in BLENDER_PROPS:
    folder = ROOT + "/" + name
    a = None
    for p in EAL.list_assets(folder, recursive=True, include_folder=False):
        x = EAL.load_asset(p)
        if isinstance(x, unreal.StaticMesh): a = x
    if a is None:
        log("mesh yo'q: " + name); continue
    mats = a.static_materials
    for i in range(len(mats)):
        sm = mats[i]; mi = sm.get_editor_property("material_interface")
        key = (mi.get_name() if mi else "wood").replace("M_", "").split(".")[0].split("_")[0].lower()
        if key not in ("wood", "cloth", "hay", "rock"): key = "wood"
        color = find_tex(folder, "T_%s_D" % key); normal = find_tex(folder, "T_%s_N" % key)
        if not color:
            log("tekstura yo'q %s %s" % (name, key)); continue
        color.set_editor_property("srgb", True); color.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        if normal:
            normal.set_editor_property("srgb", False); normal.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP); normal.set_editor_property("flip_green_channel", True)
        mp = folder + "/M_%s_%s" % (name, key)
        if EAL.does_asset_exist(mp): EAL.delete_asset(mp)
        m = AT.create_asset("M_%s_%s" % (name, key), folder, unreal.Material, unreal.MaterialFactoryNew())
        tc = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSample, -600, 0); tc.set_editor_property("texture", color); tc.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        MEL.connect_material_property(tc, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
        if normal:
            tn = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSample, -600, 300); tn.set_editor_property("texture", normal); tn.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
            MEL.connect_material_property(tn, "RGB", unreal.MaterialProperty.MP_NORMAL)
        r = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -600, 600); r.set_editor_property("r", 0.85 if key in ("hay", "rock") else 0.75); MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
        met = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -600, 800); met.set_editor_property("r", 0.0); MEL.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
        if key == "cloth": m.set_editor_property("two_sided", True)
        m.set_editor_property("used_with_instanced_static_meshes", True); m.set_editor_property("used_with_nanite", True)
        MEL.recompile_material(m); EAL.save_asset(mp)
        sm.set_editor_property("material_interface", m); mats[i] = sm
    a.set_editor_property("static_materials", mats); EAL.save_asset(a.get_path_name())
    log("blender prop material: " + name)
