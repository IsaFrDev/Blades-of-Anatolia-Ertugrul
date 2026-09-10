import unreal, os
EAL = unreal.EditorAssetLibrary
DEST = "/Game/ErtAssets/Horse"
src = "D:/Yuklanadiganlar/horse_rig/SK_Horse.fbx"
t = unreal.AssetImportTask()
t.set_editor_property("filename", src); t.set_editor_property("destination_path", DEST); t.set_editor_property("destination_name", "SK_Horse")
t.set_editor_property("automated", True); t.set_editor_property("save", True); t.set_editor_property("replace_existing", True)
ui = unreal.FbxImportUI()
ui.set_editor_property("import_mesh", True); ui.set_editor_property("import_as_skeletal", True); ui.set_editor_property("import_animations", True)
ui.set_editor_property("import_materials", True); ui.set_editor_property("import_textures", True)
ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
ui.skeletal_mesh_import_data.set_editor_property("import_morph_targets", False)
ui.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
t.set_editor_property("options", ui)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
unreal.log("[Chk] import: " + str(list(t.get_editor_property("imported_object_paths"))))
for p in EAL.list_assets(DEST, recursive=True, include_folder=False):
    a = EAL.load_asset(p)
    unreal.log("[Chk] %s (%s)" % (p, a.get_class().get_name()))
    if isinstance(a, unreal.SkeletalMesh):
        b = a.get_bounds() if hasattr(a, "get_bounds") else None
        unreal.log("[Chk] SK bounds %s" % (b.box_extent if b else "?"))
