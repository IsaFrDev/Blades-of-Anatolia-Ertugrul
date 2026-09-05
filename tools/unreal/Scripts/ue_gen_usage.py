import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
n = 0
for p in EAL.list_assets("/Game/ErtAssets/Gen", recursive=True, include_folder=False):
    a = EAL.load_asset(p)
    if isinstance(a, unreal.Material):
        a.set_editor_property("used_with_instanced_static_meshes", True)
        a.set_editor_property("used_with_nanite", True)
        a.set_editor_property("used_with_static_lighting", False)
        MEL.recompile_material(a)
        EAL.save_asset(p)
        n += 1
    elif isinstance(a, unreal.StaticMesh):
        ns = a.get_editor_property("nanite_settings")
        ns.set_editor_property("enabled", True)
        a.set_editor_property("nanite_settings", ns)
        EAL.save_asset(p)
unreal.log("[Chk] usage bayroqlari: %d material" % n)
