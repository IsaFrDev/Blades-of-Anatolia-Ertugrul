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

## Ot minish va parry
- `AErtHorse` - protsedural ot (tana, bo'yin/bosh, yol, dum, 4 oyoq, egar, uzangi), `ACharacter` asosida (relyef, gravitatsiya).
  [E] minish/tushish (320 sm ichida), W yurish 250 / yo'rtish 520, Shift chopish 950 sm/s, A/D burilish (tezlikda radius kengayadi),
  Space sakrash, orqaga 140. Oyoq animatsiyasi: yo'rtishda diagonal juftlar, chopishda oldingi/orqa juftlar; bosh tebranishi.
  Minilmaganda o'tlab yuradi. Chavandoz egarga attach, poza: sonlar oldinga-yonga (LegSpread), tizza 88, qo'llar jilovda.
  Otlar: oba darvozasi oldida 2 ta; `traversal` da HORSE/CAMEL bo'lgan epizodlarda boshlanish nuqtasi yonida.
- Parry: RMB bosilgach 0.25 s ichida zarba kelsa (yuzma-yuz) - zarar yo'q, "PARRY!" chaqnashi, raqib 1.3 s gangiydi (Stagger),
  keyingi qilich zarbasi 1.6 s davomida x2 (riposte). Oddiy blok: zarar x0.2, stamina -12. Blok/parry pozalari tanada.
- MUHIM: kontrollersiz `ACharacter` (dushman, ot) harakat qilishi uchun `bRunPhysicsWithNoController = true` shart -
  aks holda PhysWalking tezlikni nolga tenglashtiradi (dushmanlar ham shu sababli joyida turgan edi).

## Otliq dushmanlar va NPC dialoglari
- `EErtEnemyKind::Rider`: ot + chavandoz (`AErtEnemy::MountHorse`). AI otni boshqaradi: uzoqda chopib keladi, 2.6 m dan yaqinda aylanib o'tadi,
  70 gradus ichida bo'lsa zarba (300 sm, 15 zarar). Ko'rish 26 m. O'lsa otdan yiqiladi, ot bo'shaydi - o'yinchi minishi mumkin.
  Tier >= 2 da 14%, CHASE/ESCORT jangida 30%. O'yinchi qilich sferasi 150 sm (+45 sm balandlik) - otliqqa yetadi.
- Dialog grafi (`FErtDialog`): `data/dialogue/*.json` formati (nodes/start, options: text_key, next, set_flag, honor, requires_evidence).
  Matnlar `ertugrul_loc.csv` (DLG_*) va `npc_loc.csv` dan. Or/iymon hisobi (`Honor`) va bayroqlar (`Flags`) GameMode da;
  maxsus bayroqlar: give_arrows (+8 o'q), sword_sharpened (zarar 36).
- `AErtNpc` + `npcs.json`: 8 NPC (Hayma Ona, Halima, Turgut, Deli Demir, Bamsi, Gundo'g'di obada; savdogar shaharda; karvonboshi vohada),
  reja koordinatalari (place + u,v), ayol varianti (ro'mol, uzun ko'ylak, soqolsiz). Yaqinlashganda o'yinchiga qaraydi, [E] dialog.
  HUD: panel, gapiruvchi, tanlovlar (1-4 yoki Yuqori/Pastga+Enter), Esc chiqish. `tools/unreal/Scripts/make_npc_dialogs.py` NPC graflarini yaratadi.
- UE 5.8: `FJsonObject::Values` kaliti `UE::FSharedString` - `FString(Key.ToView())` bilan o'giriladi.

## Kengash dialog-dueli va qo'lda missiyalar
- `Content/Ertugrul/Data/missions.json`: epizod bosqichlarini qo'lda belgilash (travel/collect/council/stealth/defend/hunt/duel/fight).
  `collect` da har nuqta uchun `flags` (dalil bayroqlari), `council` da dialog grafi, `threshold` (duel ballari) va Bey chodiri oldida NPClar.
- EP002 "Ikki asir": Yo'l -> 4 dalil (letter, seal, selcan_talk, aykiz_witness) -> Kengash (`council_kurdoglu`: dalillar tanlov sifatida
  faqat bayroq bo'lsa chiqadi, har biri 1 ball, 3 ball = g'alaba, Kurdo'g'li fosh bo'ladi) -> Yo'l -> Jang. EP001: dalillar -> Halima bilan suhbat (`e1_camp_halima`).
- EP003: Yo'l -> Himoya (2 to'lqin) -> o'layotgan Templar serjanti (`ep003_sergeant`, rahm/so'roq tanlovi, Titus nomi) -> Jang.
  EP004: dalillar (father_letter, tribe_seal) -> Sulaymon Shoh (`ep004_father`, 1 ball) -> Yashirin. EP005: Ov -> Yo'l -> Bamsi (`ep005_bamsi`) -> Jang.
  EP006: 3 dalil (tracks, torn_cloth, dagger) -> Turgut (`ep006_turgut`, 2 ball, xanjar = 2) -> Yashirin -> Jang. Generator: `tools/unreal/Scripts/make_ep_dialogs.py`.
- Cliffhanger ekrani: epizod tugagach qora fon, sarlavha, `ep.epXXX.cliffhanger` matni satrma-satr, statistika; 16 s yoki Enter/Space bilan keyingi epizod.
- Dialogda ishlatilgan dalil bayroqdan o'chiriladi (qayta ko'rsatib bo'lmaydi). Natija HUD da: "Kengash sizni qo'lladi!" yoki "Dalillar yetarli emas edi".

## Ovoz, saqlash, sozlamalar, EP007-EP012
- `FErtAudio`: `assets/audio` dagi WAV fayllar import qilinmasdan ish vaqtida o'qiladi (`Content/Ertugrul/Data/audio/`, RIFF parser -> `USoundWaveProcedural`).
  SFX: swing/hit/kill/block/parry/bowshot/arrow_hit/arrow_wall/death (o'yinchi va dushman); VO: kat-sahna replikalari `vo/<til>/<id>.wav` (311 x 3 til).
- Saqlash `Saved/ert_save.json`: bajarilgan epizodlar, bayroqlar, or/iymon, til, sichqoncha sezgirligi, Y teskari. Dialog tugaganda va epizod bajarilganda yoziladi.
- Sozlamalar (O tugmasi): til (uz/tr/en - darhol qo'llanadi), sezgirlik 0.2-3.0, Y teskari. Chap/O'ng yoki Enter bilan o'zgartiriladi.
- EP007-EP012 qo'lda ssenariylar va 6 dialog (Al-Aziz, Ibn Arabiy, Gundo'g'di, yarador Turgut, Ko'pek, No'yon elchisi); generator `make_ep7_12.py`.

## Kun/tun, barcha 48 epizod, lagerga bosqinchilik
- Kun/tun sikli (`AErtGameMode::Tick`): 20 daqiqalik kun, quyosh balandligi/rangi/intensivligi va osmon nuri DayT bo'yicha; epizodning
  `time_of_day` (dawn/day/dusk/night) boshlang'ich vaqtni beradi.
- EP013-EP048: arxetip shablonlari bilan `make_ep13_48.py` - har epizodga kengash/suhbat dialogi (COURT/INVESTIGATION dalil dueli 2 ball;
  SIEGE elchi; DEFENSE Turgut; SURVIVAL Bamsi; INFILTRATION asir kotib; CHASE yarador Dundor; ESCORT Axi Evren; RITUAL Hoji Bektosh) va bayroqlar `<ep>_...`.
- INFILTRATION yashirin bosqichi: qorovullar mo'g'ul lageri ichida (CampE/CampN +-60 m) - o'yinchi lagerga kirib boradi.
- Ot ustida kamon: F tugmasi minib turganda ham ishlaydi (kamera nuri).

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
