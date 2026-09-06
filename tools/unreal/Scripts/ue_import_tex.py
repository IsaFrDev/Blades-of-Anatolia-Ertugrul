# Blender dan pishirilgan uzluksiz yer teksturalarini /Game/ErtAssets/Tex ga import (mavjudlarini almashtiradi; landshaft materiali o'zgarishsiz ishlaydi)
import unreal, os
EAL = unreal.EditorAssetLibrary
SRC = "D:/Unreal_projects/Ertugrul/art/blender_tex/seamless"
DEST = "/Game/ErtAssets/Tex"
tasks = []
for f in sorted(os.listdir(SRC)):
    if not f.lower().endswith(".png"): continue
    name = os.path.splitext(f)[0]
    if name.split("_")[1] in ("wood", "hay", "cloth"): continue   # faqat yer rollari
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", SRC + "/" + f); t.set_editor_property("destination_path", DEST); t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True); t.set_editor_property("save", True); t.set_editor_property("replace_existing", True)
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    for p in t.get_editor_property("imported_object_paths"):
        tx = EAL.load_asset(p)
        if not isinstance(tx, unreal.Texture2D): continue
        if p.split(".")[-1].endswith("_N"):
            tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP); tx.set_editor_property("srgb", False); tx.set_editor_property("flip_green_channel", False)
        else:
            tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT); tx.set_editor_property("srgb", True)
        tx.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP); tx.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)
        EAL.save_asset(p); unreal.log("[Chk] tex %s" % p)
unreal.log("[Chk] tex import done %d" % len(tasks))
