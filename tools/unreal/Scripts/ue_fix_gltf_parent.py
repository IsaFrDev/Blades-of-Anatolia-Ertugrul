# Interchange glTF MI lari -> ISM/Nanite bayroqli ota-material
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
SRC = "/InterchangeAssets/gltf/Substrate/M_GLTF"
DST_DIR = "/Game/ErtAssets"
DST = DST_DIR + "/M_GLTF_Ert"
src = EAL.load_asset(SRC)
if src is None:
    # Plagin mounti commandletda yo'q: ota-materialni MI orqali olamiz
    for p in EAL.list_assets("/Game/ErtAssets", recursive=True, include_folder=False):
        a = EAL.load_asset(p)
        if isinstance(a, unreal.MaterialInstanceConstant):
            par = a.get_editor_property("parent")
            if par and par.get_path_name().startswith(SRC):
                src = par; break
unreal.log("[Chk] src=%s" % src)
m = EAL.load_asset(DST) if EAL.does_asset_exist(DST) else None
if m is None and src is not None:
    m = AT.duplicate_asset("M_GLTF_Ert", DST_DIR, src)
    unreal.log("[Chk] duplicate -> %s" % m)
targets = []
if m is not None:
    targets.append(m)
if src is not None:
    targets.append(src)   # dvijok materialining o'ziga ham bayroq (saqlanadi)
for t in targets:
    t.set_editor_property("used_with_instanced_static_meshes", True)
    t.set_editor_property("used_with_nanite", True)
    MEL.recompile_material(t)
    ok = EAL.save_asset(t.get_path_name())
    unreal.log("[Chk] bayroq: %s saqlandi=%s" % (t.get_path_name(), ok))
n = 0
if m is not None:
    for p in EAL.list_assets("/Game/ErtAssets", recursive=True, include_folder=False):
        a = EAL.load_asset(p)
        if isinstance(a, unreal.MaterialInstanceConstant):
            par = a.get_editor_property("parent")
            if par and par.get_path_name().startswith(SRC):
                a.set_editor_property("parent", m)
                MEL.update_material_instance(a)
                ok = EAL.save_asset(p)
                unreal.log("[Chk] reparent %s -> %s saqlandi=%s" % (a.get_name(), a.get_editor_property("parent").get_name() if a.get_editor_property("parent") else None, ok))
                n += 1
unreal.log("[Chk] qayta bog'landi: %d MI" % n)
