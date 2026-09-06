import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ULS = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
# Eski preview aktyorlarini tozalash
for a in AS.get_all_level_actors():
    if a.get_actor_label().startswith("ErtPreview_"):
        AS.destroy_actor(a)
# Stat: uchburchaklar (render ma'lumoti)
info = []
base = unreal.Vector(0, 0, 100000)   # osmonda, dunyodan uzoq: toza fon
for i, nm in enumerate(["SM_ertugrul", "SM_ertugrul1"]):
    p = "/Game/ErtAssets/Chars/%s/%s/StaticMeshes/%s" % (nm, nm[3:], nm)
    sm = EAL.load_asset(p)
    tri = sm.get_num_triangles(0)
    ns = sm.get_editor_property("nanite_settings")
    info.append("%s tris=%d nanite=%s lods=%d" % (nm, tri, ns.get_editor_property("enabled"), sm.get_num_lods()))
    # Material: metallic 0 (charm/mo'yna metall emas)
    for s in sm.static_materials:
        mi = s.get_editor_property("material_interface")
        if isinstance(mi, unreal.MaterialInstanceConstant):
            MEL.set_material_instance_scalar_parameter_value(mi, "MetallicFactor", 0.0)
            MEL.update_material_instance(mi); EAL.save_asset(mi.get_path_name())
    a = AS.spawn_actor_from_object(sm, base + unreal.Vector(0, -70 + i * 140, 0), unreal.Rotator(0, 0, 0))
    a.set_actor_label("ErtPreview_" + nm)
    a.set_actor_scale3d(unreal.Vector(1.55, 1.55, 1.55))   # 116 cm -> 180 cm
# Yorug'lik: taxta + kamera
floor = AS.spawn_actor_from_class(unreal.StaticMeshActor, base + unreal.Vector(0, 0, -1), unreal.Rotator(0, 0, 0))
floor.set_actor_label("ErtPreview_floor")
floor.static_mesh_component.set_static_mesh(EAL.load_asset("/Engine/BasicShapes/Plane"))
floor.set_actor_scale3d(unreal.Vector(20, 20, 1))
floor.static_mesh_component.set_material(0, EAL.load_asset("/Engine/EngineMaterials/WorldGridMaterial"))
key = AS.spawn_actor_from_class(unreal.PointLight, base + unreal.Vector(-350, -250, 300), unreal.Rotator(0, 0, 0))
key.set_actor_label("ErtPreview_key"); key.point_light_component.set_intensity(60000); key.point_light_component.set_attenuation_radius(3000)
fill = AS.spawn_actor_from_class(unreal.PointLight, base + unreal.Vector(-300, 350, 200), unreal.Rotator(0, 0, 0))
fill.set_actor_label("ErtPreview_fill"); fill.point_light_component.set_intensity(20000); fill.point_light_component.set_attenuation_radius(3000)
cam_loc = base + unreal.Vector(-420, 0, 120)
ULS.set_level_viewport_camera_info(cam_loc, unreal.Rotator(-5, 0, 0))
unreal.log("[Chk] " + " | ".join(info))
unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, "ertugrul_compare.png")
unreal.log("[Chk] screenshot buyurildi")
