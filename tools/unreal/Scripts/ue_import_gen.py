import unreal, os
SRC = "D:/Unreal_projects/Ertugrul/Content/ErtAssets/Gen/src"
DEST = "/Game/ErtAssets/Gen"
EAL = unreal.EditorAssetLibrary
tasks = []
for f in sorted(os.listdir(SRC)):
    if not f.lower().endswith(".glb"): continue
    name = os.path.splitext(f)[0]
    if EAL.does_asset_exist(DEST + "/" + name):
        print("mavjud: " + name); continue
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", SRC + "/" + f)
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    t.set_editor_property("replace_existing", True)
    tasks.append(t)
if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    print("import: " + str(t.get_editor_property("imported_object_paths")))
# Natija: statik meshlar ro'yxati va o'lchamlari
for p in EAL.list_assets(DEST, recursive=True, include_folder=False):
    a = EAL.load_asset(p)
    if isinstance(a, unreal.StaticMesh):
        b = a.get_bounding_box()
        sz = b.max - b.min
        print("SM %s  size(cm)=%.0f x %.0f x %.0f  minZ=%.0f  tris=%d" % (a.get_name(), sz.x, sz.y, sz.z, b.min.z, a.get_num_triangles(0)))
