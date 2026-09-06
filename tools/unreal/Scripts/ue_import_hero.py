import unreal
EAL = unreal.EditorAssetLibrary
DEST = "/Game/ErtAssets/Chars/Hero"
t = unreal.AssetImportTask()
t.set_editor_property("filename", "D:/Yuklanadiganlar/ertugrul_fbx/SM_ErtugrulHero.fbx")
t.set_editor_property("destination_path", DEST); t.set_editor_property("destination_name", "SM_ErtugrulHero")
t.set_editor_property("automated", True); t.set_editor_property("save", True); t.set_editor_property("replace_existing", True)
opt = unreal.FbxImportUI(); opt.set_editor_property("import_mesh", True); opt.set_editor_property("import_as_skeletal", False); opt.set_editor_property("import_materials", True); opt.set_editor_property("import_textures", True)
opt.static_mesh_import_data.set_editor_property("combine_meshes", True)
t.set_editor_property("options", opt)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
unreal.log("[Chk] import: " + str(list(t.get_editor_property("imported_object_paths"))))
for p in EAL.list_assets(DEST, recursive=True, include_folder=False):
    a = EAL.load_asset(p)
    if isinstance(a, unreal.StaticMesh):
        b = a.get_bounding_box(); sz = b.max - b.min
        unreal.log("[Chk] SM %s size=%.0f x %.0f x %.0f minZ=%.0f tris=%d mats=%d" % (a.get_name(), sz.x, sz.y, sz.z, b.min.z, a.get_num_triangles(0), len(a.static_materials)))
        for i, sm in enumerate(a.static_materials):
            mi = sm.get_editor_property("material_interface"); unreal.log("[Chk]  mat %d %s" % (i, mi.get_path_name() if mi else None))
    elif isinstance(a, unreal.Material):
        a.set_editor_property("used_with_instanced_static_meshes", True); EAL.save_asset(p); unreal.log("[Chk] material %s" % p)
