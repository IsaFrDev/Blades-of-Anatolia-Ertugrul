import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
m = EAL.load_asset("/Game/ErtAssets/M_ErtFoliageTex")
m.set_editor_property("opacity_mask_clip_value", 0.02)
MEL.recompile_material(m); EAL.save_asset(m.get_path_name())
n = 0
for p in EAL.list_assets("/Game/ErtAssets/PH", recursive=True, include_folder=False):
    a = EAL.load_asset(p)
    if isinstance(a, unreal.StaticMesh) and ("tree" in a.get_name() or "fir" in a.get_name() or "grass" in a.get_name()):
        ns = a.get_editor_property("nanite_settings")
        if ns.get_editor_property("enabled"):
            ns.set_editor_property("enabled", False); a.set_editor_property("nanite_settings", ns); EAL.save_asset(p); n += 1
        for i, sm in enumerate(a.static_materials):
            mi = sm.get_editor_property("material_interface")
            unreal.log("[Chk] %s slot %d %s sections=%d" % (a.get_name(), i, mi.get_name() if mi else None, a.get_num_sections(0)))
unreal.log("[Chk] Nanite o'chirildi: %d" % n)
