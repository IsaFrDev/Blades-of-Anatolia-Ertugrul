# Ertugrul: materiallar + World darajasini yaratish (UnrealEditor-Cmd -run=pythonscript)
import unreal

AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
MAT_DIR = "/Game/Ertugrul/Materials"
MAP = "/Game/Ertugrul/Maps/World"


def log(s):
    unreal.log("[ErtMakeWorld] " + s)


def make_vertex_color_material():
    p = MAT_DIR + "/M_ErtVertexColor"
    if EAL.does_asset_exist(p):
        log("mavjud: " + p)
        return EAL.load_asset(p)
    m = AT.create_asset("M_ErtVertexColor", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    vc = MEL.create_material_expression(m, unreal.MaterialExpressionVertexColor, -400, 0)
    MEL.connect_material_property(vc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 250)
    rough.set_editor_property("r", 0.82)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 350)
    spec.set_editor_property("r", 0.3)
    MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    MEL.recompile_material(m)
    EAL.save_asset(p)
    log("yaratildi: " + p)
    return m


def make_water_material():
    p = MAT_DIR + "/M_ErtWater"
    if EAL.does_asset_exist(p):
        log("mavjud: " + p)
        return EAL.load_asset(p)
    m = AT.create_asset("M_ErtWater", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
    m.set_editor_property("two_sided", True)
    col = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 0)
    col.set_editor_property("constant", unreal.LinearColor(0.07, 0.22, 0.32, 1.0))
    MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    op = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 200)
    op.set_editor_property("r", 0.62)
    MEL.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 300)
    rough.set_editor_property("r", 0.08)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    met = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 400)
    met.set_editor_property("r", 0.25)
    MEL.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    MEL.recompile_material(m)
    EAL.save_asset(p)
    log("yaratildi: " + p)
    return m


def make_level():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if EAL.does_asset_exist(MAP):
        # Mavjud darajani yuklab, quyoshni tuzatamiz
        log("daraja mavjud, yangilanadi: " + MAP)
        les.load_level(MAP)
        for a in eas.get_all_level_actors():
            if isinstance(a, unreal.DirectionalLight):
                a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-34.0, yaw=28.0), False)
                log("quyosh burildi: %s" % a.get_actor_rotation())
            elif isinstance(a, unreal.ExponentialHeightFog):
                fc = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
                fc.set_editor_property("fog_density", 0.0006)
                fc.set_editor_property("fog_height_falloff", 0.3)
                fc.set_editor_property("start_distance", 8000.0)
                log("tuman yengillashtirildi")
            elif isinstance(a, unreal.VolumetricCloud):
                # Intel GPU: hajmli bulut shaderlari juda og'ir - olib tashlanadi
                eas.destroy_actor(a)
                log("bulut olib tashlandi")
        saved = les.save_current_level()
        log("save_current_level -> %s" % saved)
        return
    ok = les.new_level(MAP)
    log("new_level -> %s" % ok)

    builder = eas.spawn_actor_from_class(unreal.ErtWorldBuilder, unreal.Vector(0, 0, 0))
    builder.set_actor_label("ErtWorldBuilder")

    sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 60000))
    sun.set_actor_label("Sun")
    # unreal.Rotator(roll, pitch, yaw)! Quyosh 34 gradus balandlikda, 28 gradus azimut
    sun.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-34.0, yaw=28.0), False)
    sc = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sc.set_intensity(7.0)
    sc.set_light_color(unreal.LinearColor(1.0, 0.93, 0.82, 1.0))
    sc.set_editor_property("atmosphere_sun_light", True)
    sc.set_editor_property("dynamic_shadow_distance_movable_light", 12000.0)
    sc.set_editor_property("cascade_distribution_exponent", 2.5)
    sc.set_mobility(unreal.ComponentMobility.MOVABLE)

    atm = eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    atm.set_actor_label("SkyAtmosphere")
    sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 40000))
    sky.set_actor_label("SkyLight")
    skc = sky.get_component_by_class(unreal.SkyLightComponent)
    skc.set_mobility(unreal.ComponentMobility.MOVABLE)
    skc.set_editor_property("real_time_capture", True)
    fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fog.set_actor_label("Fog")
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fc.set_editor_property("fog_density", 0.004)
    fc.set_editor_property("fog_height_falloff", 0.15)
    fc.set_editor_property("start_distance", 4000.0)

    # O'yinchi boshlanishi: oba ichida, janubiy darvoza yaqinida (reja: E=-560, N=455, Z=21.2)
    start_loc = unreal.ErtWorldBuilder.plan_to_world(-560.0, 455.0, 21.5)
    ps = eas.spawn_actor_from_class(unreal.PlayerStart, start_loc, unreal.Rotator(0, 0, 0))
    ps.set_actor_label("PlayerStart_Oba")

    saved = les.save_current_level()
    log("save_current_level -> %s" % saved)


make_vertex_color_material()
make_water_material()
make_level()
log("tayyor")
