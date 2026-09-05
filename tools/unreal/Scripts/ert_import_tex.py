# Ertugrul: Poly Haven yer teksturalarini /Game/ErtAssets/Tex ga import qilish; normal xaritalar uchun sozlash (nor_gl -> yashil kanal teskari)
import unreal, json, os

ROOT = "D:/Unreal_projects/Ertugrul/art/polyhaven_tex"
DEST = "/Game/ErtAssets/Tex"
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()


def log(s):
    unreal.log("[ErtImportTex] " + s)


tex = json.load(open(os.path.join(ROOT, "textures.json")))
tasks = []
for role, files in tex.items():
    for suffix, path in files.items():
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", path)
        t.set_editor_property("destination_path", DEST)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
AT.import_asset_tasks(tasks)
n = 0
for t in tasks:
    for p in list(t.get_editor_property("imported_object_paths") or []):
        a = unreal.load_asset(p)
        if not isinstance(a, unreal.Texture2D): continue
        name = a.get_name()
        if name.endswith("_N"):
            a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            a.set_editor_property("srgb", False)
            a.set_editor_property("flip_green_channel", True)
            a.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP)
        elif name.endswith("_R"):
            a.set_editor_property("srgb", False)
            a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
        else:
            a.set_editor_property("srgb", True)
        EAL.save_loaded_asset(a)
        n += 1
        log("tekstura: " + p)
log("import tugadi: %d tekstura" % n)
