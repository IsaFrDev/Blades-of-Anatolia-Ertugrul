# Ertugrul: Poly Haven glTF modellarini /Game/ErtAssets/PH ga import qilish (Interchange), nomini SM_PH_<id> qilish
import unreal, json, os

ROOT = "D:/Unreal_projects/Ertugrul/art/polyhaven"
DEST = "/Game/ErtAssets/PH"
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()


def log(s):
    unreal.log("[ErtImportPH] " + s)


ids = json.load(open(os.path.join(ROOT, "downloaded.json")))
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
EAL.save_directory(DEST, recursive=True)
log("import tugadi: %d statik mesh" % meshes)
