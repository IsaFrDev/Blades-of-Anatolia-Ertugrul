# Ertugrul: Fab/Megascans statik meshlariga Nanite yoqish (UnrealEditor-Cmd -run=pythonscript -script=...)
import unreal

EAL = unreal.EditorAssetLibrary
PATHS = ["/Game/Fab", "/Game/Megascans", "/Game/Quixel", "/Game/MegascansLibrary", "/Game/ErtAssets"]


def log(s):
    unreal.log("[ErtNanite] " + s)


n = 0
for root in PATHS:
    if not EAL.does_directory_exist(root):
        continue
    for path in EAL.list_assets(root, recursive=True, include_folder=False):
        a = EAL.load_asset(path)
        if not isinstance(a, unreal.StaticMesh):
            continue
        ns = a.get_editor_property("nanite_settings")
        if ns.get_editor_property("enabled"):
            continue
        ns.set_editor_property("enabled", True)
        a.set_editor_property("nanite_settings", ns)
        EAL.save_asset(path)
        n += 1
log("Nanite yoqildi: %d mesh" % n)
