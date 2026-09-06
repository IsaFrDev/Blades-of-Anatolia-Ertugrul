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

## Personaj va ot materiallari

- Yangi uslublar (`ErtCol::StyleCloth` 0.10, `StyleLeather` 0.32, `StyleMetal` 0.50, `StyleFur` 0.70, `StyleSkin` 0.90): material HLSL 11 diapazonga bo'lindi (0.05 qadam). Mato: mayda to'qima; charm: ikki shovqinli don; metall: halqa (zanjir) naqshi, metallik 0.7, roughness 0.45+; jun: tolali chiziqlar; badan: nozik teri doni. Roughness va metallik alohida Custom tugunlarda (`ErtRough`, `ErtMetal`).
- `UErtHeroBody`: Build/SetShield/SetSwordTier ichida a'zo ranglari uslubli mahalliy nusxalar (KaftanS, LeatherS, SkinS, SteelS, FurS, BeardS, TrimS, TrousersS) bilan ishlatiladi; qalqon yog'och uslubi. Dushman va NPC ham shu tanadan foydalanadi.
- `AErtHorse`: yung (CoatF, Dark, Mane, WhiteM) jun uslubi, egar/tasmalar charm, gilam mato, uzangi/suvliq metall; tuya ham. Kiyik (`BuildDeer`) jun.

## Tabiat materiallari

- Yangi uslublar: `StyleFelt` 0.12 (kigiz, o'tovlar), `StyleRock` 0.27 (tabiiy qoya: uch qatlam shovqin + yoriq chiziqlari, kuchli relyef), `StyleBark` 0.64 (po'stloq: vertikal chiziqlar), `StyleLeaf` 0.97 (barg: ikki qatlam shovqin). Diapazonlar: tosh [0.15,0.24), qoya [0.24,0.30), mato [0.05,0.108), kigiz [0.108,0.15), tom [0.56,0.62), po'stloq [0.62,0.66), barg [0.96,0.99), oddiy >= 0.99.
- `AddTree`/`AddPalm`/shahar daraxtlari (Plane, Cyp, Poplar, Pine, Fruit, Willow, tut) barg+po'stloq; `BuildRocks` qoya uslubi (avval tosh blok bo'lib qolgan edi); `AddYurt` devor/tom kigiz (Felt, Cream, FeltDark, Mo'g'ul o'tovi). Alfani siljituvchi tuslar (`Leaf * 1.1f`, `Trunk * 0.8f`, `FeltDark * 0.85f`, `Basalt * 1.4f`) `Sty` bilan tuzatildi.

## Suv materiali

- `tools/unreal/Scripts/ert_make_water.py` (commandlet): `M_ErtWater` qayta yaratiladi. Custom HLSL `ErtWaves` (dunyo koordinatalari + Time): uch sinus to'lqin va ikki qatlam shovqin balandligidan dunyo-fazo normal. Rang: chuqur/sayoz `DepthFade`(350) bo'yicha, Fresnel (daraja 5) bilan osmon tusi; qirg'oq ko'pigi `1 - DepthFade(90)`; shaffoflik chuqurlikka bog'liq (0.35 -> 0.85) + Fresnel; roughness 0.06, specular 1, sindirish 1.2, ekran-fazo aks, ikki tomonlama, yuzaki per-pixel yoritish.
- Eslatma: translucent shader birinchi marta 3-4 daqiqa kompilyatsiya bo'ladi ("Preparing Shaders"), shundan keyin DDC keshda. Sinovdan oldin o'yinni bir marta 4 daqiqa ishlatib kesh isitildi.

## Osmon va yorug'lik

- `tools/unreal/Scripts/ert_make_sky.py`: `M_ErtClouds` (unlit translucent, Custom HLSL: 4 oktava shovqin, `Coverage`/`Density`/`Tint` parametrlari, kameradan 9-16 km da so'nadi, vaqt bilan oqadi) va `M_ErtStars` (unlit additive: yo'nalish bo'yicha hash yulduzlar, miltillash, ufqda so'nish, `Vis` parametri).
- `AErtWeather`: BeginPlay da 36 km bulut tekisligi (2 qatlam, 2.0 va 2.7 km) va 60 km yulduz sferasi (dinamik material nusxalari). `UpdateSky(Day, SunColor, Elev)`: bulut tusi (kunduzi oq, tong/shomda quyosh rangi, tunda ko'kimtir, bo'ronda qora), yulduz ko'rinishi (tunda, bulut qoplamiga bog'liq), tuman rangi (tun ko'k-qora, kunduz och ko'k, tong/shom zarhal, bo'ron kulrang). Ob-havo bulut qoplamini o'zgartiradi (rain/storm/snow/fog/dust/wind).
- `AErtGameMode::Tick`: quyosh rangi rampasi (`SunCol`) ajratildi, yomg'ir/bo'ronda quyosh 45% xiralashadi, har kadr `Weather->UpdateSky`.
- Sinov skriptida shom, tun va bo'ron skrinshotlari (`34_dusk`, `35_night`, `36_storm`).

## Tekis tomlar va ko'cha toshlari

- Yangi uslublar: `StyleMudRoof` 0.92 (tuproq tom: uch qatlam shovqin + yoriqlar) va `StyleCobble` 0.95 (tosh yotqizma: 28 sm jitterli dumaloq toshlar, chuqur choklar, kuchli relyef). Badan diapazoni [0.86,0.91) ga toraytirildi.
- `AddHouse`: tom (`Wall * 0.75f`) alfani jun diapazoniga tushirib swirl ko'rinar edi; endi tuproq tom uslubi va tom chekkasi qo'shildi; gumbaz oddiy uslubda. Bu Bagras shahri va So'g'ut uylariga ta'sir qiladi.
- `BuildCity` (Bagras shahri): markaziy maydon (42x42 m), 4 asosiy ko'cha, 40 ta radial tor ko'cha tosh yotqizma bilan qoplandi, maydon chetida tosh ustunchalar; Ochre devor tuslari tosh uslubida.

## Dushman va NPC kiyim detallari

- `UErtHeroBody` yangi bayroqlar: `bTurban` (uch qatlam o'ralgan salla, bezak), `bLamellar` (ko'krak va orqada 5x5 metall plastinka qatorlari, charm tasmalar), `bPauldrons` (yelka himoyasi, qatlamlar, to'g'nog'ich), `bCloak` (orqa plash, yelka to'g'nog'ichlari, `Cloak` rangi), `bQuiver` (sadoq, o'qlar, tasma), `bBackShield` (orqadagi yog'och qalqon, metall umbon), `bBoots` (baland charm etik, to'qa), `bMail` (zanjir ko'ylak, metall uslubi).
- Dushmanlar: piyoda - etik, orqa qalqon; serjant - lamellar, yelka, etik; arbaletchi - sadoq, etik; elita - lamellar, yelka, qizil plash, etik; boss - lamellar, yelka, qora plash, zanjir, etik; otliq - zanjir, plash, etik.
- NPC (erkaklar): ulamo/savdogar oq salla; oddiylar 50% rangli salla; beklar/hokimlar/tekfurlar plash, etik, oltin hoshiya (tekfur yelka himoyasi bilan); 20% plash, 40% etik.
- Sinov skriptida dushman yaqinidan kadr (`enemy`).
- Tuzatish: `AErtEnemy::BeginPlay` avval har doim piyoda sifatida tanani qurar edi, spawner `Init` keyin faqat statistikani o'zgartirardi (barcha dushmanlar bir xil ko'rinardi). Endi standart Init birinchi Tick da chaqiriladi, spawner Init undan oldin ishlaydi.

## Fab / Quixel Megascans assetlari

- Loyihada `Fab` plagini yoqildi (`Ertugrul.uproject`). Assetlarni yuklash uchun muharrirda Window > Fab ochib Epic hisobiga kiriladi (bu qismni faqat foydalanuvchi qila oladi), kerakli paketlar (masalan Megascans daraxtlar, qoyalar, o'tlar) "Add to project" qilinadi. Ular odatda `/Game/Fab` yoki `/Game/Megascans` ga tushadi.
- `ErtFab.h/.cpp` (`FErtFabLib`): Asset Registry orqali `/Game/Fab`, `/Game/Megascans`, `/Game/Quixel`, `/Game/MegascansLibrary`, `/Game/ErtAssets` skanerlanadi; statik meshlar nomiga qarab toifalanadi: pine/fir/spruce/cedar -> qarag'ay; tree/oak/beech/birch/maple/poplar/willow/olive -> daraxt; rock/boulder/cliff/stone -> qoya; bush/shrub -> buta; grass/fern/plant -> o't. `Content/Ertugrul/Data/fab_assets.json` manifestida ro'yxatlar qo'lda ham beriladi (`scan_paths` qo'shimcha yo'llar).
- `AErtWorldBuilder`: `BuildForest`/`BuildRocks` da mesh topilsa protsedural o'rniga `HierarchicalInstancedStaticMeshComponent` instanslari (mesh -> HISM, kolliziya bilan) qo'yiladi; masshtab mesh bounds dan kerakli balandlik/radiusga keltiriladi (`ScaleToHeight`, `ScaleToRadius`). Topilmasa protsedural daraxt/qoyalar.
- Build.cs: `AssetRegistry` moduli. Nanite loyihada o'chiq, Fab meshlari fallback mesh bilan ko'rinadi.
- Sinov: manifestga dvigatelning Cone/Sphere shakllari qo'yib, quvur (pipeline) tekshirildi, keyin manifest bo'shatildi.

## Jang effektlari, o't-o'lan folyaji, yer auto-materiali

- `ErtFx.h/.cpp` (`AErtBurst`): protsedural zarralar (har kadr qayta quriladigan kesishgan kvadratlar, gravitatsiya, yerga yopishish, so'nish): `Blood` (qon, 14 zarra x kuch), `Sparks` (uchqun, tez, sariq), `Dust` (chang); `SwordArc` - qilich izi (yoy shaklidagi shaffof mesh, T bo'yicha kengayadi va so'nadi; 0 gorizontal, 1 pastdan yuqoriga, 2 og'ir vertikal).
- Ilgaklar: `DoAttack` - zarba boshida qilich izi, tegsa qon (ijroda 2.2x), to'silsa uchqun; `ReceiveHit` - blokda uchqun, zarar olganda qon; `AErtEnemy::Die` - yerga yiqilganda chang.
- `UErtHeroBody`: `TriggerHurt(SideSign)` yo'nalishli reaksiya (tana buriladi va egiladi, qo'llar siltanadi); `SetDead(HalfH, Variant)` uch o'lim pozasi: orqaga yiqilish, yuztuban, tiz cho'kib yonboshga (tasodifiy).
- `BuildGrass` (ErtWorldBuilder, 26000 tup, 6x6 chunk): har tup 3 kesishgan tola (alfa: tag 0, uch 1), yashil o'tloqlarda zich, janubda siyrak, oba/So'g'ut/yaylov/Bursa atrofida zichroq, yo'l va suvdan tashqari. `M_ErtGrass` (`ert_make_grass.py`): ikki tomonlama folyaj shading, WPO shamol (uch sinus, sekin o'zgaruvchi kuch, alfa bilan), rang tagi qoramtir.
- Yer materiali (st 0) auto-qatlamlar: qiyalikda (N.z < 0.9) qoya naqshi va rangi, 75 m dan yuqorida qor, tekis pastlikda (N.z > 0.992, Z < 24 m) shovqinli ko'lmaklar (qorong'i, roughness 0.12), o't tolalari chiziqlari.

## Ufq tumani va quyosh nurlari; Blender yo'riqnomasi

- `AErtGameMode::BeginPlay`: quyoshda ekran-fazo light shaft (bloom 0.35, threshold 0.6, okklyuziya 0.08, 1.2 km) - bulut va tog' orqasidan nur tolalari, Intel GPU uchun arzon.
- `AErtWeather::BeginPlay`: tuman ikki qatlamli (height falloff 0.12, start 25 m, max opacity 0.92, ikkinchi qatlam zichlik 0.0006, falloff 0.02, offset -400 m) - ufqdagi keskin oq chiziq yumshaydi; yo'naltirilgan inscattering (daraja 12, 80 m dan) kun vaqtiga qarab quyosh rangida (`UpdateSky`).
- `docs/guides/blender_ue_guide.html`: Blender 4.x -> UE 5.8 yo'riqnomasi (birliklar, model, vertex rang + alfa uslublari, Rigify, FBX eksport katakchalari, UE import, `fab_assets.json`, xatolar jadvali). Artifact sifatida ham chop etilgan.
- `BuildWater`: dunyo chekkasidan 30 km gacha "uzoq dengiz" halqasi (suv materiali, 48 segment) - ufqda bo'shliq o'rniga dengiz; shom tumani rangi xiralashtirildi.

## Qon dog'lari, jarohat izlari, Poly Haven assetlari

- `AErtSplats` (ErtFx): yerdagi doimiy qon dog'lari - bitta aktor, 400 tagacha (FIFO), har dog' 9 nurli notekis ko'pburchak + 4 tomchi; qon zarrasi yerga tekkanda trace bilan yer nuqtasiga qo'yiladi.
- `UErtHeroBody::AddWound(Side, Strength)`: Torso ga biriktirilgan `Wounds` qismi, 12 tagacha qora-qizil yassi dog' (old/orqa/yon yuzada) va pastga oqqan iz; dushman `ApplyHit` va personaj `ReceiveHit` da chaqiriladi.
- Ko'lmaklar yana kamaytirildi va ochroq (rasmda qora dog'lar bo'lib ko'rinardi).
- Poly Haven (CC0, hisobsiz): `art/polyhaven/<id>/` ga glTF 1k yuklab olindi (User-Agent kerak): boulder_01, rock_07, rock_09, rock_moss_set_01, namaqualand_boulder_02, shrub_01, shrub_03, dead_tree_trunk, tree_stump_01, stone_fire_pit, wooden_barrels_01, wooden_crate_01, wooden_bucket_01, wooden_lantern_01, wooden_table_02, wooden_stool_02, kite_shield. Daraxtlar (pine_tree_01 950 MB, fir_tree_01 465 MB, island_tree 45-64 MB) juda og'ir - protsedural qoldi. Sketchfab/CGTrader hisob va litsenziya talab qiladi, avtomatik yuklab bo'lmaydi.
- `tools/unreal/Scripts/ert_import_ph.py` (commandlet): glTF ni Interchange orqali `/Game/ErtAssets/PH/<id>` ga import qiladi, statik meshni `SM_PH_<id>` deb qayta nomlaydi; `FErtFabLib` `/Game/ErtAssets` ni skanerlab qoya/buta sifatida ishlatadi.
- Toifalash: stump/trunk/log -> `Stumps` (o'rmonda 4% joyda to'nka/yotgan tana), barrel/crate/bucket/lantern/table/stool/fire_pit/shield -> `Props`; `BuildProps`: 15 ta joy (oba, So'g'ut, yaylov, Bursa, Shahar, Damashq, Halab, Konya, Qayseri, Sivas, Erzurum, Nikeya, lager, karvonsaroy, Karacahisar etagi) atrofida trace bilan bo'sh joyga buyumlar; o't orasida har 40 tupdan biri buta (`Bushes`). Yuklab olingan manbalar `art/polyhaven/` (gitignore), import qilingan assetlar `Content/ErtAssets/PH`.

## Skeletli personaj (character.json)

- `Content/Ertugrul/Data/character.json`: profillar `hero`, `enemy`, `npc`. Profilda `mesh` (USkeletalMesh yo'li) bo'lsa `UErtHeroBody::Build` protsedural tana o'rniga `USkeletalMeshComponent` yaratadi (yaw/z/scale bilan kapsulaga joylanadi), `AnimationSingleNode` rejimida holatga qarab animatsiya almashtiradi: idle/walk/run (play rate tezlikka bog'liq: `walk_ref`, `run_ref`), fall/jump, crouch/crouchwalk, block, ride, swim, attack (tasodifiy ro'yxatdan) / heavy / kick, hurt, death (oxirgi kadrda qoladi). Bir martalik animatsiya (`OneShotT`) tugaguncha lokomotsiya almashtirilmaydi. Qilich `sword.socket` suyagiga protsedural mesh sifatida biriktiriladi.
- Barcha chaqiruvchilar (personaj, dushman, NPC) o'zgarmagan: `IsBuilt()` skelet bo'lsa ham true; `SetShield`, `AddWound`, kiyim bayroqlari skelet rejimida e'tiborga olinmaydi.
- Sinov uchun hozir `hero` = SKM_Manny_Simple, `enemy` = SKM_Quinn_Simple (UE shablon Mannequin + MM_ animatsiyalari, loyihada bor). `npc` bo'sh - protsedural. Blender FBX tayyor bo'lganda `/Game/ErtAssets/SK_...` yo'lini `hero` ga yozish kifoya. Protseduralga qaytish: `mesh` ni bo'sh qoldiring.
- Skelet qilichi: Mannequin suyaklarida X o'qi barmoqlar tomon, shuning uchun tig' +X, dasta -X bo'ylab quriladi (`SkelBuildSword`). Ma'lum kamchilik: Mannequin'da o'tirish/minish animatsiyasi yo'q, otda `idle` (tik) ko'rsatiladi; Blender'dan `ride` animatsiyasi qo'shilsa avtomatik ishlatiladi.

## Haqiqiy yer teksturalari, ichki jihozlar, skeletli ot

- (j) Poly Haven CC0 teksturalari (`art/polyhaven_tex/<rol>/T_<rol>_D/N/R`, 2k jpg): grass=aerial_grass_rock, dirt=brown_mud_leaves_01, rock=rock_face_03, sand=aerial_sand, snow=snow_02, cobble=grassy_cobblestone. `ert_import_tex.py` ularni `/Game/ErtAssets/Tex` ga import qiladi (normal: TC_NORMALMAP, sRGB o'chiq, yashil kanal teskari - nor_gl). `ert_make_pbr.py`: Custom tugunlarga TextureObject kirishlari (TGrassD/N ...), yer (st 0) rangi va normali teksturadan: o't/tuproq relyef vertex rangi (yashil/quruq nisbati) bo'yicha, qum yorug' quruq joyda (cho'l), qiyalikda qoya, balandda qor; ikki masshtab (3 m va 11 m) aralashmasi takrorlanishni kamaytiradi; vertex rang bilan 35% tuslanadi. Tekstura yo'q bo'lsa dvigatel standart teksturasi.
- (k) `AErtWorldBuilder::Interiors`: `AddYurt` (R >= 2.3 m) va `AddHouse` ichki joyni ro'yxatga oladi; `BuildProps` har xonaga 1-3 buyum (0.45-0.8 m) qo'yadi (trace tekshiruvisiz, devordan uzoqroq).
- (l) `AErtHorse::TryBuildSkeletal`: `character.json` da `horse`/`camel` profilida mesh bo'lsa skeletli mesh, animatsiyalar idle/walk/trot/gallop/jump/death tezlikka qarab (`walk_ref`, `trot_ref`, `gallop_ref`), egar `saddle` [x,y,z] yoki `saddle_socket`. Hozir bo'sh - protsedural ot; Blender'dan ot FBX bo'lsa yo'llarni yozing.
- Tuzatishlar: Custom tugun ichida `Texture2DSample` qora natija berdi - teksturalar endi oddiy `TextureSample` tugunlari (dunyo XY koordinatali UV, 4 masshtab: 3 m, 11 m, 2 m qum, 4.5 m qoya; Normal sampler) orqali float3 sifatida Custom tugunlarga beriladi. Mannequin `MM_HitReact_*` animatsiyalari additiv - SingleNode da crash berardi; yuklashda additivlar o'tkazib yuboriladi (`IsValidAdditive`), hero/enemy da `hurt` yo'q.
- Buyumlar masshtabi endi maksimal o'lcham bo'yicha (`ScaleToRadius`, tashqarida 0.45-0.75 m, ichkarida 0.3-0.5 m) - yupqa taxta qismlari ulkan plita bo'lib qolmaydi; ko'p qismli importlarning qism-meshlari (`_1`, `_2`) buyum sifatida olinmaydi. O't tusi vertex rangdan 60% olinadi (sariqlik kamayadi).

## Yo'l toshi, avtomatik o'yinchi, epizod sinovi

- (m) Relyef vertex alfasi yo'lda 0.03 (ground diapazoni ichida) - shader tosh yotqizma teksturasini (T_cobble, 2.5 m plitka, 55-90% aralash) va normalini qo'yadi; o't teksturasi yashil tus (0.78, 1.02, 0.62) bilan.
- (o) `-ErtAutoPlay` (ErtCharacter::UpdateAutoPlay): kat-sahnani o'tkazadi (1.2 s), dialogda birinchi variantni tanlaydi, menyuda Confirm, Cleared da davom, maqsad: eng yaqin tirik dushman (3.5 km) -> bajarilmagan maqsad nuqtasi (Route: joriy nuqta, Hunt: kiyik) -> marker; 230 sm da hujum (15% dodge), 160 sm dan uzoqda yuradi (900 sm dan uzoqda chopadi), yaqin turganda E (NPC/buyum); sog'liq 35% dan pastda dori; 3 s tiqilsa sakraydi, 7 s da maqsad yoniga teleport; bosqich 150 s da bajarilmasa dushmanlar yo'q qilinadi; Inactive 6 s -> chiqish; 480 s umumiy limit. Log: `[AutoPlay]`.
- `tools/unreal/Scripts/run_episodes.ps1 -Episodes EP001,EP002 ...`: har epizodni `-ErtEpisode -ErtUnlockAll -ErtCutscene -ErtAutoPlay` bilan ishga tushiradi, `[Missiya]`/`[AutoPlay]`/Fatal/Warning satrlarini `episode_report.txt` ga yig'adi.
- Tuzatish: skelet animatsiyalari va Fab meshlari UPROPERTY bo'lmagan konteynerlarda edi - GC ~90 s da o'chirib crash berardi; endi `SkelAnimRefs`/`SkelMeshRef` UPROPERTY, Fab meshlari `AddToRoot`.

## Ochiq dunyo tizimlari: spline devor, dekallar, olov, hajmli tuman, 3D xarita, GPS, urush AI, grafika presetlari

- **Grid Snapping + Spline Mesh devorlar** (`ErtProcMesh.h: ErtSnap`, `AErtWorldBuilder::AddWallSpline`): nuqtalar 1 m to'rga yopishtiriladi, chiziq bo'ylab 4 m li modullar takrorlanib egiladi, burchaklarda minora, tepada tishlar. `BuildSplineWalls`: So'g'ut uzumzor devori, Domaniç qo'rasi, Bagras qo'rg'on devori, Askaniya qirg'oq devori, Konya karvon yo'li, oba chegarasi (204 modul).
- **Dekallar** (`M_ErtDecal`, `tools/unreal/Scripts/ert_make_decal.py`; `BuildDecals`): `UDecalComponent` bilan mog'or (Kind 0), yoriq (1), dog' (2) - spline devorlar va uy/o'tov tashqi devorlarida (257 ta).
- **Olov effektlari** (`ErtFire.h/.cpp: AErtFireFx`, Niagara o'rnida protsedural): olov tili billboardlari, tutun, uchqunlar + miltillovchi `PointLight` (uch chastotali sinus + shovqin). Har gulxan/mash'ala uchun `AddFire` joyidan spawn (34 ta), kameradan 150 m ichida jonlanadi.
- **Hajmli tuman**: `ExponentialHeightFog` da Volumetric Fog yoqildi (`ErtWeather.cpp`). `LocalFogVolume` sinovda butun dunyoni qopladi (radius birligi noaniq) - olib tashlandi.
- **Qirg'oq/devor o'simliklari** (`BuildShoreFoliage`): ko'l/voha/daryo bo'yida qamish tuplari va mayda toshlar, spline devor tagida maysa.
- **3D xarita** (`ErtMap3D.h/.cpp: AErtMap3D`): dunyoning 1/50 miniatyurasi (relyef 2.5x bo'rttirilgan, suv, shahar gumbazlari, marker konuslari, GPS lentasi) osmonda 2.6 km balandda; ortografik `SceneCapture2D` -> 1024 render-tekstura; HUD `DrawMap` da ko'rsatadi, `Project()` bilan shahar nomlari/markerlar ustiga chiziladi. Chap/O'ng - aylantirish, Yuqori/Past - masshtab.
- **GPS** (`ErtNav.h/.cpp: AErtGps`): 10 m hujayrali narx to'ri (qiyalik/suv narxi), A* yo'l, silliqlangan; yerda oltin lenta + maqsad ustuni (`M_ErtDust`), minimapda chiziq, marker ostida "GPS N m". Maqsad = birinchi missiya markeri. `-ErtNoGps` o'chiradi.
- **Urush AI** (`AErtEnemy::Team`, `PickTarget`): 0 dushman, 1 ittifoqchi Qayi alpi. Har 0.5 s eng yaqin raqib (o'yinchi yoki boshqa jamoa) tanlanadi; zarba `ApplyHit`/`ReceiveHit`. Ittifoqchilar raqib bo'lmasa o'yinchiga ergashadi. `AErtMissionDirector::SpawnAllies`: 3+ dushmanli to'lqinda 3-8 alp (SIEGE/DEFENSE da +3). O'yinchi zarbasi/o'qi/qulfi ittifoqchiga tegmaydi.
- **Grafika presetlari** (`AErtGameMode::ApplyGfxPreset`, sozlamalar 9-qator, `-ErtGfx=ultra|console|low`):

| Sozlama | PC Ultra | PS5 / Xbox Series X (60 FPS) | Xbox Series S (60 FPS) |
|---|---|---|---|
| Global Illumination | Lumen (Hardware RT) | Lumen (Software) | SSGI (Screen Space) |
| Soyalar | Virtual Shadow Maps (High) | Virtual Shadow Maps (Medium) | Cascaded Shadows |
| Aks | Lumen (Ray Traced) | Lumen (Software) | SSR |
| O'lcham | 100% + TSR (DLSS plagin bilan almashadi) | Dinamik 50-70% TSR | Dinamik 50-60% TSR |

- **Kengash tuzatishi**: kengash NPClari va nuqtasi obadan 400 m dan uzoq epizodlarda (Bagras, Damashq...) epizod boshlanish nuqtasida (`CouncilBase`); bo'sh dialogli kengash nuqtada 2 s turish bilan o'tadi.
- **Avtomatik sinovchi**: bosqich chegarasi 60 s, yaqinlashmaslik (8 s) bo'yicha teleport, 3 teleportdan keyin bosqich yopiladi ("XATO nomzodi"), uzoqdagi dushmanga ham boradi, `-ErtShot` bilan har 25 s skrinshot.

## Realistik butalar
- `M_ErtLeaf` (`tools/unreal/Scripts/ert_make_leaf.py`): Masked, ikki tomonlama folyaj (subsurface), UV bo'yicha protsedural barg to'plami silueti (tishli chet + teshiklar, dunyo koordinatasidan urug'), tomirlar, shamol WPO (alfa = kartochka balandligi).
- `AErtWorldBuilder::BuildBushes` (`BushCount` 2600): har buta 16-26 barg kartochkasi (ellipsoid ichida tashqariga qaragan, burilgan) + 3-5 shox silindri. Turlari: o'tloq yashil, o'rmon to'q yashil, cho'l/tog' quruq, gullagan (oba/qishloq). Zichlik: daryo/ko'l bo'yi, o'rmon chekkasi, oba/qishloq atrofi. `FErtMeshData::AddQuadUV` - to'g'ri UV li to'rtburchak.
- Spline devor tutashuvlarida tosh ustunlar (burchak bo'shliqlari yopildi).

## AssetHub (Tripo) rasm->3D modellari
- GLB fayllar `Content/ErtAssets/Gen/src/SM_<Tur>_<Nom>.glb` ga qo'yiladi; `tools/unreal/Scripts/ue_import_gen.py` (MCP `execute_python_code` yoki commandlet) import qiladi, `ue_fix_gen_mats.py` to'g'ri PBR material yaratadi (Color sRGB -> BaseColor, NormalGL -> Normal (yashil kanal teskari), ORM: R=AO, G=Roughness, metallik 0), `ue_gen_usage.py` "Used with Instanced Static Meshes" + Nanite yoqadi (busiz o'yinda kulrang standart material chiqadi).
- Skaner (`ErtFab.cpp`) nom bo'yicha yangi toifalar: yurt/tent/house/gate/well/cart/stall. `AErtWorldBuilder::FabPlace` meshni reja nuqtasiga poydevori yerga tegadigan qilib qo'yadi (balandlik yoki radius bo'yicha masshtab).
- `AddYurt`/`AddHouse` mesh bo'lsa protsedural o'rniga uni ishlatadi (qorong'i kigiz -> chodir meshi); `BuildLandmarks`: darvozalar (Bagras, shahar, Karacahisar), quduqlar (8 joy), aravalar (7), rastalar (12).
- MCP orqali Python: `python tools/unreal/Scripts/mcp_py.py <script.py>` (muharrir ochiq bo'lsa; 30 s dan uzun ishlar timeout beradi lekin bajariladi).

## Realistik relyef (tog'lar)
- `HeightAt`: tizmali ko'p-fraktal (ridged) qirralar tog' massivi va shimoliy tizmada (R1..R3, 45+18+4 m), qatlamli qoya zinalari (`sin(H*0.3)`), etak soylari; Uludog'/Erciyes tizmali detal. Qal'a tekisligi va shahar tekisliklari saqlanadi (tizma ulardan oldin qo'shiladi).
- Relyef to'ri `CellM` 10 -> 5 m (1.09 mln uchburchak, 9.4 s qurilish).
- `BuildRocks`: 700 tagacha qoya; tik yonbag'irlarda (Nm.z < 0.72, H > 30) 3-7 m li "cliff" meshlari qiyalikka moslab (35%), chuqur ko'milgan; Nm.z < 0.45 va shahar/qal'a atrofi (R+50 m) chiqarib tashlanadi (muallaq qoyalar xatosi tuzatildi).
- Shader: qor chizig'i 75 -> 150 m, qoya qatlami kulrang (balandda to'qroq); Landscape eksportidagi qor qatlami ham 150 m.

## AC/GTA uslubidagi xarita
- Boshqaruv (M): sichqoncha sudrash - surish, g'ildirak - masshtab (500..5200), o'ng tugma - yo'l nuqtasi (yana bosilsa o'chadi), WASD surish, Q/E aylantirish, Z/X egish, R - o'yinchiga markazlash, Delete - yo'l nuqtasini o'chirish. Kursor xarita ochilganda ko'rinadi (`FInputModeGameAndUI`).
- Yo'l nuqtasi (`AErtGameMode::Waypoint`): GPS lentasi va minimap/dunyo markeri (ko'k) shu nuqtaga; 4 m yetganda o'chadi. Missiya markeri ikkinchi o'rinda.
- Kashf qilingan hududlar (`Visited`, 40x40 = 50 m hujayra, o'yinchi atrofi 2 hujayra): kashf qilinmagan joylar xaritada qorong'i, joy nomlari "?" bo'ladi; `ert_save.json` da `visited` sifatida saqlanadi.
- Joy belgilari (Canvas chiziqlari): shahar (gumbaz+minora), qal'a (tishli), qishloq (uy), lager (chodir), ko'l (to'lqin), tog'; viloyat nomlari (Bitiniya, Rum Sultonligi, Shom, Mo'g'ul yerlari, Arman qirolligi); kursor ostidagi koordinata va balandlik; legenda.
- `AErtMap3D`: `Center` surish, `PanPixels`, `Tilt`, `Unproject` (ekran -> dunyo, relyef sathi), pauzada ham tick.
- 48/48 epizod avtomatik sinovdan o'tdi (docs/episode_report_full.txt).
- Diagnostika: `-ErtCam=E,N,Z,pitch,yaw` (reja koordinatalari, m) - 6 s dan keyin shu nuqtaga teleport, 8 s da skrinshot (`D:/temp/claude/camshot/01_cam.png`), 9 s da chiqish. Skrinshot ssenariysida ketma-ket teleport+kadr past FPS da aralashishi mumkin (16_damascus aslida Halab bo'lib chiqqan) - shubhali kadrni shu opsiya bilan tekshiring.

## Poly Haven daraxtlar, butalar, o't (CC0, 1K)
- `tools/unreal/Scripts/ph_download.py <id...>`: API orqali glTF + 1K teksturalar (`art/polyhaven/<id>/`, gitignored). Yuklanganlar: island_tree_01/02, fir_sapling_medium (3 variant), shrub_02/04, grass_medium_01/02, grass_bermuda_01 (jami ~200 MB).
- `ert_import_ph_new.py` (`new_ids.json` bo'yicha) import; `ue_fix_gltf_parent.py`: Interchange MI lari dvijokning `M_GLTF` (ISM bayrog'i yo'q -> o'yinda kulrang) o'rniga `/Game/ErtAssets/M_GLTF_Ert` ga qayta bog'lanadi; `ue_fix_ph_foliage.py`: barg/o't MI lari (Substrate translucent, o'yinda ko'rinmaydi) -> `M_ErtFoliageTex` (Masked, ikki tomonlama folyaj, BaseColor.A niqob) instanslari `*_ert`.
- Nanite folyaj meshlarida O'CHIRILGAN (`ue_leaf_test.py`): Nanite soddalashtirish barg geometriyasini yo'qotadi (5.8 da `shape_preservation` sozlamasi bilan sinash mumkin).
- Skaner: `tree` -> Trees (island_tree), `fir` -> Pines, `shrub` -> Bushes, `grass` -> Grass; `BuildForest` Fab daraxtlarni 10/12 m balandlikka masshtablaydi, `BuildGrass` har 5-tupda Poly Haven o't meshi.
- O't rangi: relyef shaderida to'q yashil tint (0.62, 1.06, 0.48), quruq aralashma kam; tola ranglari to'yingan yashil.

## Raqobatchi tahlili asosidagi yangiliklar (docs/COMPETITOR_ULUKAYIN.md)
- **Uch jangchi** (F1/F2/F3): Ertug'rul (qilich, 1.0), Turg'ut Alp (bolta: zarar 1.6x, tezlik 0.92, +0.45 s gangitish, uloqtirish 1.6x, bolta modeli), Meryem (kamon 1.45x, qilich 0.7x, tezlik 1.12, cho'kkanda dushman ko'rish masofasi 60%). `AErtCharacter::SetWarrior`, `Warrior*` ko'paytirgichlari.
- **Alp mahorat shkalasi** (`AlpBar`, zarba +7, parry +20; HUD): F - Bo'ron qilichi (360°, 1.9x, gangitish), Yer zarbasi (420 sm radius, uloqtirish, 1.8 s gangitish, hit-stop), Uch o'q (o'q sarflanmaydi).
- **Rangli hujum ko'rsatkichi**: dushman zarbasi oldidan sariq halqa PARRY (yengil), qizil DODGE (og'ir, `IsHeavyPending`).
- **Ot hushtagi** (Z): 300 m ichidagi bo'sh ot yo'rtib/chopib keladi (`AErtHorse::Summon`).
- **Tush bosqichi** ("Tush: Ulukayin"): RITUAL epizodlarida va har 4-epizodda (global indeks % 4 == 2) 2-bosqich; o'yinchi cho'l vohasiga o'tadi, tun + tuman + binafsha tuslash (`AErtGameMode::SetDream`), 3 belgi yig'iladi, so'ng avvalgi joyga qaytadi. Sinov: EP003 da o'tdi.

## Ot parvarishi, hunarmandchilik, tush jumboqlari, yuz animatsiyasi (2026-09-06)

**Ot parvarishi** (`ErtHorse.h`: `Care`, `Feed()`, `Groom()`, `CareSpeed()`):
- Ot yonida (3 m, minilmagan) **V** = tarash (+50% parvarish), **H** = go'sht bilan boqish (go'sht -1, ot sog'ligi to'liq, +35% parvarish).
- Parvarish 0..100%: yo'rtish/chopish tezligi +12% gacha, o'z-o'zidan tiklanish 3 -> 7 HP/s, chaqirilganda (Z) tezroq keladi. 15 daqiqada asta kamayadi.

**Hunarmandchilik** (Deli Demir, oba temirxonasi; `npc_deli_demir.json` "Temirxona" bo'limi, `AErtGameMode::RefreshCraftFlags/EndDialog`):
- Resurslar: **temir** dushman o'ljasidan (30%, boy 60%, boss 3), **teri** kiyik go'shti bilan (+1). Saqlanadi (`iron`, `leather`, `ironArmor`, `arrowTier`).
- Retseptlar: 12 po'lat o'q (1 temir + 1 teri, o'q zarari +8), temir zirh (3 + 2, zarar -20%), Damashq qilichi (4 temir), temir qoplamali qalqon (2 + 1). Variantlar faqat resurs yetganda ko'rinadi (`can_craft_*` bayroqlari). Inventar (I) da temir/teri va retseptlar ko'rsatiladi.

**Tush jumboqlari** (`dream_riddle_1..3.json`, `AErtMissionDirector` tush bosqichi tugaganda `StartDialogId`):
- Ulukayin 3 belgidan so'ng jumboq beradi (or-nomus / xabar / o'q dastasi). To'g'ri javob -> `dream_gift` bayrog'i, or +5, to'liq shifo; noto'g'ri -> or -2. Dialog tugaguncha bosqich o'tmaydi.

**Yuz animatsiyasi** (`UErtHeroBody::FaceAnimate`): skeletli personajda morph-target nomlari avtomatik qidiriladi (MouthOpen/JawOpen/CTRL_expressions_jawOpen/viseme_aa..., EyeBlink/eyeBlinkLeft/Right...). Dialogda kim gapirsa (NPC yoki "Ertugrul" so'zlovchi) o'sha og'zini qimirlatadi, ko'zlar 2-5 s da yumiladi. Jag' suyagi (jaw/FACIAL_C_Jaw) topilsa aniqlanadi, lekin suyak burilishi AnimBP/Control Rig talab qiladi (keyingi qadam).
- Fab Paragon personajlari (Kwang, Greystone, Sparrow...) uchun: Fab -> "Add to project" -> `character.json` da `hero.mesh` = `/Game/ParagonKwang/Characters/Heroes/Kwang/Meshes/Kwang` (anims ham Paragon `Kwang_Idle`, `Kwang_Jog_Fwd`...). Paragon meshlarida morph-targetlar bo'lsa yuz darhol ishlaydi; bo'lmasa jag' suyagi yo'li kerak. Mannequin (SKM_Manny) da morph yo'q — shuning uchun hozir yuz harakati ko'rinmaydi.

## Ertug'rul AssetHub modeli (2026-09-06)
`D:\Yuklanadiganlar\ertugrul.glb` (1.5 mln uchburchak, 80 MB) va `ertugrul1.glb` (500 ming) → `/Game/ErtAssets/Chars/SM_ertugrul*` (Interchange, Nanite yoqilgan, 3 tekstura: bazaviy rang, metall/g'adir-budirlik, normal). Balandlik 116 cm → o'yinda 1.55 masshtab. Rig yo'q (skins=0) — Mixamo auto-rig kerak. Solishtiruv: docs/screens_ertugrul_compare.png (chap 1.5M, o'ng 500k) — 1.5M tanlandi (kaftan panellari, pat, yuz aniqroq). Metallic 0 ga tushirildi (charm/mo'yna).

## Blender MCP (2026-09-06)
Blender 4.2.3 portable: `D:\Blender\blender-4.2.3-windows-x64\blender.exe` (5.2.0 ham bor). Addon: `%APPDATA%\Blender Foundation\Blender\4.2\scripts\addons\blender_mcp_addon.py` (ahujasid/blender-mcp, yoqilgan). `.mcp.json` da `blender: uvx blender-mcp` (port 9876). Ishga tushirish: Blender ochiq + N panel → BlenderMCP → "Connect to Claude" (yoki `--python-expr` bilan `bpy.ops.blendermcp.start_server()`), keyin Claude Code ni qayta ishga tushirish. Vazifa: GLB → FBX konvertatsiya, decimate, Mixamo uchun tayyorlash, rig tekshiruvi.
- Mixamo uchun: `ertugrul.glb` → Blender headless (`bl_decimate.py` + `bl_fix.py`): 1.5M → 150k uchburchak (Decimate collapse, UV saqlanadi), balandlik 1.80 m, oyoq Z=0, teksturalar FBX ichida → `D:\Yuklanadiganlar\ertugrul_fbx\ertugrul_mixamo_150k.fbx` (24 MB). Gotcha: glTF importdan keyin `view_layer.update()` siz `dimensions` eskirgan bo'ladi; `read_factory_settings` MCP serverni o'chiradi (headless `-b --python` ishlating).
- Blender sahnasi (`bl_ertugrul_scene.py`, `D:\Yuklanadiganlar\ertugrul_fbx\ertugrul_scene.blend`): Ertug'rul 150k + realistik material (metall 0, roughness >= 0.55, normal 1.4, rang to'yinganligi 1.25), orqada protsedural yog'och qalqon (taxta to'lqin teksturasi, temir halqa, bo'rtiq, 12 mix; personajga parent), dasht yeri, Nishita osmon, quyosh + to'ldiruvchi yorug'lik, 3 kamera (old 3/4, orqa, yuz). Renderlar: docs/blender/ertugrul_*.png. Gotcha: Blender zarra-soch (hair particle) socket orqali evaluated bo'lmaydi va uzunligi noto'g'ri — soqol/soch modelning o'z teksturasida qoldi; UE uchun strand soch = Groom (Alembic), keyin.
