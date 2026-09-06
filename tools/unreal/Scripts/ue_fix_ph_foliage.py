# Poly Haven glTF barg/o't materiallari (Substrate translucent) -> o'z Masked folyaj master materiali (M_ErtFoliageTex) instanslari
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
MASTER = "/Game/ErtAssets/M_ErtFoliageTex"


def log(s):
    unreal.log("[ErtFoliage] " + s)


def make_master(def_bc, def_nm):
    if EAL.does_asset_exist(MASTER):
        m = EAL.load_asset(MASTER)
        MEL.delete_all_material_expressions(m)
    else:
        m = AT.create_asset("M_ErtFoliageTex", "/Game/ErtAssets", unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    m.set_editor_property("used_with_nanite", True)
    m.set_editor_property("opacity_mask_clip_value", 0.4)
    bc = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
    bc.set_editor_property("parameter_name", "BaseColorTexture")
    bc.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    bc.set_editor_property("texture", def_bc)
    MEL.connect_material_property(bc, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(bc, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    nm = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -600, 300)
    nm.set_editor_property("parameter_name", "NormalTexture")
    nm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    nm.set_editor_property("texture", def_nm)
    MEL.connect_material_property(nm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    rg = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -600, 600)
    rg.set_editor_property("r", 0.75)
    MEL.connect_material_property(rg, "", unreal.MaterialProperty.MP_ROUGHNESS)
    sss = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -600, 850)
    sss.set_editor_property("constant", unreal.LinearColor(0.35, 0.6, 0.15, 1.0))
    MEL.connect_material_property(sss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    MEL.recompile_material(m)
    EAL.save_asset(MASTER)
    return m


# Standart teksturalar: birinchi barg MI dan (dvijok standart teksturalari commandletda topilmasligi mumkin)
def_bc = None; def_nm = None
for p in EAL.list_assets("/Game/ErtAssets/PH", recursive=True, include_folder=False):
    a = EAL.load_asset(p)
    if isinstance(a, unreal.MaterialInstanceConstant) and not a.get_name().endswith("_ert"):
        for tp in a.get_editor_property("texture_parameter_values"):
            nmn = str(tp.get_editor_property("parameter_info").get_editor_property("name")); tv = tp.get_editor_property("parameter_value")
            if nmn == "BaseColorTexture" and tv and "alpha" in tv.get_name(): def_bc = tv
            if nmn == "NormalTexture" and tv and def_bc is not None and def_nm is None: def_nm = tv
        if def_bc and def_nm: break
log("standart: bc=%s nm=%s" % (def_bc.get_name() if def_bc else None, def_nm.get_name() if def_nm else None))
if def_nm:
    def_nm.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP); def_nm.set_editor_property("srgb", False); EAL.save_asset(def_nm.get_path_name())
master = make_master(def_bc, def_nm)


def is_foliage(mi):
    for sp in mi.get_editor_property("scalar_parameter_values"):
        if str(sp.get_editor_property("parameter_info").get_editor_property("name")) == "AlphaMode" and sp.get_editor_property("parameter_value") >= 1.5:
            return True
    bo = mi.get_editor_property("base_property_overrides")
    return bo.get_editor_property("override_blend_mode") and bo.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_OPAQUE


def tex_of(mi, name):
    for tp in mi.get_editor_property("texture_parameter_values"):
        if str(tp.get_editor_property("parameter_info").get_editor_property("name")) == name:
            return tp.get_editor_property("parameter_value")
    return None


cache = {}
n = 0
for p in EAL.list_assets("/Game/ErtAssets/PH", recursive=True, include_folder=False):
    a = EAL.load_asset(p)
    if not isinstance(a, unreal.StaticMesh):
        continue
    mats = a.static_materials
    changed = False
    for i in range(len(mats)):
        mi = mats[i].get_editor_property("material_interface")
        if isinstance(mi, unreal.MaterialInstanceConstant) and mi.get_name().endswith("_ert"):
            # Allaqachon bizniki: ota-materialni yangilash
            if mi.get_path_name() not in cache:
                mi.set_editor_property("parent", master)
                MEL.update_material_instance(mi)
                EAL.save_asset(mi.get_path_name())
                cache[mi.get_path_name()] = mi
            continue
        if not isinstance(mi, unreal.MaterialInstanceConstant) or not is_foliage(mi):
            continue
        key = mi.get_path_name()
        if key not in cache:
            folder = key.rsplit("/", 1)[0]
            name = mi.get_name() + "_ert"
            path = folder + "/" + name
            if EAL.does_asset_exist(path):
                EAL.delete_asset(path)
            nmi = AT.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
            nmi.set_editor_property("parent", master)
            for tn in ("BaseColorTexture", "NormalTexture"):
                t = tex_of(mi, tn)
                if t:
                    if tn == "BaseColorTexture":
                        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
                        t.set_editor_property("compression_no_alpha", False)
                        t.set_editor_property("srgb", True)
                    else:
                        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
                        t.set_editor_property("srgb", False)
                        t.set_editor_property("flip_green_channel", True)
                    EAL.save_asset(t.get_path_name())
                    MEL.set_material_instance_texture_parameter_value(nmi, tn, t)
            MEL.update_material_instance(nmi)
            EAL.save_asset(path)
            cache[key] = nmi
            log("folyaj MI: " + name)
        sm = mats[i]
        sm.set_editor_property("material_interface", cache[key])
        mats[i] = sm
        changed = True
    if changed:
        a.set_editor_property("static_materials", mats)
        EAL.save_asset(p)
        n += 1
log("tayyor: %d mesh, %d folyaj material" % (n, len(cache)))
