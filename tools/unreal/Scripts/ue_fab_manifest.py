# Fab/AssetHub statik meshlari manifesti: paketlangan o'yinda asset registri to'liq bo'lmaydi,
# shuning uchun ErtFab.cpp shu ro'yxatdan yuklaydi. Chiqish: Content/Ertugrul/Data/fab_manifest.json
import unreal, json, io, os
EAL = unreal.EditorAssetLibrary
ROOTS = ["/Game/Fab", "/Game/Megascans", "/Game/Quixel", "/Game/MegascansLibrary", "/Game/ErtAssets"]
out = []
for r in ROOTS:
    if not EAL.does_directory_exist(r):
        continue
    for p in EAL.list_assets(r, recursive=True, include_folder=False):
        ad = EAL.find_asset_data(p)
        if ad and str(ad.asset_class_path.asset_name) == "StaticMesh":
            out.append(str(ad.package_name) + "." + str(ad.asset_name))
out = sorted(set(out))
path = os.path.join(unreal.Paths.project_content_dir(), "Ertugrul/Data/fab_manifest.json")
io.open(path, "w", encoding="utf-8", newline="\n").write(json.dumps({"_izoh": "ue_fab_manifest.py avtomatik yaratadi - qo'lda tahrirlamang", "meshes": out}, ensure_ascii=False, indent=1) + "\n")
unreal.log("[Chk] fab manifest: %d mesh -> %s" % (len(out), path))
