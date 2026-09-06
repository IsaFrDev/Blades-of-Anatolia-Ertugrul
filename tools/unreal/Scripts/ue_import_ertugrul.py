import unreal, os
SRC = "D:/Yuklanadiganlar"
DEST = "/Game/ErtAssets/Chars"
EAL = unreal.EditorAssetLibrary
tasks = []
for f in ["ertugrul.glb", "ertugrul1.glb"]:
    name = "SM_" + os.path.splitext(f)[0]
    if EAL.does_asset_exist(DEST + "/" + name + "/" + name) or EAL.does_asset_exist(DEST + "/" + name):
        unreal.log("[Chk] mavjud: " + name); continue
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", SRC + "/" + f)
    t.set_editor_property("destination_path", DEST + "/" + name)
    t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    t.set_editor_property("replace_existing", True)
    tasks.append(t)
if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    unreal.log("[Chk] import: " + str(list(t.get_editor_property("imported_object_paths"))))
for p in EAL.list_assets(DEST, recursive=True, include_folder=False):
    a = EAL.load_asset(p)
    if isinstance(a, unreal.StaticMesh):
        b = a.get_bounding_box(); sz = b.max - b.min
        unreal.log("[Chk] SM %s size=%.0f x %.0f x %.0f minZ=%.0f tris=%d mats=%d" % (a.get_name(), sz.x, sz.y, sz.z, b.min.z, a.get_num_triangles(0), len(a.static_materials)))
    elif isinstance(a, (unreal.MaterialInstanceConstant, unreal.Material, unreal.Texture2D)):
        unreal.log("[Chk] asset %s (%s)" % (p, a.get_class().get_name()))
