# Unreal Engine 5.8 porti (C++)

Loyiha: `D:\Unreal_projects\Ertugrul\Ertugrul.uproject` (manbalar shu repoda `tools/unreal/` ostida nusxalangan).

## Qurish
- Toolchain: VS Build Tools 2022 (`D:\BuildTools`, MSVC 14.44) + .NET Framework 4.8 SDK (SwarmInterface uchun shart).
- Build: `D:\UE_5.8\Engine\Build\BatchFiles\Build.bat ErtugrulEditor Win64 Development -Project="D:\Unreal_projects\Ertugrul\Ertugrul.uproject" -WaitMutex -NoHotReload`
  (muharrir ochiq bo'lsa Live Coding tashqi buildni bloklaydi - muharrirni yoping).
- DDC/Zen keshi `UE-LocalDataCachePath=D:\UE_DDC` (C: to'lib qolmasligi uchun).
- Daraja va materiallar: `tools/unreal/Scripts/ert_make_world.py` (`UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<py>`).

## Modullar (`Source/Ertugrul`)
- `ErtProcMesh` - protsedural geometriya (quti/silindr/konus/shar/to'r, vertex rang). UE chap qo'l tizimi: old yuz = `-Cross(B-A, C-A)`.
  `Unit` maydoni: 100 = radius/balandlik metrda (dunyo), 1 = sm (personaj). Vertex ranglar sRGB konversiyasiz.
- `ErtCharacter` - yurish 135 / yugurish 330 / chopish 620 sm/s, stamina, sakrash, cho'kish, qiyalikda sirpanish,
  to'siqqa chiqish (mantle, 45-185 sm), orbital kamera. Enhanced Input xaritalari kodda (klaviatura+gamepad).
  `-ErtShot=<papka>` bilan avtomatik sinov: harakat ssenariysi + 12 skrinshot.
- `ErtHeroBody` - 11 bo'g'imli protsedural tana, pozalar kodda (idle/yurish/yugurish/havo/cho'kish).
- `ErtWorldBuilder` - 2000x2000 m dunyo: relyef (Perlin, tog' massivi, tekisliklar), daryo+ko'l, yo'llar,
  Qayi obasi (250x250 reja), tosh qal'a (232 m cho'qqi), devorli shahar (R=190, oltin gumbaz), mo'g'ul lageri (R=150), o'rmon, toshlar.
  Komponentlar `UserConstructionScript` sifatida yaratiladi - darajaga saqlanmaydi, har yuklashda qayta quriladi (~2-4 s).
- `ErtGameMode` - DefaultPawn = ErtCharacter.

## Missiyalar (C++ dvijokdan ko'chirildi)
- `ErtEpisodeDb` - `Content/Ertugrul/Data/episodes_v2.json` (48 epizod) ni o'qiydi; `ErtLoc` - `ui_loc.csv` + `episodes_loc.csv` (uz/tr/en).
- `ErtMissionDirector` - epizod arxetipidan bosqichlar zanjiri (asl `Encounter.cpp` bilan bir xil ssenariylar):
  SIEGE/DEFENSE: Yo'l -> Himoya -> Yo'l -> Jang; SURVIVAL: Ov -> Yo'l -> Jang; INFILTRATION: Yo'l -> Yashirin -> Qidiruv -> Yo'l;
  CHASE: Yo'l -> Jang -> Yo'l -> Yakkama-yakka; ESCORT: Yo'l -> Himoya -> Yo'l -> Jang; RITUAL: Qidiruv -> Ov -> Yo'l -> Yakkama-yakka;
  COURT: Qidiruv -> Yo'l -> Yashirin -> Yakkama-yakka; INVESTIGATION: Yo'l -> Qidiruv -> Yashirin -> Jang.
  Maqsadlar: DefeatAll, SurviveTime, StayUndetected (sezilsa qo'shimcha kuch), Route (tom/qoya ustidagi nuqtalar - kolliziya trace bilan),
  Hunt (kiyik), Collect, HoldPoint, Timer. Nazorat nuqtasi har bosqich boshida; o'lim -> 3.5 s -> qayta boshlash; epizod tugadi -> 7 s -> keyingi (unlocks[0]).
  Har arxetip xaritaning o'z hududida boshlanadi (oba, qal'a yonbag'ri, o'rmon, lager g'arbi, chorraha, daryo yo'li, shahar shimoli).
  Taraqqiyot: `Saved/ert_progress.txt`.
- `ErtEnemy` - Footman/Sergeant/Crossbow/Elite (protsedural tana, dubulg'a, qilich) va Deer (4 oyoqli, qochadi).
  AI: patrul, ko'rish konusi + LOS trace (cho'kkan o'yinchi 900 sm dan), ta'qib, zarba (kechiktirilgan 0.22 s), kamonchi masofadan.
- `ErtCharacter` jang: LMB qilich (sfera 115 sm, 30 zarar), RMB blok (yuzma-yuz 80% kamaytiradi, stamina), F kamon (kamera nuri 50 m, 45 zarar, o'qlar 12/16), sog'liq 100 (5 s dan keyin tiklanadi).
- `ErtHUD` - epizod/bosqich/maqsadlar, sog'liq/stamina/o'q, markerlar (masofa bilan), dushman sog'liq chiziqlari, holat xabarlari.
- Sinov: `-ErtEpisode=EP003 -ErtPhase=1` bilan istalgan bosqichdan boshlash; `-ErtFreeRoam` missiyasiz.

## Kat-sahnalar, menyu, cho'l, suzish, qadamlar
- `ErtCutsceneDirector` - `Content/Ertugrul/Data/cutscenes/<epizod>_intro.json` (49 sahna, C++ dvijokniki bilan bir xil format):
  aktyorlar (protsedural figuralar, id dan deterministik rang) kalitlar bo'ylab yuradi, kamera kalitlar orasida smoothstep,
  subtitr + gapiruvchi (`cutscene_loc.csv`), letterbox, fade. Space/Enter - keyingi replika, Esc - tashlab ketish.
  Dvijok koordinatalari (x o'ng, y yuqori, z oldinga, metr) -> UE (X=z, Y=x, Z=yer+y) epizod anchor atrofida, yer trace bilan.
- `ErtGameMode` oqimi: menyu -> kat-sahna -> missiya -> keyingi epizod. Menyu (Esc/Tab): 48 epizod, ochilgan/bajarilgan/qulf,
  o'ng panelda sinopsis. `-ErtUnlockAll`, `-ErtEpisode=EP003 -ErtCutscene` (to'g'ridan-to'g'ri, kat-sahna bilan), `-ErtFreeRoam`.
- Dunyo qo'shimchalari (darsliklardagi g'oyalar resurssiz): janubiy cho'l (barxan shovqini, qum rangi), voha ko'li (55 m) + 26 palma
  (egri tana + 8 barg), karvonsaroy (40x40, minoralar, gumbaz, hujralar), qadimiy xarobalar, shimoliy tog' tizmasi, oba yonidagi ko'l chuqurlashtirildi.
  Post-process volume (saturation/contrast/bloom) va tuman Python orqali.
- Suzish (Water plaginisiz): `AErtWorldBuilder::IsWater` (daryo/ko'l/voha, sath balandligi) -> oyoq sathdan 105 sm past bo'lsa `MOVE_Flying`
  + suzuvchanlik (kapsula sathdan 35 sm past), 240/330 sm/s, stamina sarfi, tana gorizontal suzish pozasi; qirg'oqda yurishga qaytadi.
- `ErtFootsteps` - qadam changi (6 ta pooled protsedural shar, `M_ErtDust` translucent, qumda kattaroq) va protsedural qadam tovushi
  (`USoundWaveProcedural`: shovqin + konvert + past chastota filtri, tasodifiy balandlik; qumda yumshoqroq). Qadam oralig'i tezlikka bog'liq.

## Render sozlamalari (Intel iGPU)
Lumen/VSM/Nanite/Distance Fields o'chirilgan, FXAA, auto-exposure yoqilgan, bulut (VolumetricCloud) olib tashlangan
(SM5 da bulut shaderlari har biri ~1 daqiqa kompilyatsiya bo'lardi).

## Saboqlar
- Python `unreal.Rotator(roll, pitch, yaw)` - argument tartibi! Quyosh ufq ostiga tushib, sahna qop-qora bo'lgan edi.
- Ichkariga qaragan yuzalar kolliziya normallarini ham teskari qiladi: personaj doim "havoda" bo'lib sakray olmaydi.
- Bash heredoc ichida apostrof (') bo'lgan uzun skriptlar tool tomonidan buziladi - patchlarni Write bilan faylga yozib `python patch.py` bilan qo'llang.
- `UnrealEditor-Cmd -game` oynasini yopish `Ctrl+C` (0xC000013A) bilan tugatadi - sinovni `-WindowStyle Hidden` bilan ishga tushiring.

## Keyingi qadamlar
Missiyalar/epizodlar, jang, kat-sahnalar (mavjud JSON), NPC, lokalizatsiya, ovoz.
