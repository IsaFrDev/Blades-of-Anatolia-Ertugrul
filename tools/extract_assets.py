# CC0 paketlardan o'yin uchun kerakli modellarni chiqaradi.
# Manba: kenney.nl — Creative Commons CC0 (cheklovsiz, atribusiya shart emas).
import os
import shutil
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOWNLOADS = os.path.join(ROOT, "assets", "downloads")
MODELS = os.path.join(ROOT, "assets", "models")

# Aniq model ro'yxati — nomi shu bilan boshlansa olinadi
PACKS = {
    "nature-kit.zip": ("nature", [
        # Chodirlar — Qayi obasi uchun asosiy
        "tent_detailedOpen", "tent_detailedClosed", "tent_smallOpen", "tent_smallClosed",
        # Daraxtlar
        "tree_default", "tree_detailed", "tree_oak", "tree_pineDefaultA", "tree_pineTallA",
        "tree_pineRoundA", "tree_thin", "tree_tall", "tree_blocks", "tree_cone",
        "tree_plateau", "tree_simple", "tree_fat",
        # O'simlik
        "plant_bush", "plant_bushDetailed", "plant_bushLarge", "plant_bushSmall",
        "grass", "grass_large", "grass_leafs", "flower_redA", "flower_purpleA", "flower_yellowA",
        "mushroom_red", "mushroom_tan",
        # Toshlar va yog'och
        "rock_largeA", "rock_largeB", "rock_largeC", "rock_smallA", "rock_smallB",
        "log", "log_large", "log_stack", "stump_round", "stump_old",
        # Oba jihozlari
        "campfire_logs", "campfire_stones", "campfire_planks",
        "fence_simple", "fence_planks", "fence_gate", "fence_corner",
        "crops_wheatStageD", "crops_cornStageD", "statue_obelisk",
        "bridge_wood", "hanging_moss",
    ]),
    "fantasy-town.zip": ("town", [
        "cart", "cart-high", "stall", "stall-bench", "stall-red", "stall-green", "stall-stool",
        "banner-red", "banner-green", "barrel", "crate", "lantern", "well",
        "fence", "fence-gate", "pillar-wood", "planks", "wall-wood", "roof-gable",
        "blade", "shield",
    ]),
    "characters.zip": ("characters", ["character-"]),
}


def matches(name, patterns):
    base = name.rsplit("/", 1)[-1].rsplit(".", 1)[0]
    return any(base == p or base.startswith(p) for p in patterns)


def extract():
    shutil.rmtree(MODELS, ignore_errors=True)
    os.makedirs(MODELS, exist_ok=True)
    total = 0

    for archive, (target_name, patterns) in PACKS.items():
        path = os.path.join(DOWNLOADS, archive)
        if not os.path.exists(path):
            print("O'tkazib yuborildi:", archive)
            continue

        target = os.path.join(MODELS, target_name)
        os.makedirs(os.path.join(target, "Textures"), exist_ok=True)

        with zipfile.ZipFile(path) as z:
            for name in z.namelist():
                lower = name.lower()
                base = name.rsplit("/", 1)[-1]

                if "obj format" in lower and (lower.endswith(".obj") or lower.endswith(".mtl")):
                    if not matches(name, patterns):
                        continue
                    with z.open(name) as src, open(os.path.join(target, base), "wb") as dst:
                        shutil.copyfileobj(src, dst)

                elif lower.endswith(".png") and "obj format" in lower:
                    with z.open(name) as src, open(os.path.join(target, "Textures", base), "wb") as dst:
                        shutil.copyfileobj(src, dst)

                elif base.lower().startswith("license"):
                    with z.open(name) as src, open(os.path.join(target, "License.txt"), "wb") as dst:
                        shutil.copyfileobj(src, dst)

        objs = sorted(f for f in os.listdir(target) if f.endswith(".obj"))
        total += len(objs)
        print(f"{target_name}: {len(objs)} model")
        for o in objs[:6]:
            print("   ", o)

    # Umumiy litsenziya eslatmasi
    with open(os.path.join(MODELS, "CREDITS.md"), "w", encoding="utf-8") as f:
        f.write("# 3D modellar manbasi\n\n")
        f.write("Barcha modellar **Kenney** (https://kenney.nl) tomonidan yaratilgan va\n")
        f.write("**Creative Commons CC0** litsenziyasi ostida tarqatiladi —\n")
        f.write("ya'ni jamoat mulki: tijoriy loyihalarda ham cheklovsiz ishlatish mumkin,\n")
        f.write("atribusiya majburiy emas (lekin biz baribir ko'rsatamiz).\n\n")
        f.write("| Paket | Manba |\n|---|---|\n")
        f.write("| Nature Kit | https://kenney.nl/assets/nature-kit |\n")
        f.write("| Fantasy Town Kit | https://kenney.nl/assets/fantasy-town-kit |\n")
        f.write("| Blocky Characters | https://kenney.nl/assets/blocky-characters |\n")

    print()
    print("Jami:", total, "model ->", MODELS)


if __name__ == "__main__":
    extract()
