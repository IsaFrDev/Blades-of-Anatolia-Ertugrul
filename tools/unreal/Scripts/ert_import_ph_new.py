# Poly Haven: faqat yangi ID lar (new_ids.json) import, so'ng /Game/ErtAssets ostidagi barcha materiallarga ISM/Nanite bayroqlari, meshlarga Nanite
import unreal, json, os
ROOT = "D:/Unreal_projects/Ertugrul/art/polyhaven"
DEST = "/Game/ErtAssets/PH"
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary


def log(s):
    unreal.log("[ErtImportPH] " + s)


ids = json.load(open(os.path.join(ROOT, "new_ids.json")))
tasks = []
for i in ids:
    src = os.path.join(ROOT, i, i + ".gltf").replace("\\", "/")
    if not os.path.exists(src):
        log("yo'q: " + src); continue
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", src)
    t.set_editor_property("destination_path", DEST + "/" + i)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    tasks.append(t)
if tasks:
    AT.import_asset_tasks(tasks)
meshes = 0
for t in tasks:
    paths = list(t.get_editor_property("imported_object_paths") or [])
    sms = [p for p in paths if unreal.load_asset(p) and isinstance(unreal.load_asset(p), unreal.StaticMesh)]
    i = os.path.basename(os.path.dirname(t.get_editor_property("filename")))
    for k, p in enumerate(sms):
        newname = "SM_PH_%s%s" % (i, "" if k == 0 else "_%d" % k)
        folder = p.rsplit("/", 1)[0]
        target = folder + "/" + newname
        if p.split(".")[0] != target:
            EAL.rename_asset(p, target)
        meshes += 1
    log("%s: %d mesh, %d ob'ekt" % (i, len(sms), len(paths)))
# Bayroqlar: barcha ErtAssets materiallari (ISM + Nanite), statik meshlar Nanite (o't/barg kabi masked ham)
nm = 0; nn = 0
for p in EAL.list_assets("/Game/ErtAssets", recursive=True, include_folder=False):
    a = EAL.load_asset(p)
    if isinstance(a, unreal.Material):
        ch = False
        if not a.get_editor_property("used_with_instanced_static_meshes"):
            a.set_editor_property("used_with_instanced_static_meshes", True); ch = True
        if not a.get_editor_property("used_with_nanite"):
            a.set_editor_property("used_with_nanite", True); ch = True
        if ch:
            MEL.recompile_material(a); EAL.save_asset(p); nm += 1
    elif isinstance(a, unreal.StaticMesh):
        ns = a.get_editor_property("nanite_settings")
        if not ns.get_editor_property("enabled") and a.get_num_triangles(0) > 2000:
            ns.set_editor_property("enabled", True)
            a.set_editor_property("nanite_settings", ns)
            EAL.save_asset(p); nn += 1
EAL.save_directory(DEST, recursive=True)
log("import tugadi: %d statik mesh; bayroq %d material, Nanite %d mesh" % (meshes, nm, nn))
