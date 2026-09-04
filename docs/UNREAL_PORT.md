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

## Oba hayoti, ijro, ob-havo, vibratsiya
- NPClar kunduzi uyi atrofida 1.5-6 m yuradi (yer trace, to'siq tekshiruvi), tunda uyida turadi, o'yinchi yaqinlashsa to'xtab qaraydi, suhbatda qimirlamaydi.
- Ijro (execute): gangigan (parry) yoki sog'lig'i 25% dan kam raqibga qilich zarbasi - bir zarbda o'ldiradi, "IJRO!" chaqnashi, kuchli vibratsiya.
- `AErtWeather`: epizod `weather` (clear/rain/snow/dust/fog) - kamera atrofida 3000 sm qutida protsedural zarralar (yomg'ir chiziqlari, qor parchalari,
  chang), pastga siljish va shamol drifti, tuman zichligi. Bitta mesh, kameraga ergashadi.
- Gamepad vibratsiyasi: zarar olganda (kuchi zararga bog'liq), parry, ijro.

## Menyular, xarita, zarba reaksiyasi
- Menyu holati (`EErtMenu`): Main (o'yin boshida: Boshlash/Davom etish, Epizodlar, Sozlamalar, Chiqish), Pause (Esc: Davom etish, Xarita,
  Epizodlar, Sozlamalar, Saqlab chiqish), Episodes, Settings, Map. Menyu ochiq bo'lsa o'yin pauzada (`SetGamePaused`), menyu kirishlari
  `bTriggerWhenPaused` bilan ishlaydi. Esc har doim bir qadam orqaga.
- Xarita (M): pergament fon, cho'l/tog' zonalari, daryo, obyektlar (oba, qal'a, shahar, lager, voha, karvonsaroy, ko'l, xarobalar),
  maqsad markerlari (oltin), dushmanlar (qizil), NPClar (ko'k), o'yinchi (yashil, yo'nalish). Minimap doim o'ng yuqorida (250 m radius).
- Zarba reaksiyasi: dushman zarba yeganda orqaga uchadi (kuchi zararga bog'liq) va 0.35 s gangiydi; o'yinchi zarba yeganda orqaga siljiydi,
  kamera silkinadi; har tegishda hitstop (0.07 s, ijroda 0.22 s).

## Qulflash, dodge, ogohlantirish
- Q / o'rta sichqoncha / chap stik: eng yaqin oldindagi dushmanga qulflash - personaj nishonga qaraydi (strafe), kamera yumshoq ergashadi,
  zarba nishon tomon; o'lsa yoki 26 m dan uzoqlashsa ochiladi.
- X / B: dodge - kirish yo'nalishida sakrab qochish (kirish yo'q bo'lsa orqaga), 0.5 s, 0.2 s dan keyingi qismi daxlsiz, stamina -12.
- Dushman zarba oldidan (0.22 s) boshida qizil "!" chiqadi - parry/dodge vaqti. Qulflangan nishonda oltin retikul.

## Inventar va daraja
- XP: dushman 20-80 (turiga qarab), kiyik 10, epizod 150. Daraja chegarasi 80 + 60*(L-1). Daraja oshganda sog'liq +10, stamina +5, zarar +3, to'liq shifo, "DARAJA N!".
- Oltin: dushmandan 4-14, epizod 40. Dori (H): +45 sog'liq. Inventar (I): daraja/XP, sog'liq, oltin, dori, o'qlar, jihoz, zarar.
- Jihoz: Damashq qilichi (+12 zarar, tig' rangi), yog'och qalqon (chap bilakda mesh, blok 95%, stamina -6), kompozit kamon (+20 zarar, 24 o'q).
- Savdogar Yusuf (shahar bozori) dialogi orqali sotib olish: bayroqlar buy_* -> EndDialog da narx tekshiruvi (dori 15, 8 o'q 10, qalqon 60, kamon 90, qilich 120).
- Saqlash: gold/potions/arrows/level/xp/sword/bow/shield `ert_save.json` da; o'yinchi paydo bo'lgach 0.5 s da qayta qo'llanadi.

## Jang kombinatsiyalari
- LMB seriya: o'ngdan -> chapdan -> yakunlovchi (1.6x, gangitadi 0.6 s, uloqtiradi). Oyna 1.1 s, HUD da x2/x3.
- LMB ushlab turish (0.35 s): og'ir zarba (2.2x, ikki qo'l tepadan, qalqonni sindiradi, 0.6 s gangitadi, stamina -14, cd 1.05 s).
- V: tepki (0.3x, gangitadi 0.9 s, 4.2 m uloqtiradi, qalqonni chetlab o'tadi).
- Qalqonli dushmanlar (serjant, elita, otliq): yuzma-yuz yengil zarbani 45% to'sadi (blok tovushi, qisqa kamera silkinishi); og'ir zarba/tepki/gangigan holatda yo'q.

## Ov o'ljasi va No'yon dueli
- O'lgan kiyik yonida [E]: +2 kiyik go'shti (+5 XP). H: dori bo'lmasa go'sht yeyiladi (+25). Inventar va saqlashda.
- Boss `EErtEnemyKind::Boss` (No'yon): 400 sog'liq, 22 zarar, 60% qalqon, gangish yarim, uloqtirish 35%, ijro faqat 12% dan past;
  har 8 s to'sib bo'lmaydigan og'ir zarba (0.6 s ogohlantirish, 2x zarar, blok/parry ishlamaydi - faqat dodge). XP 400.
- `missions.json` "boss" bosqichi: EP012 (Bagras), EP035, EP047 yakunida (guards - 2 ta elita hamroh). HUD: tepada No'yon sog'liq chizig'i,
  og'ir zarba oldidan "OG'IR ZARBA - DODGE (X)!".
- Patch skriptlarida qo'shish-turidagi almashtirishlar takrorlanib qolmasligi uchun `rep` avval `b in s` ni tekshirsin (bir marta duplikatlar chiqdi, mirror'dan tiklandi).

## Yon kvestlar
- `sidequests.json`: `episodes.json` dagi 11 yon kvest (Oyqiz, Ibn Arabiy, Deli Demir, Gundo'g'di, Afshin Bey, Selchan, Banu Chichek,
  Geyikli Bobo, Artuq Bey, Dundor, Hamza) - har biri beruvchi NPC, bosqichlar (missions.json formati), mukofot (XP, oltin, or/iymon).
- Beruvchi NPC dialogida "Kvest: ..." varianti (`requires_evidence: sq_avail_<id>` - bajarilmagan bo'lsa ko'rinadi), qabul qilinsa
  `sq_start_<id>` bayrog'i -> EndDialog -> `StartSideQuest` (o'yinchi turgan joydan, teleportsiz; faqat epizod faol bo'lmaganda).
  Tugagach: mukofot, `sq_done_<id>`, beruvchining yakuniy so'zi cliffhanger o'rnida (6 s), keyingi epizod yo'q.
- 9 yangi NPC (jami 17), yangi joy: forest (-330, 700). Xarita (M) o'ng ustunida kvest jurnali: [ ] mavjud, [>] faol, [x] bajarilgan.
- `StartEpisode` -> `StartEpisodeData(E, Start, PhaseOverride)` ga ajratildi; override JSON obyekti bo'lsa missions.json o'rniga u ishlatiladi.

## Ot sog'lig'i va or/iymon ta'siri
- Ot: 200 sog'liq; chavandozga kelgan zararning 40% otga (chavandoz 60%), dushman otliqlarida 30%. Sog'liq 0 -> ot yiqiladi,
  chavandoz uloqtiriladi (+15 to'sib bo'lmas zarar), 25 s dan keyin yo'qoladi. Minilmaganda 3/s tiklanadi. HUD: minganda "Ot N" chizig'i.
- Or/iymon: >= 10 shifo x1.6 va narxlar -15%; <= -5 narxlar +30% va savdogar "nomingiz yaxshi emas" deydi; <= -10 shifo x0.6 va hech kim
  yon kvest bermaydi; >= 15 oddiy askarlar sog'lig'i 30% dan tushganda qochadi. Unvon: Sharafli Bey / Mo'tabar / Oddiy / Shubhali / Badnom.
- Dialog bayroqlari `honor_high` / `honor_low` (`requires_evidence`): Hayma Ona duosi (+2 dori, to'liq shifo), savdogarning shikoyati.

## Ko'rinadigan o'qlar va kvest mukofotlari
- `AErtArrow`: protsedural o'q (dasta, uch, pat), yengil gravitatsiya, segment-nuqta masofasi bilan tegish, yerga sanchiladi (14 s).
  Kamonchi dushmanlar endi haqiqiy o'q otadi (o'yinchi tezligiga qarab oldinga mo'ljal, +-40 sm tarqalish) - dodge/qochish ishlaydi.
  O'yinchi kamoni: kamera nuri nishoniga qarab qo'ldan uchadi (4200 sm/s), tegsa zarar/XP/oltin.
- Yon kvest mukofotlari (`sidequests.json` "reward"): pelt (bo'ri terisi zirhi, zarar -15%, saqlanadi), potions3, arrows12, xp100, shield, bow, meat6.
  Yakuniy ekranda mukofot matni ko'rsatiladi; inventarda "Zirh" qatori.

## Tugmalarni sozlash
- Sozlamalar (O) -> "Tugmalar >" sahifasi: 15 harakat (sakrash, chopish, cho'kish, yurish, zarba, blok, kamon, gaplashish/minish,
  dodge, qulflash, tepki, inventar, dori, xarita, sozlamalar). Enter -> "tugmani bosing..." -> keyingi bosilgan klaviatura/sichqoncha tugmasi
  (`WasInputKeyJustPressed` bilan barcha `EKeys` bo'yicha), Esc bekor qiladi. Gamepad xaritalari o'zgarmaydi.
- `AErtCharacter::SetBinding` IMC dagi klaviatura xaritalarini olib tashlab yangisini qo'yadi va `RequestRebuildControlMappings` chaqiradi.
  `ert_save.json` "keys" obyektida saqlanadi, o'yin boshida qo'llanadi.

## O'lja tushishi
- `AErtLoot`: dushman (kiyikdan tashqari) o'lganda yerga to'rva tushadi (aylanib tebranadi, 90 s). Tarkibi: oltin 4-14 (elita/otliq 10-26,
  boss 150), o'q (kamonchi 3-7, boshqalar 35% da 1-3), dori (12%/35%, boss 3). [E] bilan olinadi, HUD "O'lja: ..." ko'rsatadi.
  Oltin endi faqat o'ljadan (o'ldirganda XP saqlanadi).

## Ovozli salomlashuv va dialog ovozi
- NPC 4 m ga yaqinlashganda salomlashadi (20 s da bir): or/iymon >= 10 - hurmatli, <= -5 - sovuq, ayollar - "choy ichasizmi?"; boshi ustida matn 3 s.
- Dialog tugunlari ochilganda `vo/<til>/<text_key>.wav` o'ynaydi (bo'lsa). Yopilganda to'xtaydi.
- Placeholder ovozlar `tools/unreal/Scripts/gen_tts.ps1` bilan Windows SAPI (Zira, en-US) dan yaratildi: 552 fayl (DLG_* va greet.* x 3 til,
  ~180 kalit). Haqiqiy diktor yozuvlari shu nomlar bilan almashtiriladi. Fayllar repoga qo'shilmaydi (Content/Ertugrul/Data/audio).

## Ekran sozlamalari
- Sozlamalar qatorlari 5-8: Ekran rejimi (to'liq / oynali to'liq / oyna), O'lcham (1280x720, 1600x900, 1920x1080, 2560x1440),
  Grafika sifati (Past/O'rta/Yuqori/Epik - `SetOverallScalabilityLevel`), VSync. `UGameUserSettings::ApplySettings` + `SaveSettings`
  (GameUserSettings.ini). Chap/O'ng yoki Enter bilan o'zgartiriladi. Intel GPU uchun tavsiya: Past/O'rta, VSync yoqilgan.

## Oba faoliyatlari
- Kamon musobaqasi (Turgut dialogi): mashq maydonida 3 ta `AErtTarget` (somon halqa, kolliziyali), +10 o'q, 60 s ichida 5 tegish;
  o'q nishonga tegsa ochko va tebranish. G'alaba: +30 oltin, +60 XP, +1 or.
- Kurash (Bamsi dialogi): 12 s, LMB tez bosish kuch chizig'ini oshiradi, Bamsi qarshiligi vaqt o'tgan sari kuchayadi; 100% - g'alaba
  (+50 XP, +2 or), 0% - mag'lubiyat. HUD paneli va natija matni.
- Dialogdagi `act_archery` / `act_wrestle` bayroqlari -> `StartActivity(1|2)`.

## Tuya va vault
- `AErtHorse::Init(Coat, bCamel)`: tuya varianti (o'rkach, uzun bo'yin, uzun oyoqlar, keng tovon, gilam va egar, popuklar), yo'rg'a yurish,
  260 sog'liq, tezlik 200/420/760, qumda sekinlashmaydi (ot qumda 0.75x). Karvonsaroy oldida 2 ta tuya; `traversal` da CAMEL bo'lgan
  epizodlar (EP008) tuya bilan boshlanadi, ESCORT+CAMEL anchor - karvonsaroy.
- Vault (ACRPGVaultingComponent::FindVaultTargets asosida, montajsiz): yugurib 40-120 sm to'siqqa Space - usti tekis va orqasida joy bo'lsa
  0.45 s parabola bilan sakrab o'tadi (kolliziya vaqtincha o'chadi), qo'ngach oldinga tezlik. Mantle mexanizmi bilan bir xil holat.
- MetaHuman: Quixel Bridge orqali Epic akkaunt bilan yuklab olinadi (tashqi asset, ~1-2 GB, Intel iGPU ga og'ir) - buyruq qatoridan
  avtomatlashtirib bo'lmaydi; Geometry Script plaginidagi AppendCylinder/AppendBox bizning FErtMeshData bilan ekvivalent.

## Qayiq
- `AErtBoat`: protsedural korpus (tub, qiya bortlar, tumshuq, o'rindiqlar) + 2 eshkak (tezlikka qarab eshadi), suv sathida tebranadi,
  `IsWater` hududidan chiqmaydi (qirg'oqqa urilsa to'xtaydi, tovush). W 380 sm/s, S orqaga, A/D burilish.
- [E] o'tirish (3.5 m ichida) / chiqish - eng yaqin qirg'oq nuqtasiga (16 yo'nalish, 3-30 m) qo'yadi. O'tirganda o'tirish pozasi, kamera uzoqroq,
  jang/sakrash o'chadi. Daryoda 2 ta (N=480, 300), ko'lda 1 ta.

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

## Damashq shahri

- `BuildDamascus()` (ErtWorldBuilder): cho'lning janubi-sharqida (E=720, N=-850, 280x220 m) devorli shahar: kungurali devor, 4 burj, 2 darvoza (janub/g'arb), Umaviylar masjidi (marmar hovli, gumbaz, 34 m kvadrat minora), saroy, bozor arkadasi, ~100 uy, palmalar. Relyef `HeightAt` da tekislanadi, `IsBuildable` chiqarib tashlaydi.
- Xarita: `Damashq` belgisi; NPC joyi `damascus` (Damashq savdogari, Ibn Arabiy); Sham/Halab/Damashq epizodlari (EP007, EP008) shimoliy darvoza oldidan boshlanadi.
- Sinov skrinshoti: `16_damascus`.

## Halab shahri

- `BuildHalab()` (ErtWorldBuilder): sharqda (E=800, N=120, R=130 m) aylana shahar: 16 burchakli kungurali devor, 8 burj, sharqiy darvoza; markazda 24 m glasis tepaligida qal'a (halqa devor, burjlar, saroy, hammom gumbazlari, minora) va tepalikdan tushuvchi arkali ko'prik; yopiq gumbazli bozor ko'chasi, halqa-halqa uylar, katta masjid (dumaloq minora), sarvlar. Tepalik `HeightAt` da yasaladi (yurish mumkin).
- Xarita: `Halab` belgisi; NPC joyi `halab` (Halab savdogari, Al-Aziz); `Halab` epizodlari (EP007) sharqiy darvoza oldidan boshlanadi, Sham/Damashq esa Damashqdan.
- Sinov skrinshoti: `17_halab`.

## Konya shahri

- `BuildKonya()` (ErtWorldBuilder): shimoliy-markazda (E=120, N=480, R=150 m) Saljuqiylar poytaxti: 24 burchakli devor, har burchakda kvadrat burj, sharqiy va g'arbiy darvozalar; markazda 9 m Aloiddin tepaligi (masjid, feruza gumbaz, kvadrat minora, chodir tomli saroy ko'shki, ichki devor); konus tomli kumbetlar, madrasa (portal, naqshli baland minora), karvonsaroy, bozor, sopol tomli ayvonli uylar, chinor xiyoboni. Yo'l (300,300) dan sharqiy darvozaga.
- Xarita: `Konya` belgisi; NPC joyi `konya` (Konya savdogari, Sa'duddin Ko'pek); Konya/Kubadabad/Kayseri epizodlari sharqiy darvoza oldidan boshlanadi.
- Sinov skrinshoti: `18_konya`.

## Qayseri shahri va Erciyes

- `BuildKayseri()` (ErtWorldBuilder): markaz-sharqda (E=450, N=100, R=125 m) qora bazalt shahar: 20 burchakli devor (oq choklar, kungura, yarim dumaloq burjlar), g'arbiy va shimoliy darvozalar; to'rtburchak ichki qo'rg'on (8 burj, saroy, qo'rg'oshin gumbaz); Hunat Xotun majmuasi (masjid, minora, madrasa, sakkiz qirrali kumbet); gumbazli yopiq bozor (bedesten); tekis tomli mo'rili uylar; teraklar.
- Erciyes: relyefda vulqon konusi (E=470, N=-150, R=130 m, 120 m baland), bazalt qoya rangi, cho'qqisi qorli; daraxt/qoya qo'yilmaydi.
- Xarita: `Qayseri`, `Erciyes` belgilari; NPC joyi `kayseri` (Qayseri savdogari, subashi); `Kayseri` epizodlari g'arbiy darvoza oldidan boshlanadi. Yo'l (300,300) dan g'arbiy darvozaga.
- Sinov skrinshoti: `19_kayseri`.

## Sivas shahri

- `BuildSivas()` (ErtWorldBuilder): shimoli-g'arbda (E=-180, N=400, R=120 m) Saljuq madrasalar shahri: 14 burchakli tosh devor, burjlar, janubiy va sharqiy darvozalar; shimolda past qal'a tepaligi (devor halqasi, burj, saroy); Go'k Madrasa (feruza koshinli portal, qo'sh minora), Chifte Minorali madrasa (baland fasad, qo'sh minora), Burujiya madrasasi, Ulu Jome' (keng past bino, qiyshaygan g'isht minora); yog'och soyabonli bozor, g'isht-tosh uylar, tollar. Yo'l (-300,250) dan janubiy darvozaga.
- Xarita: `Sivas` belgisi; NPC joyi `sivas` (Sivas savdogari, Go'k Madrasa mudarrisi); `Sivas` epizodlari janubiy darvoza oldidan boshlanadi.
- Sinov skrinshoti: `20_sivas`.

## Erzurum shahri

- `BuildErzurum()` (ErtWorldBuilder): shimoli-sharqda (E=420, N=730, R=110 m, 22 m balandlik) sovuq tekislik shahri: 12 burchakli qalin kulrang devor, qiya tomli kvadrat burjlar, janubiy va g'arbiy darvozalar; shimoli-sharq tepaligida qal'a (ichki devor, Tepsi minora soat xonasi bilan, saroy); Chifte Minorali madrasa (baland portal, qo'sh minora, orqada katta kumbet); Uch Kumbet; Ulu Jome' (ko'p gumbazli tom, kalta minora); tunuka tomli tosh do'konlar; past tuproq tomli uylar (mo'ri, tomda qor); qarag'aylar. Yo'l (500,520) dan janubiy darvozaga.
- Xarita: `Erzurum` belgisi; NPC joyi `erzurum` (savdogar, Erzurum beyi); `Erzurum`/`Erzincan` epizodlari janubiy darvoza oldidan boshlanadi.
- Sinov: `27_erzurum` skrinshoti (ssenariy oxirida); xarita skrinshoti uchun xarita 0.7 s ochiq turadi (kadr kechikishi tufayli).

## Bursa shahri va Uludog'

- `BuildBursa()` (ErtWorldBuilder): g'arbda (E=-540, N=60, R=120 m) ochiq (devorsiz) yashil shahar: shimolda Hisor tepaligi (tosh devor, burjlar, Usmon va O'rxon maqbaralari, bey saroyi); Ulu Jome' (4x5 = 20 gumbaz, ikki minora, shadirvon); Yashil masjid (feruza fasad) va sakkiz qirrali Yashil maqbara; Koza xon (ikki qavatli hovli, ustunlar ustidagi masjidcha); hammom (gumbazlar, bug'); oq suvoqli chiqma ayvonli sopol tomli uylar; chinorlar; tashqarida tut bog'lari. Yo'l (-300,250) dan sharqiy kirishga.
- Uludog': relyefda o'rmonli tog' (E=-620, N=-200, R=110 m, 95 m), cho'qqisi qorli.
- Xarita: `Bursa`, `Uludog'` belgilari; NPC joyi `bursa` (ipak savdogari, qozi); `Bursa`/`Nikeya` epizodlari sharqiy kirishdan boshlanadi.

## Realistik ot

- `AErtHorse::Build` qayta yozildi: ellipsoid tana (bochka, sag'ri, son, ko'krak, yag'rin), 52 gradus egilgan konus bo'yin (`NeckMesh`) va yol tolalari, tumshuqli bosh (ko'zlar, burun teshiklari, konus quloqlar, peshona dog'i), yugan/jilov/suvliq, dum dastasi (`TailMesh`), ikki bo'g'inli oyoqlar (`Legs` yelka/son + `LowerLegs` bilak, to'piq, tuyoq). Tasodifiy belgilar: qora oyoq-tumshuq, oq paypoqlar, peshona dog'i. Egar: o'rindiq, oldingi/orqa qosh, qanotlar, ayil, uzangi tasmalari va temir uzangilar, gilam.
- Animatsiya: pastki bo'g'in oyoq oldinga uchganda bukiladi, chopishda bo'yin cho'ziladi, turganda o'tlaydi (bosh pastga), dum va quloqlar tebranadi, tana yengil chayqaladi.
- Sinov skrinshotlari: `28_bursa`, `29_horse`; minish sinovi endi janubga chopadi.

## Nikeya shahri va Askaniya ko'li

- `BuildNikeya()` (ErtWorldBuilder): g'arb-markazda (E=-300, N=-60, R=110 m) Vizantiya shahri: g'isht qatorli tosh qo'sh devor (tashqi past, ichki baland), 28 dumaloq burj, 4 Rim arkli darvoza (shimoliy darvoza ko'lga); Ayo Sofiya bazilikasi (nef, apsida, gumbaz, xoch, ustunlar); marmar ustunli agora va favvora; Rim teatri (yarim aylana zinalar, qisman xaroba, sahna devori); to'g'ri burchakli ko'chalar (cardo/decumanus), g'isht-tosh sopol tomli uylar, sarvlar; ko'l darvozasi oldida iskala.
- Askaniya ko'li: yangi ko'l (E=-300, N=95, R=65 m, suv sathi 4.5 m): `IsWater`, relyef chuqurligi, suv diski, qumloq qirg'oq, 2 qayiq.
- Xarita: `Nikeya`, `Askaniya ko'li` belgilari; NPC joyi `nikeya` (savdogar, tekfur); `Nikeya` epizodlari sharqiy darvoza oldidan boshlanadi. Yo'l (-150,-150) dan sharqiy darvozaga.
- Sinov skrinshoti: `30_nikeya`.

## Karacahisar qal'asi

- `BuildKaracahisar()` (ErtWorldBuilder): shimolda (E=-60, N=760) 45 m qora qoya (relyefda, tepasi 30 m tekis plato, tik qiyalik, qora tosh rangi); ustida Vizantiya qal'asi: 11 burchakli notekis g'isht qatorli devor, kungura, kvadrat burjlar, janubiy darvoza (temir panjara), 18 m donjon (tirqish derazalar, bayroq), cherkov (gumbaz, xoch), kazarma, sardoba, ombor, gulxan; janubdan uch bo'lakli ilon izi tosh yo'l; etakda 6 kulbali qishloq, quduq, tekfur bayrog'i. Yo'l Konya g'arbiy darvozasidan qal'a etagiga.
- Xarita: `Karacahisar` belgisi; NPC joyi `karacahisar` (tekfur - EP041 dialogi, qishloq ayoli); `Karacahisar` epizodlari qoya etagidan (janub) boshlanadi.
- Sinov skrinshoti: `31_karacahisar`.

## So'g'ut qishlog'i

- `BuildSogut()` (ErtWorldBuilder): Karacahisar janubi-g'arbida (E=-220, N=600, R=70 m) yashil vodiy qishlog'i: egri ariq (ko'k tasma, loy qirg'oq), ikki yog'och ko'prik, suv tegirmoni (charx); tosh poydevorli masjid (yog'och minora, qo'rg'oshin gumbaz), sakkiz qirrali yashil gumbazli Ertug'rul turbasi; bozor maydoni (soyabonli rastalar, quduq, gulxan); yog'och/oq suvoqli uylar (AddHouse) va bog'cha panjaralari; qo'ylar qo'rasi va pichan g'aramlari; mevazor qatorlari, bug'doy dalasi; osilgan shoxli tollar; kuzatuv minorasi va Qayi tug'i. Yo'l Karacahisar yo'lidan qishloqqa.
- Xarita: `So'g'ut` belgisi; NPC joyi `sogut` (tegirmonchi, bozorchi, imom); `So'g'ut`/`Domaniç` epizodlari sharqiy kirishdan boshlanadi.
- Sinov skrinshoti: `32_sogut`.

## Domaniç yaylovi

- `BuildDomanic()` (ErtWorldBuilder): Bursa va Sivas orasida, oba yo'li yonida (E=-440, N=270, R=90 m) 18 m ko'tarilgan yassi baland o'tloq: yam-yashil rang va gul dog'lari (shovqinli rang), 260 ta rangli yovvoyi gul; 8 kichik yozgi o'tov halqasi, gulxan, tug', kigiz, o'tin; tosh cho'pon kulbasi va qurut/pishloq tokchalari; qo'y qo'rasi (40 qo'y, 2 it, cho'pon); 9 statik yilqi (`AddHorse`); tosh o'ralgan buloq havzasi va jilg'a; chetda qarag'ay to'plari; kuzatuv posti; yo'l boshida tosh belgi. Yaylov ichida daraxt/qoya qo'yilmaydi.
- Xarita: `Domaniç yaylovi` (yashil) belgisi; NPC joyi `domanic` (cho'pon, kampir); `Domaniç` epizodlari yo'l boshidan boshlanadi.
- Sinov skrinshoti: `33_domanic`.

## Protsedural PBR materiallar va Bagras qal'asi

- `tools/unreal/Scripts/ert_make_pbr.py` (commandlet: `-run=pythonscript -script=...`): `M_ErtVertexColor` qayta yaratiladi. Ikki Custom HLSL tugun (rang+balandlik, dunyo-fazo normal): vertex rangi alfa kanali naqshni tanlaydi (`ErtCol::StyleGround/Stone/Wood/Roof/Brick/Plain`, `ErtCol::Sty(C, Style)`), naqsh dunyo koordinatalarida tri-planar (UV kerak emas): tuproq/o't shovqini, tosh bloklar (chok, har blok rangi), yog'och taxtalar (don), cherepitsa (qator, egri), g'isht. Balandlikdan normal (relyef) va roughness. Tangent-space normal o'chirilgan.
- Relyef vertexlari `StyleGround`. Boshqa barcha eski qurilmalar `StylePlain` (yengil don) - keyin bosqichma-bosqich uslub beriladi.
- Renderer: ekran-fazo GI (SSGI, sifat 3), SSR sifat 3, AO, kontakt soyalar, 4 kaskad 4096 soya. Lumen/RT yo'q (Intel GPU).
- `BuildFortress()` (Bagras) qayta yozildi: glasisli dumaloq burjlar (qator choklari, mashikuli konsollari, parapet, kungura tishlari, o'q tirqishlari, konus cherepitsa tom), devorlar (poydevor, mashikuli tokchasi, parapet, tishlar, tirqishlar, ichki yog'och yo'lak va ustunlar), 7 burj, darvozaxona (ikki burj, ark, temir panjara, tushirilgan ko'tarma ko'prik, zanjirlar, xandaq), donjon (burchak ustunlari, 3 qavat peshtoqli derazalar, mashikuli, shiypon tom, bayroq, eshik, zina), cherkov (apsida, xoch), kazarma, otxona (ot), temirchi (mo'ri, sandon, olov), quduq, bochkalar, yashiklar, qurol ustuni, mash'alalar.
- Skrinshotlarda birinchi kadrlar shader kompilyatsiyasi tugaguncha kulrang bo'lishi mumkin (DDC keshlanadi).

## Barcha shaharlarga material uslublari

- `ErtWorldBuilder.cpp` dagi barcha rang e'lonlari uslub bilan belgilandi (`ErtCol::Sty`): tosh nomlari (Stone, Lime, Basalt, HStone, KStone, SStone, NStone, BStone, Grey, StoneG...) -> `StyleStone`; yog'och (Wood, DarkWood, *Wood, Timber) -> `StyleWood`; Tile/Shingle -> `StyleRoof`; Brick/BrickD -> `StyleBrick`. `Vary` alfani saqlaydi, shuning uchun barcha ishlatishlar avtomatik uslub oladi. Qorong'i eshik tuslari (`* 0.3f` kabi) `StylePlain` da qoldirildi (alfa siljimasligi uchun).
- Shunday qilib oba devori, Bagras shahri, Mo'g'ul lageri, cho'l binolari, Damashq, Halab, Konya, Qayseri, Sivas, Erzurum, Bursa, Nikeya, Karacahisar, So'g'ut, Domaniç hammasi naqshli PBR ko'rinishga o'tdi.
