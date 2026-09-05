# Fotorealistik (AAA) xarita: nima tayyor, nimani muharrirda qilasiz

Buyruq satridan qilinganlar (kod, avtomatik):

| Bosqich | Holat | Qayerda |
|---|---|---|
| Lumen GI + aks, Virtual Shadow Maps | Tayyor (preset PS5/Ultra) | Sozlamalar → Render preseti; `-ErtGfx=ultra` |
| Exponential Height Fog + Volumetric Fog, Sky Atmosphere, kun/tun | Tayyor | `ErtWeather.cpp`, `ErtGameMode.cpp` |
| PCG uslubidagi protsedural joylashtirish (daraxt, buta, qoya, o't, tosh, buyumlar) | Tayyor (C++ skatter, relyef/qiyalik/suv/yo'l qoidalari) | `BuildForest/BuildRocks/BuildGrass/BuildBushes/BuildShoreFoliage/BuildProps` |
| Spline yo'l/devor tizimi | Tayyor (yo'llar `GRoads`, spline devorlar `AddWallSpline`) | `ErtWorldBuilder.cpp` |
| Landscape heightmap + 6 qatlam weightmap (2017x2017, 1 m/piksel) | Eksport qilinadi | `art/landscape/` (`-ErtExportLandscape=<papka>`) |
| Fab/Megascans meshlarini avtomatik ishlatish (daraxt/qoya/buta/buyum) | Tayyor (skaner) | `ErtFab.cpp`, `fab_assets.json` |
| Nanite yoqish (Fab meshlari) | Skript | `tools/unreal/Scripts/ert_enable_nanite.py` |
| Realistik daraxt/buta (barg kartochkali, shamol, subsurface) | Tayyor (protsedural) | `M_ErtLeaf`, `AddTree`, `BuildBushes` |

Muharrirda 4 qadam (assetlar hisob talab qiladi, shuning uchun qo'lda):

## 1. Assetlar (Fab)
Fab panelida bepul: **Electric Dreams Environment** (Epic), **Megascans Trees: European Beech / Black Alder**, **Megascans Rocks / Cliffs**, **Megascans Surfaces**.
"Add to project" → `/Game/Fab/...`. O'yin ishga tushganda skaner ularni topadi: daraxtlar `Trees/Pines`, qoyalar `Rocks`, butalar `Bushes`, buyumlar `Props` (nomi bo'yicha). Hech qanday kod o'zgarmaydi.
Keyin Nanite:
```bash
D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\Unreal_projects\Ertugrul\Ertugrul.uproject -run=pythonscript -script=D:\My_apps\Ertugrul\tools\unreal\Scripts\ert_enable_nanite.py
```

## 2. Landscape (haqiqiy relyef aktori)
1. O'yinni bir marta `-ErtExportLandscape="D:/My_apps/Ertugrul/art/landscape"` bilan ishga tushiring (allaqachon bajarilgan: `art/landscape/heightmap.png`).
2. Muharrir: Landscape Mode (Shift+2) → **Import from File** → `heightmap.png`.
   Section Size 63x63, Sections per Component 2x2, Components 32x32 (2017x2017 avtomatik).
   Location X=-100000 Y=-100000 Z=14000, Scale X=100 Y=100 Z=62.5 (`art/landscape/README.txt`).
3. Layers: `grass, dirt, rock, snow, sand, road` — har biriga `weight_<nom>.png`.
4. Landscape Material: `M_ErtVertexColor` ni o'rniga qatlamli material (Landscape Layer Blend: Megascans Surfaces yoki `/Game/ErtAssets/Tex/T_<rol>_D/N`). Protsedural relyef meshini o'chirish uchun `AErtWorldBuilder` da `bBuildTerrainMesh=false` (World Outliner → ErtWorldBuilder) yoki `-ErtNoTerrainMesh` (keyingi builddan boshlab).
   *Eslatma:* HeightAt() hisob-kitobi o'zgarmaydi, shuning uchun shaharlar, yo'llar, GPS, missiyalar Landscape bilan ham mos keladi.

## 3. PCG Graph (ixtiyoriy)
C++ skatter allaqachon PCG vazifasini bajaradi. Muharrir PCG Graph ishlatmoqchi bo'lsangiz: PCG Volume → Surface Sampler (Landscape) → Density Filter (slope < 30°, `weight_grass`) → Static Mesh Spawner (Fab daraxt/qoya). Bunday holda `TreeCount=0`, `BushCount=0` qiling.

## 4. Yoritish
Directional Light (quyosh) va Sky Light kun vaqtiga bog'liq — `ErtGameMode::SetTimeOfDay`. Post Process: bloom 0.35, exposure bias 0.3 (`ert_make_world.py`). Ultra preset: Lumen Hardware RT (DX12 SM6 allaqachon yoqilgan).
