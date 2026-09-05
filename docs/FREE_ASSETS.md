# Bepul assetlar (personaj, yer, devor, ot)

Barcha havolalar bepul/ochiq litsenziyali. Fab'da "Add to My Library" → UE muharririda **Fab** panelidan
"Add to project" bosiladi; asset `/Game/Fab/...` ga tushadi va `FErtFabLib` skaneri uni o'zi topadi.
Boshqa saytlardan yuklangan FBX/GLB ni `Content/ErtAssets/` ga sudrab kiritish kifoya.

## Personaj (skeletli, animatsiyali)
| Nom | Havola | Litsenziya | Ishlatish |
|---|---|---|---|
| Paragon: Kwang (qilichboz jangchi) | https://www.fab.com/search?q=paragon%20kwang | Bepul (Epic) | `character.json` → hero.mesh = SK_Kwang, anims Paragon animatsiyalari |
| Paragon: Greystone / Crunch / Serath | https://www.fab.com/search?q=paragon&is_free=1 | Bepul (Epic) | enemy/npc profillari |
| Medieval Knight (Sketchfab, CC-BY) | https://sketchfab.com/search?q=medieval+knight+rigged&features=downloadable&licenses=322a749bcfa841b29dff1e8a1bb74b0b | CC-BY | FBX import → hero.mesh |
| Mixamo (personaj + animatsiyalar) | https://www.mixamo.com/ | Bepul (Adobe) | "Sword and Shield" to'plami: idle/walk/run/attack/hurt/death (FBX for Unreal) |
| Quaternius Ultimate Modular Characters | https://quaternius.com/packs/ultimatemodularcharacters.html | CC0 | oddiy tayyor personajlar, riglangan |

## Yer teksturalari (allaqachon `/Game/ErtAssets/Tex` da: grass/dirt/sand/rock/snow/cobble Poly Haven)
| Nom | Havola | Litsenziya |
|---|---|---|
| Poly Haven teksturalar | https://polyhaven.com/textures?c=terrain | CC0 |
| Megascans Surfaces (Fab, bepul) | https://www.fab.com/search?q=megascans%20surface&is_free=1 | Bepul (Epic, UE ichida) |
| ambientCG (grass/ground/rock) | https://ambientcg.com/list?category=Ground | CC0 |
| Quixel Megascans Trees (Fab) | https://www.fab.com/search?q=megascans%20trees&is_free=1 | Bepul (Epic) — `Trees/Pines` kategoriyasi skanerda |

## Devor / bino teksturalari va modellar
| Nom | Havola | Litsenziya |
|---|---|---|
| Poly Haven stone/brick wall | https://polyhaven.com/textures?c=wall | CC0 |
| ambientCG Bricks / Stone wall | https://ambientcg.com/list?category=Bricks | CC0 |
| Medieval Village / Castle (Fab bepul) | https://www.fab.com/search?q=medieval%20castle&is_free=1 | Bepul/CC |
| Kenney (oddiy props) | https://kenney.nl/assets/category:3D | CC0 |
| Quaternius Medieval Village | https://quaternius.com/packs/medievalvillage.html | CC0 |

## Ot (skeletli)
| Nom | Havola | Litsenziya | Ishlatish |
|---|---|---|---|
| Horse Animset (Fab, bepul emas, lekin arzon) | https://www.fab.com/search?q=horse%20animset | tijoriy | eng sifatlisi |
| Sketchfab Horse rigged (CC-BY) | https://sketchfab.com/search?q=horse+rigged+animated&features=downloadable&licenses=322a749bcfa841b29dff1e8a1bb74b0b | CC-BY | FBX → horse.mesh, anims: idle/walk/trot/gallop |
| Quaternius Animals (ot bor) | https://quaternius.com/packs/ultimateanimatedanimals.html | CC0 | past poligonli, animatsiyali |
| Poly Haven horse (statik) | https://polyhaven.com/models?s=horse | CC0 | faqat bezak |

## Import qadamlari (muharrir)
1. Fab: Library → "Add to project" (yoki saytdan yuklab, `Content/ErtAssets` ga sudrash).
2. Personaj/ot: Content Browser'da skeletal mesh yo'lini nusxalab `Content/Ertugrul/Data/character.json` ga yozing:
   `"mesh": "/Game/Fab/.../SK_Kwang"`, `anims.idle/walk/run/attack/death` — o'sha paketdagi AnimSequence yo'llari.
3. Teksturalar: `T_<rol>_D` / `T_<rol>_N` nomi bilan `/Game/ErtAssets/Tex` ga qo'ying (grass/dirt/sand/rock/snow/cobble/stone/brick/wood).
4. Muharrirni qayta oching — hech qanday kod o'zgarishi kerak emas.
