# BLADES OF ANATOLIA: ERTUĞRUL — GAME DESIGN DOCUMENT (GDD)
## 1-mavsum va 2-mavsum asosida • O'yin arxitekturasi va hikoya tuzilishi

**Janr:** 3D Action-Adventure / Stealth RPG (uchinchi shaxs)
**Dvijok:** Unreal Engine 5.4+ (C++ yadro mantiq, Blueprint — VFX/UI)
**Davr:** XIII asr, Anadolu — Halab — Amanos — Mo'g'ul chegarasi
**Bosh qahramon:** Ertuğrul G'oziy (Suleyman Shohning o'g'li, Qayı qabilasi alpi)
**Referens loyihalar:** Assassin's Creed (stealth/parkour), The Witcher 3 (kvest/dialog), Ghost of Tsushima (duel/stand-off)

> Eslatma: Ushbu hujjatda kod yo'q. Faqat arxitektura, tizimlar nomlari, epizodlar, kvestlar, personajlar va ularning rollari yozilgan. Texnik tizimlar "qaysi komponent/tizim kerak" darajasida tavsiflanadi.

---

# I QISM — O'YIN KONSEPTSIYASI

## 1.1 Bir jumlalik konsept
O'yinchi Ertuğrul sifatida Qayı qabilasini Templar fitnasidan (1-mavsum) va Mo'g'ul bosqinidan (2-mavsum) himoya qiladi; ochiq olamda ot minadi, yashirin kirib boradi, qilich bilan jang qiladi va tarixiy-dramatik kvestlar orqali "alp"dan "bey"ga aylanadi.

## 1.2 Asosiy ustunlar (Design Pillars)
1. **Alp sharafi** — jang halol, og'ir, har zarba sezilarli. Spam qilish jazolanadi, parry/dodge mukofotlanadi.
2. **Soya va shamshir** — har katta missiya ikki yo'l: yashirin (stealth) yoki ochiq jang. Ikkalasi ham to'liq o'ynaladi.
3. **Oba — tirik uy** — Qayı obasi o'yinchi bazasi: rivojlanadi, ovoz beradi, NPC'lar munosabati o'zgaradi.
4. **Iymon va sabr** — hikoya yadro: Ibn Arabiy bilan suhbatlar, zikr, sinov va qayta tug'ilish (2-mavsum).
5. **Seriald dramaturgiya** — har "Epizod" = seriyaning bir bo'lagi; cliffhanger bilan tugaydi.

## 1.3 Asosiy o'yin sikli (Core Loop)
**Obada tayyorlanish → Kvest olish (Bey/Hayme Ana/Ibn Arabiy) → Ot bilan sayohat → Razvedka (stealth/eavesdrop) → Jang yoki infiltratsiya → Qaytish, mukofot, obani rivojlantirish → Oba majlisi (kengash) → Yangi epizod.**

## 1.4 Progressiya
- **Alp darajasi (XP):** jang, stealth, kvestlardan.
- **Uchta ko'nikma daraxti:** *Qilich yo'li* (combat), *Soya yo'li* (stealth), *Yo'lboshchi* (ot, ov, oba buyruqlari, alplarni chaqirish).
- **Sharaf (Honor) ko'rsatkichi:** begunohlarni himoya qilish, asirlarni qiynamaslik → oba sodiqligi oshadi, Halima bilan munosabat, Suleyman Shoh ishonchi.
- **Jihozlar:** qilich (Deli Demir yasaydi), qalqon, kamon, xanjar (chap qo'l), zirh, ot anjomlari.

---

# II QISM — TIZIM ARXITEKTURASI (kodsiz, modullar darajasida)

## 2.1 Umumiy modul tuzilishi
```
Game Core
 ├─ Character Layer
 │   ├─ Player (Ertuğrul)          — harakat, jang, stealth, ot, parkour
 │   ├─ Companion (Turgut/Bamsı/Doğan) — AI hamroh, buyruqlar
 │   └─ NPC / Enemy                 — Templar, Mo'g'ul, qaroqchi, saroy soqchisi
 ├─ Combat Layer
 │   ├─ Melee System (Light/Heavy/Combo/Parry/Dodge/Finisher)
 │   ├─ Ranged System (kamon, nayza uloqtirish)
 │   ├─ Mounted Combat (otda qilich/kamon)
 │   ├─ Injury System (2-mavsum: yaradorlik holati, chap qo'l rejimi)
 │   └─ Boss System (fazali bosslar)
 ├─ Stealth Layer
 │   ├─ Detection (ko'rish konusi, ovoz, yorug'lik)
 │   ├─ Hiding (buta, olomon, soyadan)
 │   ├─ Disguise (rohib/savdogar libosi)
 │   └─ Assassination (yuqoridan, orqadan, burchakdan)
 ├─ World Layer
 │   ├─ Open World Streaming (World Partition)
 │   ├─ Day/Night + Weather
 │   ├─ Oba (baza) boshqaruvi
 │   ├─ Faction & Reputation
 │   └─ Fast Travel (ot bekatlari / karvonsaroylar)
 ├─ Narrative Layer
 │   ├─ Quest System (asosiy/yon/oba kvestlari)
 │   ├─ Dialogue System (tanlovlar, sharaf ta'siri)
 │   ├─ Cutscene (Sequencer)
 │   └─ QTE System
 ├─ Minigames
 │   ├─ Blacksmith (temirchilik ritm o'yini)
 │   ├─ Crafting/Alchemy (Artuk Bey dori-darmon)
 │   ├─ Ov (iz qidirish)
 │   └─ Kengash (majlis dialog-duel)
 └─ UI/UX
     ├─ HUD (sog'lik, chidam, sharaf, ko'rinuvchanlik)
     ├─ Xarita / Kodeks (tarixiy ma'lumotlar)
     └─ Ko'nikma daraxti / Inventar
```

## 2.2 Asosiy tizimlar va ularning vazifasi (nomlari)
| Tizim / Komponent | Vazifasi |
|---|---|
| `UHealthComponent` | Sog'lik, zirh, o'lim/holsizlanish, qon ketish (2-mavsum) |
| `UStaminaComponent` | Og'ir zarba, dodge, parkour, otda chopish uchun chidam |
| `UMeleeCombatComponent` | Kombo daraxti, parry oynasi, hit-reaction, finisher |
| `UTargetLockComponent` | Nishonni qulflash, olomon jangida nishon almashish |
| `URangedComponent` | Kamon tortish, zoom, o'qlar turlari (oddiy, olovli) |
| `UStealthComponent` | Ko'rinuvchanlik darajasi, ovoz, cho'kkalab yurish |
| `UDisguiseComponent` | Libos bilan maskirovka, "shubha" metri |
| `UAssassinationComponent` | Kontekstli o'ldirish (yuqoridan, orqadan, suvdan) |
| `UClimbingComponent` / `UParkourComponent` | Devor, tom, karvonsaroy balkonlari |
| `UHorseComponent` + `AHorseCharacter` | Ot chaqirish, minish, otda jang, chidam |
| `UInjuryStateComponent` | 2-mavsum: o'ng qo'l yaralangan holat, chap qo'l rejimi, qon yo'qotish |
| `UBindingSwordComponent` | Qilichni qo'lga bog'lash — "G'azab / Og'ir zarba" rejimi |
| `ACompanionAIController` | Turgut/Bamsı/Doğan xulqi: ergashish, hujum, yashirinish |
| `AEnemyAIController` (Templar / Mongol / Bandit) | Qo'riqlash, hushyorlik darajalari, guruh bo'lib o'rab olish |
| `ABossAIController` (Titus / Noyan) | Fazalar, maxsus hujumlar, sahna skriptlari |
| `UQuestManagerSubsystem` | Kvest holatlari, maqsadlar, branching |
| `UDialogueSubsystem` | Dialog daraxti, sharaf tanlovlari |
| `UReputationSubsystem` | Fraksiya munosabati (Qayı, Saljuqiy, Halab, Dodurga) |
| `UObaManagerSubsystem` | Oba rivojlanishi, resurslar, NPC jadvallari |
| `UQTESubsystem` | Tugma ketma-ketligi, ritm, "mash" QTE |
| `UBlacksmithMinigame` | Temirchilik ritm o'yini (zikr bilan) |
| `UCutsceneDirector` | Sequencer sahnalarini boshqarish, o'yin→kino o'tishlar |
| `UWeatherTimeSubsystem` | Kun/tun, tuman, qor (2-mavsum qish) |

## 2.3 Jang tizimi (qisqacha dizayn)
- **Yengil zarba** — tez, 3–4 zarbali kombo.
- **Og'ir zarba** — chidam sarflaydi, qalqonni sindiradi.
- **Parry** — aniq vaqtda blok → raqib ochiladi → "alp zarbasi" finisher.
- **Dodge** — yon/orqa, i-frame qisqa.
- **Qalqon** — Templar ritsarlari uchun; o'yinchi qalqonni tortib olishi mumkin.
- **Olomon jangi** — 360° hujum, raqiblar navbat bilan hujum qiladi (AC uslubi), lekin "qiyin" rejimda bir vaqtda.
- **Otda jang** — yonlama qilich zarbasi, o'tib ketishda kamon.
- **Finisher sahnalari** — kontekstli (devor yonida, yerda, otdan tushirish).

## 2.4 Stealth tizimi
- Ko'rinuvchanlik indikatori: yorug'lik + harakat + masofa.
- Yashirinish joylari: buta, xirmon, olomon (Halab bozori), chodir orqasi, tom.
- Hushyorlik darajalari: *Tinch → Shubha → Qidiruv → Jang*.
- Maskirovka: rohib libosi (Amanos), savdogar libosi (Halab). "Shubha metri" — ofitserlarga yaqinlashish xavfli.
- Chalg'itish: tosh otish, otni chaqirish (shovqin), olov.

## 2.5 Ot tizimi
- Ot — doimiy hamroh (ismi: o'yinchi tanlaydi; default "Aktolgali").
- Hushtak bilan chaqirish, otda kamon, otda qilich, nayza uloqtirish.
- Ot chidami — uzoq chopish → charchaydi.
- Yuk: ov o'ljasi, yarador hamrohni olib ketish.

## 2.6 Oba (baza) tizimi
- **Binolar:** Bey chodiri, temirchilik (Deli Demir), shifoxona (Artuk Bey — 2-mavsum), otxona, alplar maydoni, to'quv chodiri (Hayme Ana), oshxona.
- **Resurslar:** go'sht (ov), teri, temir, mato (savdo), oltin.
- **Kengash (majlis):** epizodlar orasida oba oqsoqollari yig'iladi — dialog-duel; o'yinchi tanlovlari sharafga ta'sir qiladi (Kurdoğlu bilan bahslar).
- **Obani himoya qilish** voqealari: qaroqchi/Mo'g'ul hujumi — tower-defense emas, oddiy jang + alplarga buyruq.

---

# III QISM — PERSONAJLAR VA ROLLAR (CAST)

## 3.1 Qayı qabilasi
| Personaj | Roli o'yinda | Gameplay funksiyasi |
|---|---|---|
| **Ertuğrul** | Bosh qahramon, o'yinchi | To'liq boshqaruv |
| **Suleyman Shoh** | Qayı beyi, Ertuğrulning otasi | Asosiy kvest beruvchi (1-mavsum), 1-mavsum oxirida vafot → 2-mavsum boshida motam |
| **Hayme Ana** | Ona, oba ma'naviy tayanchi, keyin "Bey Ana" | Oba kvestlari, 2-mavsumda qabila rahbari |
| **Gündoğdu** | Katta aka, ehtiyotkor, savdo tarafdori | Kengashda raqib/ittifoqchi; ba'zi missiyalarda hamroh |
| **Selcan Xotun** | Gündoğdu xotini, Kurdoğlu ta'sirida | 1-mavsum yon kvestlari (ichki fitna), 2-mavsumda tavba |
| **Dündar** | Kichik uka (yosh) | Ov tutorial hamrohi, oba kvestlari |
| **Turgut Alp** | Eng yaqin do'st, qo'sh bolta | Hamroh: og'ir zarbalar, eshik buzish |
| **Bamsı Beyrek** | Ikki qilichli, hazilkash | Hamroh: olomon jangi, "kuch" QTE, hazil dialoglari |
| **Doğan Alp** | Kamonchi, tez | Hamroh: uzoqdan qo'llab-quvvatlash, razvedka |
| **Deli Demir** | Oba temirchisi, Aykız otasi | Temirchilik minigame, qurol yangilash |
| **Aykız** | Deli Demir qizi, Turgut sevgilisi | Oba kvestlari, 1-mavsum drama (Kurdoğlu hiylasi) |
| **Gökçe** | Selcan singlisi, Ertuğrulni sevadi | Dialog/munosabat yon chizig'i |
| **Kurdoğlu** | Suleyman Shohning "qon qardoshi", XOIN | 1-mavsum ichki antagonist — Epizod 2 da fosh bo'ladi |
| **Akça Koca** | Oqsoqol, donishmand | Kengash, yon kvestlar |
| **Hamza Alp** | Alp, 2-mavsumda Noyan tomoniga o'tadi | 2-mavsum josus chizig'i |
| **Abdurrahman Alp** | Sodiq alp | 2-mavsum hamrohi |
| **Wild Demir (Yabanchi)** | Deli Demir yordamchisi | Minigame NPC |

## 3.2 Saljuqiylar / Halab / ma'naviy shaxslar
| Personaj | Roli |
|---|---|
| **Halima Sulton** | Saljuqiy shahzoda qizi, Ertuğrulning sevgilisi. Ba'zi sahnalarda o'ynaladigan qisqa segment (Epizod 3 da asirlikdan qochish) |
| **Shahzoda Numan** | Halima otasi, qochqin Saljuqiy shahzodasi |
| **Yig'it** | Halima ukasi, Templarlar o'g'irlaydi — 1-mavsum qutqarish kvesti |
| **Ibn Arabiy (Muhyiddin)** | Ma'naviy ustoz; har epizod boshida "hikmat" sahnalari, kodeks ochadi, 2-mavsumda qayta tug'ilish sahnasi |
| **Sulton Alouddin Kayqubod** | Saljuqiy sultoni (2-mavsum) — ittifoq kvesti |
| **Afshin Bey** | Sulton josusi, ikkiyuzlama ittifoqchi |
| **Ertokush Bey** | Saljuqiy qo'mondoni |
| **Saduddin Ko'pak** | Saroy vaziri, yashirin raqib (2-mavsum oxiri, 3-mavsumga ko'prik) |
| **Al-Aziz** | Halab amiri, yosh va ta'sirchan |
| **Nosir (Nasır)** | Halab vaziri, Templar agenti — Epizod 2 antagonisti |
| **Shahobiddin / Sulaymon / Eftelya** | Halab ichki fitna ishtirokchilari; Eftelya — Templar ayol josusi |
| **Hoja/Karvonsaroy egasi** | Yon kvest NPC |

## 3.3 Templarlar (1-mavsum antagonistlari)
| Personaj | Roli |
|---|---|
| **Titus** | Amanos qal'asi qo'mondoni, bosh boss (1-mavsum finali) |
| **Petruccio Manzini (Kardinal)** | Templar rahbari, strateg — Titus ortidagi aql |
| **Bisol** | Titus ukasi, Epizod 1–2 mini-boss |
| **Claudius / Omar** | Templar ritsar, islomni qabul qiladi — yon kvest, keyin ittifoqchi |
| **Templar ritsarlari / Oq-qora qo'shin** | Asosiy dushman turlari: ritsar (qalqon), kamonchi, ruhoniy-ofitser |

## 3.4 Mo'g'ullar (2-mavsum antagonistlari)
| Personaj | Roli |
|---|---|
| **Bayju Noyan** | Bosh antagonist, Mo'g'ul qo'mondoni, finalo bossi |
| **Tangut** | Noyan o'ng qo'li, qiynoq sahnasida ishtirokchi, mini-boss |
| **Ulubilge** | Mo'g'ul shomon/ayg'oqchi |
| **Goncagul** | Noyan josusi, Dodurga obasida (Tuğtekinga uylanadi) |
| **Aytolun** | Korkut Bey xotini, Noyan bilan til biriktirgan xoin |
| **Efrosiyob / Kocabaş** | Josuslar, yon kvestlar |
| **Keshik jangchilari** | Elita Mo'g'ul gvardiyasi: ot ustida kamonchi, og'ir nayzachi |

## 3.5 Dodurga qabilasi (2-mavsum)
| Personaj | Roli |
|---|---|
| **Korkut Bey** | Dodurga beyi, Hayme Ana akasi |
| **Tuğtekin** | Korkut o'g'li, Ertuğrul raqibi-ittifoqchisi |
| **Gumushtekin** | Saljuqiy amiri, Noyan bilan til biriktirgan |
| **Sungurtekin** | Ertuğrulning "o'lgan" akasi, Mo'g'ullar orasidan qaytadi — sirli yordamchi |
| **Artuk Bey** | Tabib, Ertuğrulni davolaydi (crafting NPC) |
| **Banu Chechak** | Dodurga qizi, yon chiziq |
| **Geyikli Baba** | Darvesh, o'rmon ustozi — 2-mavsum qayta tug'ilish sahnalari |

---

# IV QISM — 1-MAVSUM: TEMPLAR FITNASI VA AMANOS QAL'ASI

## EPIZOD 1 — OV VA QUTQARILGAN BEGONALAR (Tutorial fazasi)

### Umumiy
- **Maqsad:** Boshqaruvni o'rgatish, oba bilan tanishtirish, Halima bilan uchrashuv, Templar tahdidini kiritish.
- **Ishtirokchilar:** Ertuğrul, Turgut, Bamsı, Doğan, Dündar (ov boshida), Suleyman Shoh, Hayme Ana, Gündoğdu, Kurdoğlu, Halima, Numan, Yig'it, Deli Demir, Aykız. Dushman: Bisol boshchiligidagi Templar ritsarlari.
- **Cliffhanger:** Begonalarni obaga olib kelishdi; Kurdoğlu yashirincha Templar xabarchisiga xat yozadi.

---

### Kvest 1.1 — Kiyik ovi
**Maqsad:** Ov qilib, obaga go'sht olib qaytish. Harakat, ot minish, kamon tutorial.

**Cutscene / Hikoya:**
Tong. Qayı obasi. Ertuğrul chodirdan chiqadi, Hayme Ana "ehtiyot bo'l" deydi. Dündar ukasi ham borishni so'raydi, Ertuğrul ruxsat beradi (tutorial NPC sifatida ergashadi). Turgut, Bamsı, Doğan otlarda kutishadi. Bamsı hazil: "Kiyik emas, qo'y bo'lsa ham roziman". Suleyman Shoh uzoqdan qarab turadi — otasining qarashi o'yinchiga "bu oba seni kutyapti" hissini beradi.

**Gameplay sikli:**
1. **Piyoda harakat** (yurish, yugurish, cho'kkalash) — oba ichida.
2. **Ot chaqirish va minish** — oba tashqarisida.
3. **Iz qidirish (ov minigame):** kiyik izlari, shox urilgan daraxt — "Ov hissi" ko'rinishi (rangli iz).
4. **Kamon tutorial:** Doğan o'rgatadi — nafas ushlash, shamol, harakatdagi nishon.
5. **Otda quvish:** kiyik qochadi, otda chopib kamondan otish.
6. **O'ljani yuklash** — otga bog'lash, obaga yo'l.

**Kerakli tizimlar:** `UHorseComponent`, `URangedComponent`, `UTrackingSenseComponent` (iz ko'rish), `ACompanionAIController` (tutorial rejimi), `UQuestManagerSubsystem`, `UTutorialPromptSubsystem`.

**Level dizayni:**
- Oba → daryo bo'yi → qarag'ayzor o'rmon → ochiq yaylov.
- Tong nuri, tuman pastda, qushlar ovozi. Yo'l ochiq, o'yinchini o'rmon ichiga "olib kiruvchi" yorug'lik koridori.
- Hech qanday dushman yo'q; faqat bo'ri guruhi (ixtiyoriy — kamonni sinash uchun).

---

### Kvest 1.2 — O'rmondagi pistirma
**Maqsad:** Templarlar quvayotgan Halima, Numan va Yig'itni himoya qilish. Yaqin jang va parry tutorial.

**Cutscene / Hikoya:**
O'rmondan qichqiriq eshitiladi. Ertuğrul va alplar otda yetib borishadi: Templar ritsarlari (Bisol buyrug'i bilan) uch begona — yarador keksa (Numan), yosh ayol (Halima) va bola (Yig'it) ni o'rab olgan. Ertuğrul: "Bu yerlarda begunohga qilich ko'targanning joni bizdan so'raladi." Halima va Ertuğrulning ko'zlari uchrashadi — muzika motivi birinchi bor yangraydi (lyeytmotiv).

**Gameplay sikli:**
1. **Otdan tushib hujum** (mounted dismount attack).
2. **Yengil/og'ir zarba** — 2 ta oddiy askar.
3. **Parry tutorial** — ritsar hujumi, vaqtida blok → finisher.
4. **Dodge tutorial** — og'ir zarbali ritsar.
5. **Mini-boss: Bisol** (qochib ketadi 50% sog'liqda — skriptlangan).
6. **Hamrohlar** jangda ishtirok etadi, o'yinchi buyruq berishni o'rganadi (kichik).

**Kerakli tizimlar:** `UMeleeCombatComponent`, `UTargetLockComponent`, `UHealthComponent`, `AEnemyAIController` (Templar), `ABossAIController` (mini-boss Bisol — qochish skripti), `UCutsceneDirector`.

**Level dizayni:**
- Zich o'rmon ichidagi kichik ochiq maydon ("arena"), atrofida yiqilgan daraxtlar — harakatni cheklash.
- Halima/Numan/Yig'it — markazda, "himoya qilish" holati (ularga zarar yetsa, qayta boshlash emas, balki sharaf pasayadi).
- Kunduz, lekin daraxtlar soyasida — kontrast yorug'lik.

---

### Kvest 1.3 — Birinchi qon va qochish
**Maqsad:** Qochqinlarni Qayı obasiga kuzatib borish; tunda lager himoyasi.

**Cutscene / Hikoya:**
Numan yarador, Halima "biz kimligimizni ayta olmaymiz" deydi. Ertuğrul: "Mehmon — Xudoning omonatidir." Yo'lda tun tushadi, lager qurishadi. Lagerda Halima va Ertuğrul o'rtasida birinchi haqiqiy dialog (o'yinchi 2–3 tanlov: hurmatli / qiziquvchan / ehtiyotkor — munosabat metriga ta'sir). Bamsı sho'rva pishiradi (hazil). Tunda Templarlar qaytib hujum qiladi. Obaga yetib borgach — Suleyman Shoh qabul qiladi, Kurdoğlu "begonalarni olib kelish xavfli" deb e'tiroz bildiradi (birinchi kengash). Hayme Ana Halimani chodirga joylaydi. Yakun: Kurdoğlu tunda xat yozadi.

**Gameplay sikli:**
1. **Eskort:** ot sekin yurishi, Numanni ko'tarib borish (zaif ot), yo'lda tanlov: qisqa xavfli yo'l yoki uzun xavfsiz.
2. **Lager qurish** (qisqa interaktiv: olov yoqish, qo'riqlash joyini tanlash).
3. **Tun himoyasi:** 3 to'lqin — avval kamonchilar (Doğan bilan), keyin ritsarlar, oxirida o'tli o'qlar (chodir yonadi — o'chirish QTE yoki Halimani olib chiqish).
4. **Obaga kirish** — birinchi oba sayri, NPC'lar bilan tanishish (Deli Demir, Aykız, Gökçe, Selcan).
5. **Birinchi kengash** — dialog sahnasi, sharaf tanlovlari.

**Kerakli tizimlar:** `UEscortObjectiveComponent`, `UWaveSpawnerSubsystem`, `UQTESubsystem` (olov o'chirish), `UObaManagerSubsystem` (oba ochilishi), `UDialogueSubsystem`, `UReputationSubsystem`.

**Level dizayni:**
- O'rmon chekkasi → tog' yo'li → daryo kechuvi → oba vodiysi.
- Tungi lager: olov atrofida yorug'lik doirasi, tashqari zulmat — dushmanlar qorong'idan chiqadi (birinchi "dahshat" hissi).
- Oba: 40–60 chodir, markazda Bey chodiri, atrofda qo'riqchi minoralari (yog'och), otxona, temirchilik tutuni.

---

## EPIZOD 2 — SAVDO, JOSUSLIK VA SOYA URUSHI

### Umumiy
- **Maqsad:** Qayı obasi yaylov va savdo kelishuvi uchun Halabga boradi. Halabda Templar-Nosir fitnasi, Ibn Arabiy bilan uchrashuv, karvonsaroy hujumi, Kurdoğlu xoinligi fosh bo'ladi.
- **Ishtirokchilar:** Ertuğrul, Turgut, Bamsı, Doğan, Ibn Arabiy, Al-Aziz, Nosir, Eftelya, Titus (niqobda), Bisol, Claudius/Omar, Halima (oba), Kurdoğlu, Selcan, Gündoğdu, Suleyman Shoh, Afshin Bey.
- **Cliffhanger:** Yig'it o'g'irlanadi, Amanosga olib ketiladi. Kurdoğlu fosh bo'lib, qochadi yoki qatl etiladi (o'yinchi tanlovi → sharaf).

---

### Kvest 2.1 — Halabga sayohat
**Maqsad:** Halab amiri Al-Aziz bilan yaylov kelishuvi. Shahar infiltratsiyasi va parkour.

**Cutscene / Hikoya:**
Suleyman Shoh Ertuğrulni elchi qilib yuboradi (Gündoğdu e'tiroz bildiradi — "men kattaman", Kurdoğlu Gündoğduni qo'llaydi — ichki ziddiyat). Yo'lda karvon, sahro. Halab darvozasida soqchilar "turkmanlar qurol bilan kirmaydi" deydi — o'yinchi qilichlarini topshiradi (stealth sababi). Shaharda Ibn Arabiy bilan birinchi uchrashuv bozorda: "Yo'lingni yo'qotsang, yulduzlarga emas, qalbingga qara." Saroyda Al-Aziz iliq kutib oladi, lekin vazir Nosir sovuq. Eftelya (Templar josusi) raqs sahnasida Ertuğrulni kuzatadi.

**Gameplay sikli:**
1. **Ot bilan uzoq sayohat** — ochiq olam, yo'lda yon kvest (qaroqchilar karvonni talayapti — tanlov).
2. **Halab bozori — olomon stealth:** qurolsiz; Nosir odamlari Ertuğrulni ta'qib qiladi → olomon ichida yashirinish, tomlarga chiqish (parkour tutorial).
3. **Parkour yo'li:** bozor ayvonlari → tom → masjid hovlisi → Ibn Arabiy uyi (tinch "hub").
4. **Saroy sahnasi** — dialog-duel Nosir bilan (ayblov va javob, sharaf).
5. **Tun — saroy yashirin tekshiruvi (ixtiyoriy):** Nosir xonasiga kirib Templar muhrli xatni topish (kelajak kvest uchun dalil).

**Kerakli tizimlar:** `UParkourComponent`, `UClimbingComponent`, `UStealthComponent` (olomon yashirinish rejimi), `UCrowdSubsystem` (bozor NPC olomoni), `UDialogueSubsystem` (duel rejimi), `UWorldStreaming` (shahar zona).

**Level dizayni:**
- Halab: devor bilan o'ralgan shahar hubi — bozor (zich, rangli mato soyabonlar), saroy (marmar, hovuz), masjid hovlisi, tor ko'chalar, tomlar bir-biriga yaqin (parkour uchun).
- Kunduzi issiq, oltin-sariq yorug'lik, chang. Tunda — mash'alalar, saroy soqchilari patrul.
- Dushman joylashuvi: bozorda 6–8 Nosir ayg'oqchisi (ko'k sallali), saroyda 12 soqchi (3 patrul marshruti).

---

### Kvest 2.2 — Karvonsaroy tungi pistirmasi
**Maqsad:** Halab yo'lidagi karvonsaroyda Templar hujumini qaytarish; tungi stealth jang va assassinatsiyalar.

**Cutscene / Hikoya:**
Qayı karvoni (mato, teri) karvonsaroyga tunaydi. Karvonsaroy egasi shubhali. Turgut: "Bu yerdagi sukunat menga yoqmayapti." Yarim tun — Templarlar (Bisol boshchiligida, Nosir ma'lumoti bilan) karvonsaroyni o'rab oladi, mash'alalar o'chadi. Ertuğrul alplarni uyg'otadi: "Qilich emas, soya bo'laylik." Oxirida Bisol bilan haqiqiy duel → Bisol o'ladi (yoki asirga olinadi — tanlov: asir → Titus haqida ma'lumot; o'ldirish → Titus qasos motivatsiyasi kuchayadi).

**Gameplay sikli:**
1. **Stealth bosqich:** hovlidagi 10–12 Templar, mash'ala yorug'ligi orasida. Suv havzasiga sho'ng'ish, balkondan sakrab o'ldirish, chodir orqasidan xanjar.
2. **Mash'alalarni o'chirish** — qorong'ilik zonasini kengaytirish (stealth afzalligi).
3. **Alplarga buyruq:** Doğan — tomdan kamon, Turgut — eshikni ushlab turish, Bamsı — chalg'itish.
4. **Ochiq jangga o'tish** (agar fosh bo'lsa yoki skript bo'yicha): hovli jangi, 2 to'lqin.
5. **Duel: Bisol** — parry-asosiy duel, Bisol qalqonli, "qalqon sindirish" mexanikasi o'rgatiladi.
6. **Tanlov:** Bisolni o'ldirish / asirga olish.

**Kerakli tizimlar:** `UAssassinationComponent` (yuqoridan, suvdan), `ULightingAwarenessSubsystem` (mash'ala o'chirilganda ko'rinuvchanlik pasayadi), `UCompanionCommandSubsystem`, `ABossAIController` (Bisol duel), `UChoiceConsequenceSubsystem`.

**Level dizayni:**
- Klassik to'rtburchak karvonsaroy: markazda hovli + hovuz, ikki qavatli ayvonlar, tuyaxona, darvoza.
- Tun, to'lin oy, mash'alalar — yorug'lik "orollari" va soya koridorlari.
- Templarlar: 4 ta darvoza soqchisi, 2 ta tomda kamonchi, 6 ta hovli patruli, Bisol ikkinchi qavat ayvonida.
- Vertikallik: ayvonlar → tomga zinalar → hovuz (suv yashirinish).

---

### Kvest 2.3 — Xoinni fosh qilish
**Maqsad:** Kurdoğlu va Templar agentlarining uchrashuvini eshitish, dalil to'plash, kengashda fosh qilish.

**Cutscene / Hikoya:**
Obaga qaytgach Ertuğrul Nosir xonasidan olgan xatda Qayı muhrining nusxasini ko'radi. Halima "Kurdoğlu menga sovuq qarar edi" deydi. Selcan Xotun Kurdoğlu bilan sirli gaplashadi (Selcan — aldangan, xoin emas, lekin ishtirokchi). Ertuğrul tunda Kurdoğluni ta'qib qiladi — Kurdoğlu o'rmondagi eski tegirmonda Templar agenti va Eftelya bilan uchrashadi: reja — Suleyman Shohni zaharlash, Yig'itni Templarlarga topshirish, Ertuğrulni ayblash. Kengash sahnasi: Ertuğrul dalillarni ko'rsatadi, Kurdoğlu inkor qiladi, Suleyman Shoh sud qiladi. **Shu payt** Yig'itning o'g'irlangani xabari keladi — Kurdoğlu tartibsizlikdan foydalanib qochadi (yoki o'yinchi uni ushlab qoladi — quvish segmenti). Suleyman Shoh zaharlangan, og'ir ahvolda (1-mavsum oxirida vafotiga zamin).

**Gameplay sikli:**
1. **Oba ichida tergov:** 4 ta dalil (xat, muhr, Selcan dialogi, Aykız guvohligi) — "tergov" interfeysi.
2. **Kurdoğluni ta'qib qilish (tailing):** masofa saqlash, butalarda yashirinish, orqadan ergashish.
3. **Eavesdropping (tinglash):** tegirmon tashqarisida yashirinib suhbatni to'liq eshitish — qanchalik yaqin bo'lsa, shuncha ko'p ma'lumot (dalil foizi).
4. **Tanlov:** darhol hujum (jang, lekin kam dalil) / to'liq tinglab kengashga borish (sharaf).
5. **Kengash dialog-dueli:** dalillarni ketma-ket ko'rsatish, Kurdoğlu javoblariga qarshi tanlov.
6. **Kurdoğlu qochishi — otda quvish** (oba → o'rmon), tutsa — jazo tanlovi.

**Kerakli tizimlar:** `UInvestigationSubsystem` (dalillar), `UTailingObjectiveComponent`, `UEavesdropComponent` (masofaga qarab ovoz/subtitr), `UDialogueSubsystem` (dalil-tanlov rejimi), `UHorseChaseSubsystem`, `UReputationSubsystem`.

**Level dizayni:**
- Oba tungi rejim: chodirlar orasida soyalar, qo'riqchilar (do'st, lekin ko'rsa "nega uxlamayapsan" dialogi).
- O'rmon yo'li → eski suv tegirmoni daryo bo'yida: g'ildirak shovqini (ovozni yashiradi), deraza tirqishlari, tom.
- Kengash: Bey chodiri ichi — yumaloq o'tirish, olov markazda, kamera o'yinchi tanlovida yuzlarga yaqinlashadi.

---

## EPIZOD 3 — AMANOS QAMALI (1-mavsum finali)

### Umumiy
- **Maqsad:** Yig'itni qutqarish, Halimani (o'g'irlangan bo'lsa) qutqarish, Titus va Templar fitnasini yo'q qilish.
- **Ishtirokchilar:** Ertuğrul, Turgut, Bamsı, Doğan, Claudius/Omar (ichkaridan yordam), Halima, Yig'it, Numan, Titus, Petruccio, Templar ritsarlari, Afshin Bey (tashqaridan Saljuqiy kuchi — tanlovga qarab), Gündoğdu (qo'shin bilan keladi).
- **Cliffhanger / Yakun:** Titus o'ladi, Yig'it qutqariladi. Suleyman Shoh vafot etadi (motam cutscene). Qayı oba ko'chishga qaror qiladi — Mo'g'ul tahdidi uzoqdan eshitiladi (2-mavsumga ko'prik).

---

### Kvest 3.1 — Amanos qal'asiga infiltratsiya
**Maqsad:** Rohib/savdogar niqobida qal'aga kirish, darvoza soqchilarini yashirin yo'q qilish, darvozani ochish.

**Cutscene / Hikoya:**
Claudius (endi Omar) Ertuğrulga qal'a ichki tuzilishini chizib beradi: "Har juma rohiblar karvoni kiradi." Alplar rohib libosini kiyadi (Bamsı: "Bu libos menga tor" — hazil). Yo'lda Ibn Arabiy duosi. Qal'a darvozasida ofitser tekshiradi — dialog QTE (lotincha "Pax vobiscum" javobi — Omar o'rgatgan). Ichkarida Petruccio Titusga "Ertuğrul keladi, tayyor bo'l" deydi — o'yinchi buni eshitishi mumkin.

**Gameplay sikli:**
1. **Disguise bosqichi:** rohib libosida qal'a hovlisi. Shubha metri — ofitserlarga yaqinlashma, yugurma, ibodat animatsiyasi bilan "qo'shilish".
2. **Darvoza soqchilari:** 4 ta soqchi + 2 ta minora kamonchisi — yashirin o'ldirish (orqadan, minoradan tashlash).
3. **Zindonga yo'l (ixtiyoriy):** Yig'itni topish — asirlar orasida, Yig'it qo'rquvda; uni xavfsiz joyga olib chiqish.
4. **Darvoza mexanizmi:** ikki g'ildirak — Turgut va Ertuğrul birga (QTE sinxron), tashqaridagi Gündoğdu qo'shini kiradi.
5. Agar fosh bo'lsa — "qal'a signali" → 3.2 ga erta o'tish (qiyinroq).

**Kerakli tizimlar:** `UDisguiseComponent`, `USuspicionMeterComponent`, `UAssassinationComponent`, `UInteractiveMechanismComponent` (darvoza g'ildiragi), `UQTESubsystem` (sinxron QTE), `UAlarmStateSubsystem`.

**Level dizayni:**
- Amanos: tog' ustidagi tosh qal'a — tashqi devor, darvoza minoralari, ichki hovli, cherkov, zindon, Titus minorasi.
- Kech kuz, kulrang osmon, yomg'ir boshlanadi (finalga yaqin kuchayadi).
- Joylashuv: hovlida 15 rohib (do'st/neytral NPC), 8 soqchi, ofitser 2 ta (shubha kuchli), minoralarda 4 kamonchi.

---

### Kvest 3.2 — Qal'a hovlisi jangi
**Maqsad:** Qayı alplari va Templar ritsarlari o'rtasidagi katta jang; hovlini egallash.

**Cutscene / Hikoya:**
Darvoza ochiladi, Gündoğdu otda kiradi: "Qayı uchun!" Ertuğrul va Gündoğdu birinchi marta yelkama-yelka — aka-uka ziddiyati bir lahza unutiladi. Jang o'rtasida Petruccio Halimani (u Kurdoğlu yordamida o'g'irlangan) minoraga olib chiqadi. Yomg'ir kuchayadi.

**Gameplay sikli:**
1. **Katta jang (30–40 dushman, to'lqinlar):** hovli 3 zonaga bo'lingan — darvoza, cherkov oldi, minora zinasi.
2. **Alplarga buyruq:** "Turgut — chap qanot", "Doğan — minorani otib tur", "Bamsı — men bilan".
3. **Maxsus dushmanlar:** og'ir ritsar (qalqon+gurzi), kamonchilar, "ruhoniy-ofitser" (atrofdagilarni kuchaytiradi — birinchi o'ldirish kerak).
4. **Muhit mexanikalari:** yonayotgan aravani itarish, zanjirni kesib darvoza panjarasini tushirish (dushman to'lqinini kesish).
5. **Minora zinasi** — tor joyda 1-2 raqib bilan jang (koridor jangi), yuqoriga.

**Kerakli tizimlar:** `ULargeBattleSubsystem` (to'lqin va zonalar), `UCompanionCommandSubsystem`, `AEnemyAIController` (Heavy Knight / Archer / Priest), `UEnvironmentalHazardComponent`, `UCrowdCombatLOD` (ko'p NPC optimallashtirish).

**Level dizayni:**
- Hovli: tosh plitalar, yomg'irda yaltiraydi, markazda quduq, chetda yonayotgan aravalar.
- Yorug'lik: chaqmoq (dinamik yorug'lik), mash'alalar, yomg'ir partikllari.
- Dushman: darvoza zonasida 12, cherkov oldida 14, zinada 6–8.

---

### Kvest 3.3 — Final boss: Titus
**Maqsad:** Titusni mag'lub etish, Halima va Yig'itni qutqarish.

**Cutscene / Hikoya:**
Minora tepasi, yomg'ir va chaqmoq. Titus Halimani tutib turibdi: "Sening xalqing yo'q bo'ladi, turk." Ertuğrul: "Biz yo'q bo'lganimizda ham, haq yo'q bo'lmaydi." Petruccio qochadi (2-mavsum yon chizig'i uchun). Duel boshlanadi. Yakun: Ertuğrul Titusni finisher bilan mag'lub etadi — "Alp zarbasi". Halima va Ertuğrul birinchi bor ochiq bir-biriga tuyg'u izhor qiladi (yomg'irda). Qaytish — Suleyman Shoh so'nggi nafasida Ertuğrulga qilichini beradi: "Obani senga emas, Xudoga topshiraman, lekin sen uning qo'riqchisisan." Motam. Titr: 1-mavsum tugadi.

**Boss fazalari:**
- **1-faza — Qalqon va qilich:** Titus qalqonli, og'ir. O'yinchi parry orqali qalqonni sindirishi kerak (3 muvaffaqiyatli parry → qalqon yoriladi).
- **2-faza — Ikki qo'lli uzun qilich:** tez va uzoq masofa. Dodge asosiy. Titus minora atrofini aylanib, ustunlarni ishlatadi.
- **3-faza — Yaralangan g'azab:** Titus tez zarbalar, mash'ala bilan olov zonasi yaratadi; chaqmoq chaqnaganda ko'rish qiyinlashadi (vizual mexanika).
- **Finisher QTE:** Ertuğrul qilichni Titus qalqon qoldig'idan o'tkazadi — "Alp zarbasi" animatsiyasi.

**Kerakli tizimlar:** `ABossAIController` (Titus), `UBossPhaseComponent`, `UShieldBreakComponent`, `UArenaHazardComponent` (olov), `UQTESubsystem` (finisher), `UCutsceneDirector` (boss→kino o'tishlar), `UWeatherTimeSubsystem` (chaqmoq sinxron).

**Level dizayni:**
- Aylana minora tepasi (diametr ~25 m), atrofi devor qismlari, 4 ustun, bir tomoni buzilgan — jarlik (Titus o'yinchini itarishi mumkin — dodge).
- Yomg'ir, chaqmoq, qora-ko'k palitra, mash'ala olovi — qizil aksent.

---

# V QISM — 2-MAVSUM: MO'G'UL BOSQINI VA QAYTA TUG'ILISH

## Mavsum o'tishi (prolog)
Qayı oba Suleyman Shoh vafotidan keyin Hayme Ana boshchiligida sharqqa, Dodurga qabilasi (Korkut Bey) yoniga ko'chadi. Qish boshlanadi. Gündoğdu va Ertuğrul o'rtasida "Bey kim bo'ladi" ziddiyati. Mo'g'ul qo'mondoni Bayju Noyan Anadoluga kiradi. Hamza Alp yashirincha Noyan tomonga o'tgan. Aytolun va Goncagul Dodurga ichidan fitna qiladi.

## EPIZOD 4 — NOYAN SOYASI VA ASIRLIK

### Umumiy
- **Ishtirokchilar:** Ertuğrul, Turgut, Bamsı, Doğan, Abdurrahman, Hamza (xoin), Tuğtekin, Korkut Bey, Hayme Ana, Gündoğdu, Halima, Bayju Noyan, Tangut, Ulubilge, Keshik jangchilari, Sungurtekin (sirli ko'rinish).
- **Yakun:** Ertuğrul o'ng qo'li mixlangan, yarador, bir o'zi qochadi; o'rmonda Geyikli Baba topib oladi.

---

### Kvest 4.1 — Chegaradagi pistirma
**Maqsad:** Mo'g'ul Keshiklariga qarshi chegara jangi; Tuğtekin bilan birga (majburiy ittifoq).

**Cutscene / Hikoya:**
Korkut Bey chegaradan Mo'g'ul razvedkasi haqida xabar oladi. Ertuğrul va Tuğtekin birga yuborilishadi — ikkisi bir-birini yoqtirmaydi (dialog: sovuq/diplomatik/do'stona). Hamza "yo'lni biladi" — aslida Noyanga olib boryapti. Qorli dara. Keshiklar ot ustida kamon bilan paydo bo'ladi. Jang. Noyan uzoqdan tomosha qiladi: "Mana o'sha Ertuğrul." Jang oxirida Hamza xoinligi — Ertuğrulni orqadan zarba bilan hushsiz qiladi, Keshiklar asirga oladi. Turgut yarador, Tuğtekin chekinadi.

**Gameplay sikli:**
1. **Qorda ot bilan yurish** — ot sekinlashadi, izlar ko'rinadi (Mo'g'ul izlari — tracking).
2. **Otda jang:** Keshik kamonchilari aylanib otadi — otda quvib tushirish, nayza uloqtirish.
3. **Piyoda jang darada:** og'ir nayzachilar (Keshik), yangi dushman turi — "lasso" (arqon bilan otdan tushiradi).
4. **Tuğtekin bilan hamkorlik:** vaqtinchalik hamroh, "birga zarba" QTE.
5. **Skriptlangan mag'lubiyat:** oxirgi to'lqinda Hamza xiyonati — cutscene.

**Kerakli tizimlar:** `UMountedCombatComponent`, `AEnemyAIController` (Keshik: otliq kamonchi, nayzachi, lasso), `USnowDeformationSubsystem` (qor izlari, vizual), `UTemporaryCompanionComponent` (Tuğtekin), `UScriptedDefeatSubsystem`.

**Level dizayni:**
- Tog' darasi, qor, qarag'aylar, muzlagan soy. Tor o'tish joyi — pistirma uchun ideal.
- Oq-kulrang palitra, Mo'g'ul qizil-qora bayroqlari kontrast.
- Dushman: 8 otliq kamonchi (aylanish marshruti), 6 nayzachi, 2 lasso, Tangut uzoqdan (jangga kirmaydi).

---

### Kvest 4.2 — Qiynoq va mix sahnasi
**Maqsad:** Cutscene + QTE: Noyan va Tangut Ertuğrulning o'ng qo'lini yog'och ustunga mixlaydi.

**Cutscene / Hikoya:**
Mo'g'ul lageri, tun, qor. Ertuğrul ustunga bog'langan. Noyan: "Seni o'ldirmayman. Qilich tutadigan qo'lingni o'ldiraman. Xalqing seni kuchsiz ko'rsin." Tangut mixni oladi. Ulubilge shomon qo'ng'irog'i chaladi. Noyan Ertuğrulni "tiz cho'k, Qayıni Mo'g'ulga topshir" deb taklif qiladi — **o'yinchi tanlovi**: indamaslik / "Hech qachon" / duo o'qish (hammasi bir natijaga olib keladi, lekin sharaf va keyingi dialoglarga ta'sir). Mix uriladi — ekran qora, faqat ovoz. Ertuğrul ichida "Yo Sabur" deydi. Hamza chetda ko'zini olib qochadi (keyinchalik vijdon azobi chizig'i). Sungurtekin Mo'g'ul libosida olomon orasida — Ertuğrulga ko'z tashlaydi (sir).

**Gameplay sikli:**
1. **Interaktiv cutscene:** kamera Ertuğrul ko'zidan, nafas olish QTE (ritmik tugma — sabr ko'rsatkichi).
2. **Qarshilik QTE:** "mash" — jismonan qarshilik, lekin maqsad yutish emas, balki "sinmasdan turish" (og'riq shkalasi to'lsa — hushdan ketadi, lekin o'yin davom etadi; to'lmasa — qo'shimcha dialog, sharaf).
3. **Dialog tanlovi** Noyan bilan.
4. **Mix sahnasi** — nazorat yo'q, tovush dizayni va qora ekran.
5. **Post-sahna:** Ertuğrul qafasda, o'ng qo'l bog'langan — `UInjuryStateComponent` faollashadi.

**Kerakli tizimlar:** `UCutsceneDirector` (interaktiv sequencer), `UQTESubsystem` (ritm + mash), `UInjuryStateComponent` (doimiy yarador holat), `UDialogueSubsystem`, `UAudioOcclusionSubsystem` (qora ekran ovoz dizayni).

**Level dizayni:**
- Mo'g'ul lageri markazi: katta gulxan, shomon ustuni, qafaslar, Noyan chodiri (oq-qizil).
- Qor yog'yapti, gulxan qizil-sariq, atrof zangori tun.
- Olomon: 20–30 Mo'g'ul askari aylana bo'lib (do'st NPC emas, cutscene olomoni).

---

### Kvest 4.3 — Bir qo'lli qochish
**Maqsad:** Og'ir yarador holda Mo'g'ul lageridan qochish — faqat chap qo'l xanjari va stealth.

**Cutscene / Hikoya:**
Tun oxiri, qorovul almashinuvi. Sungurtekin (niqobda) Ertuğrul qafasiga xanjar tashlab ketadi: "Sharqqa qarab yur, daryoni kuzat" — ismini aytmaydi. Ertuğrul qafasdan chiqadi. Qon ketyapti, ko'rish xiralashadi. Qochish. Oxirida daryoga sakrash, oqim olib ketadi — o'rmonda Geyikli Baba va Artuk Bey topib oladi. Noyan g'azablanadi, Hamzani "seni ham mixlayman" deb qo'rqitadi.

**Gameplay sikli (Survival Stealth rejimi):**
1. **Yangi boshqaruv:** o'ng qo'l ishlamaydi — faqat chap qo'l xanjari, qilich yo'q, parry yo'q. Jang = faqat stealth o'ldirish (orqadan) yoki qochish.
2. **Qon ketish shkalasi:** vaqt o'tgan sari sog'lik kamayadi; yo'lda latta/bint topish kerak (3 ta check-point).
3. **Ko'rish effekti:** xiralashish, tovush "tunnel" effekti — HUD minimal.
4. **Yashirinish:** qorda izlar qoladi (dushman izni kuzatadi!) — daryo/toshlar orqali izni yo'qotish.
5. **Chalg'itish:** otlarni qo'yib yuborish (otxona) — katta chalg'itish.
6. **Ot o'g'irlash (ixtiyoriy):** bir qo'l bilan minish — boshqaruv qiyinlashgan.
7. **Oxirgi segment:** Tangut quvadi — jang emas, qochish; daryoga sakrash QTE.

**Kerakli tizimlar:** `UInjuryStateComponent` (chap qo'l rejimi, parry o'chirilgan), `UBleedingComponent`, `UStealthComponent` (iz qoldirish), `USnowTrackSubsystem` (dushman AI izni kuzatadi), `UDistractionSubsystem`, `UPostProcessStateManager` (xiralashish), `UChaseSequenceComponent`.

**Level dizayni:**
- Lager: qafaslar → ochiq maydon → otxona → chodirlar labirinti → palisad devor → qorli o'rmon → daryo jarlik.
- Tong oldi ko'k-kulrang, gulxanlar so'nayapti — soyalar ko'p.
- Qorovul: 14 ta, 4 patrul marshruti, 2 it (hidlab topadi — yangi xavf), minora kamonchi 2.

---

## EPIZOD 5 — MUQADDAS QAYTA TUG'ILISH VA ZIKR

### Umumiy
- **Ishtirokchilar:** Ertuğrul, Geyikli Baba, Artuk Bey, Ibn Arabiy (ruhiy ko'rinish/tush), Deli Demir, Turgut, Bamsı, Doğan, Abdurrahman, Halima, Hayme Ana, Gündoğdu, Selcan (tavba sahnasi), Dündar, Korkut Bey, Tuğtekin; dushman: Mo'g'ul razvedkachilari (Ulubilge boshchiligida).
- **Yakun:** Ertuğrul qilichi qo'liga bog'langan holda qaytadi — oba uni "tirilgan" deb kutib oladi. Noyan xabar oladi: "U hali tirikmi?!"

---

### Kvest 5.1 — Temirchilikka qaytish
**Maqsad:** Davolanish, ruhiy tiklanish va Deli Demir bilan qilichni qayta yasash — ritmik temirchilik minigame'i.

**Cutscene / Hikoya:**
**1-bo'lim — O'rmon (Geyikli Baba):** Ertuğrul isitmada. Tush: Ibn Arabiy bilan suhbat — "Qo'ling sinmadi, sinovdan o'tdi. Qilich qo'lda emas, qalbda tutiladi." Geyikli Baba: "Sabr — jangning yarmi." Artuk Bey qo'lni davolaydi: "Bu qo'l endi avvalgidek bo'lmaydi. Lekin boshqacha bo'ladi."
**2-bo'lim — Obaga qaytish:** Halima yig'laydi, Hayme Ana peshonasidan o'padi, Gündoğdu uzr so'raydi (aka-uka yarashuvi). Selcan tavba qiladi.
**3-bo'lim — Temirchilik:** Deli Demir: "Bu qilichni men yasagan edim, endi ikkimiz yasaymiz." Turgut, Bamsı, Doğan atrofda turadi. Har bolg'a zarbasida zikr:
- 1-zarba: **"Yo Alloh!"**
- 2-zarba: **"Yo Qodir!"**
- 3-zarba: **"Yo Qaviy!"**
- 4-zarba: **"Yo Kabir!"**
Alplar va oba ahli jo'r bo'ladi, ritm tezlashadi, uchqunlar — kamera aylanadi. Oxirida Deli Demir teri tasmani oladi: "Qo'ling qilichni tutolmasa — qilich qo'lingni tutadi." Tasma bilan bog'laydi. Ertuğrul qilichni birinchi marta ko'taradi — **"G'azab / Og'ir zarba"** ochiladi.

**Gameplay sikli:**
1. **Davolanish minigame (Artuk Bey):** dori tayyorlash — o'simliklarni tanlash (crafting).
2. **O'rmonda yurish (chap qo'l bilan ov — xanjar):** ruhiy tiklanish segmenti, Geyikli Baba bilan suhbatlar, kiyiklar.
3. **Obaga qaytish — hub sahnalari:** dialoglar, munosabat yangilanishi.
4. **Temirchilik ritm minigame'i (`UBlacksmithMinigame`):**
   - Ekranda bolg'a "ritm yo'li" (rhythm track) — 4/4 takt.
   - Har to'g'ri zarba → zikr jo'r ovozi + uchqun VFX + "Iymon" shkalasi to'ladi.
   - 3 bosqich: *Qizdirish* (ko'rik bosish — ritm), *Zarb* (bolg'a — aniqlik), *Toblash* (suvga botirish — vaqt QTE).
   - Perfect zarbalar ketma-ketligi → oba ahli jo'ri balandlashadi (audio layerlar qo'shiladi).
   - Natija qilich sifatiga ta'sir qiladi (zarba kuchi bonusi: +5% / +10% / +15%).
5. **Tasma bog'lash sahnasi** — interaktiv: o'yinchi tasmani tortadi (QTE) → yangi qobiliyat ochiladi.
6. **Yangi boshqaruv tutorial:** "G'azab zarbasi" — chidam o'rniga "Iymon" shkalasi sarflaydi; sekin, lekin qalqon va zirhni buzadi, bir zarbada oddiy dushmanni yiqitadi. Parry qaytadi, lekin "og'riq" — parry keyin qisqa stun.

**Kerakli tizimlar:** `UBlacksmithMinigame`, `URhythmTrackComponent`, `UAudioLayerSubsystem` (zikr jo'ri qatlamlari), `UCraftingSubsystem` (Artuk Bey), `UBindingSwordComponent` (tasma rejimi), `UFaithMeterComponent` ("Iymon" shkalasi), `UAbilityUnlockSubsystem`, `UCutsceneDirector`.

**Level dizayni:**
- Geyikli Baba g'ori: o'rmon ichida, sharshara, kiyiklar, tuman — sokin, yashil-oltin.
- Oba qishki ko'rinish: qor, chodirlar tutuni, iliq sariq ichki yorug'lik.
- Temirchilik: yarim ochiq chodir, ko'rik olovi — butun sahna qizil-to'q sariq, tashqarida ko'k qor. Oba ahli aylana bo'lib turadi, mash'alalar.

---

### Kvest 5.2 — Iymon sinovi
**Maqsad:** Oba chegarasiga hujum qilgan Mo'g'ul razvedkachilarini yangi bog'langan qilich bilan yo'q qilish.

**Cutscene / Hikoya:**
Ulubilge boshchiligidagi razvedka otryadi obaga yaqinlashadi — maqsad Ertuğrul tirik yoki yo'qligini tekshirish. Doğan minoradan ko'radi. Ertuğrul: "Bugun Noyanga javob yuboramiz." Halima uni to'xtatmoqchi, Hayme Ana: "Qo'yib yubor, u o'g'lim — u tirilishi kerak." Jang oxirida Ertuğrul Ulubilgeni tirik qoldiradi: "Noyanga ayt: Ertuğrul tirik, qo'li esa qilichga bog'langan."

**Gameplay sikli:**
1. **Oba himoyasi:** 3 nuqta (darvoza, otxona, temirchilik) — alplarga joylashuv buyrug'i.
2. **Yangi jang uslubi testi:** G'azab zarbasi bilan qalqonli Mo'g'ul askarini sindirish; parry-stun balansi.
3. **Mini-boss: Ulubilge** — shomon: tutun bombalar, ikki xanjar; o'yinchi stealth (tutunda) va G'azab zarbasini kombinatsiya qiladi.
4. **Tanlov:** Ulubilgeni o'ldirish / xabarchi qilib yuborish (sharaf + Noyan reaksiyasi 6-epizodda).
5. **Oba jo'ri:** jang paytida "Iymon" shkalasi to'lganda oba ahli takbir aytadi (audio buff).

**Kerakli tizimlar:** `UBindingSwordComponent`, `UFaithMeterComponent`, `ULargeBattleSubsystem` (kichik rejim), `ABossAIController` (Ulubilge: tutun, teleport-illyuziya), `UCompanionCommandSubsystem`, `UChoiceConsequenceSubsystem`.

**Level dizayni:**
- Oba chegarasi: yog'och palisad, qorli dala, o'rmon chekkasi — dushman o'rmondan chiqadi.
- Kunduz, qor yorug'ligi — aniq, sof, "tiriklik" hissi (4-epizod zulmatidan kontrast).
- Dushman: 12 Mo'g'ul (2 qalqonli), 4 otliq kamonchi, Ulubilge.

---

## EPIZOD 6 — HISOB-KITOB (2-mavsum finali)

### Umumiy
- **Ishtirokchilar:** Ertuğrul, Turgut, Bamsı, Doğan, Abdurrahman, Gündoğdu, Tuğtekin, Korkut Bey (vafot — Aytolun fitnasi), Hayme Ana, Halima, Sungurtekin (ochiq qaytish), Sulton Alouddin, Ertokush Bey, Saduddin Ko'pak (soyada), Gumushtekin (xoin — fosh), Aytolun, Goncagul, Hamza (tavba yoki o'lim), Bayju Noyan, Tangut, Keshiklar.
- **Yakun:** Noyan mag'lub, qo'li mixlanadi. Qayı oba g'arbga — yangi yurt sari ko'chadi. Halima bilan nikoh. Saduddin Ko'pak soyada: "Bu Ertuğrul xavfli" — 3-mavsumga ko'prik.

---

### Kvest 6.1 — Beyliklarni birlashtirish
**Maqsad:** Saljuqiy sultoni va turkman beyliklarini Noyanga qarshi birlashtirish — diplomatiya, ot sayohati, ichki xoinlarni fosh qilish.

**Cutscene / Hikoya:**
Sungurtekin ochiq qaytadi — Hayme Ana "o'lgan" o'g'lini quchoqlaydi; u Mo'g'ullar orasida Sulton josusi bo'lgan. Sungurtekin Gumushtekin va Aytolun xoinligini ochadi. Korkut Bey zaharlanib o'ladi — Tuğtekin Bey bo'ladi, Ertuğrul bilan to'liq yarashadi. Ertuğrul Sulton Alouddin huzuriga boradi (Konya): saroy sahnasi, Saduddin Ko'pak shubha bilan kuzatadi. Sulton Ertokush Bey qo'shinini beradi. Hamza tavba qilib Noyan lageri rejasini beradi (yoki o'yinchi uni rad etadi).

**Gameplay sikli:**
1. **Tergov (oba):** Aytolun/Goncagul dalillari — Selcan yordam beradi (tavba qilgan).
2. **Ot sayohati 3 beylikka:** har birida kichik vazifa (Dodurga — Tuğtekin bilan duel-sparring; Chavdar — qaroqchilarni tozalash; Konya — saroy diplomatiyasi).
3. **Saroy dialog-dueli:** Sulton oldida Ko'pak bilan bahs — sharaf va dalil.
4. **Gumushtekinni fosh qilish — quvish/jang:** karvonsaroyda, qisqa boss.
5. **Qo'shin yig'ish:** har ittifoq = finalda qo'shimcha kuch (6.2 da ko'rinadi).

**Kerakli tizimlar:** `UInvestigationSubsystem`, `UDialogueSubsystem` (duel), `UReputationSubsystem` (beyliklar ittifoq darajasi), `UAllyForceSubsystem` (6.2 uchun kuch hisoblash), `UFastTravelSubsystem`, `ABossAIController` (Gumushtekin — qisqa).

**Level dizayni:**
- Konya saroyi: feruza koshinlar, hovuz, ustunlar — Halabdan farqli "Saljuqiy" estetikasi.
- Dodurga obasi: Qayıdan kattaroq, tosh poydevorli chodirlar.
- Yo'llar: qish oxiri, qor eriydi — bahor boshlanishi (qayta tug'ilish motivi).

---

### Kvest 6.2 — Mo'g'ul lageriga tungi reyd
**Maqsad:** Birlashgan kuch bilan Noyan lageriga tungi hujum; qamal qurollarini yoqish.

**Cutscene / Hikoya:**
Hujum oldi — Ertuğrul alplarga nutq: "Biz o'ch uchun emas, yurt uchun kelganmiz." Ibn Arabiy duosi (ruhiy). Tun, lager. Ertuğrul kichik guruh bilan yashirin kiradi (4.3 yo'li teskarisi — o'yinchi shu lagerni taniydi!). Qamal qurollari yonganda — signal, Ertokush va Tuğtekin otliqlari hujum qiladi. Tangut bilan duel — Tangut o'ladi (mix uchun birinchi javob). Noyan chodiridan chiqadi: "Ertuğrul!"

**Gameplay sikli:**
1. **Stealth kirish (4 kishi):** 4.3 xaritasi — endi to'liq qurolli; o'yinchi "qafas" joyidan o'tadi (emotsional eslash).
2. **Qamal qurollarini yoqish:** 3 katapult + 2 taran — mash'ala bilan, soqchilarni yashirin olib tashlash; 2-tasidan keyin signal (tanlov: hammasini yashirin yoqish = bonus).
3. **Katta jang:** ittifoqchilar soniga qarab dushman/ do'st nisbati. Zonalar: otxona, shomon maydoni, Noyan chodiri.
4. **Otda jang segmenti:** ot chaqirib, yonayotgan lager ichida Keshiklarni quvish.
5. **Mini-boss: Tangut** — ikki qo'lli qilich, mixchi — o'yinchi G'azab zarbasi bilan sindiradi. Finisher: Ertuğrul Tangut qo'lidan mixni oladi (6.3 uchun).
6. **Hamza** — tavba qilgan bo'lsa, Ertuğrulni o'q zarbasidan himoya qilib o'ladi; yo'qsa — jangda dushman.

**Kerakli tizimlar:** `UStealthComponent`, `USabotageObjectiveComponent` (qurollarni yoqish), `UFireSpreadSubsystem` (olov tarqalishi vizual/hazard), `ULargeBattleSubsystem` (`UAllyForceSubsystem` bilan), `UMountedCombatComponent`, `ABossAIController` (Tangut), `UChoiceConsequenceSubsystem` (Hamza).

**Level dizayni:**
- Lager 4.3 bilan bir xil, lekin kengaytirilgan: qamal qurollari maydoni, ot qo'ralari, Noyan chodiri tepalikda.
- Tun → lager yonishi bilan to'q sariq yorug'lik ortib boradi; tutun ko'rishni cheklaydi (stealth va xaos).
- Dushman: stealth fazada 20 soqchi, jangda 60+ (LOD), Tangut shomon maydonida.

---

### Kvest 6.3 — Final boss: Bayju Noyan
**Maqsad:** Noyanni mag'lub etish; qasos — Noyan qo'lini ustunga mixlash.

**Cutscene / Hikoya:**
Shomon ustuni oldida — Ertuğrul mixlangan joy. Noyan: "Sen o'lishing kerak edi." Ertuğrul: "Men o'ldim. Qaytganim — Xudoning irodasi." Duel. 3-faza oxirida Noyan tiz cho'kadi. Ertuğrul Tangutdan olgan mixni chiqaradi. **O'yinchi tanlovi (sharaf):**
- *Mixlash* — "Qo'ling endi turk qoniga tegmaydi" — kanonik (serial) yakun; Noyan tirik qoladi (3-mavsum uchun).
- *Mixlamasdan qoldirish* — "Men sen emasman" — alternativ, sharaf maksimal, lekin Noyan qasam ichadi.
Ikkala holda Noyan tirik qoladi (tarixiy/serial uzluksizlik). Epilog: bahor, Qayı oba g'arbga ko'chadi; Ertuğrul va Halima nikohi; Hayme Ana: "Endi sen Beysan." Sungurtekin kuzatadi. Konya — Ko'pak soyada. Titr.

**Boss fazalari:**
- **1-faza — Noyan ot ustida:** arena chetida aylanadi, nayza va kamon; o'yinchi otda yoki piyoda — otni yiqitish kerak (nayza uloqtirish / otning oldiga chiqib dodge). Lager yonayapti.
- **2-faza — Egri qilich va qamchi:** tez, qamchi bilan masofadan tortadi (grab), "g'azab" zarbalar. O'yinchi parry-stun jazosi bor — vaqtni to'g'ri tanlash. Iymon shkalasi — G'azab zarbasi Noyan qalqonini yoradi.
- **3-faza — Qo'l jangi / qonli duel:** ikkisi ham yarador, sekin og'ir zarbalar, har zarba katta zarar. Kamera yaqin. Olov va tutun. Noyan "mix" haqida masxara qiladi — o'yinchi "Iymon" to'lganda maxsus finisher.
- **Finisher QTE:** Ertuğrul Noyanni ustunga itaradi → tanlov → mix urish QTE (3 zarba — temirchilik ritmi bilan bir xil ritm: "Yo Alloh, Yo Qodir, Yo Qaviy" — ovoz qaytadi) yoki mixni tashlash.

**Kerakli tizimlar:** `ABossAIController` (Noyan), `UBossPhaseComponent` (otliq→piyoda o'tish), `UMountedBossComponent`, `UGrabAttackComponent` (qamchi), `UFaithMeterComponent`, `UBindingSwordComponent`, `UQTESubsystem` (ritm finisher), `UChoiceConsequenceSubsystem`, `UCutsceneDirector`, `UEpilogueSequencer`.

**Level dizayni:**
- Shomon maydoni: aylana arena (~30 m), markazda ustun (mix joyi), atrofda yonayotgan chodirlar — olov zonalari doimiy o'zgaradi (hazard).
- Tun, yong'in, qor erimoqda — qora tuproq, qizil olov, oq qor uchlik palitra.
- 1-fazada arena kengroq (ot uchun), 2-fazada chodirlar qulab arenani toraytiradi (skript), 3-fazada faqat ustun atrofidagi kichik doira.

---

# VI QISM — YON KVESTLAR VA OBA HAYOTI (qisqacha)

| Nomi | Mavsum | Beruvchi | Mazmuni |
|---|---|---|---|
| Aykız ko'z yoshlari | 1 | Aykız | Kurdoğlu Turgutni ayblaydi — dalil topish |
| Claudiusdan Omarga | 1 | Ibn Arabiy | Templar ritsarini qutqarish, e'tiqod dialoglari |
| Bo'ri ovi | 1 | Deli Demir | Oba podasini qo'riqlash, teri → zirh |
| Halab karvoni | 1 | Gündoğdu | Savdo karvonini himoya — oltin, reputatsiya |
| Eftelya izida | 1 | Afshin Bey | Josus ayolni kuzatish (stealth) |
| Selcanning sirlari | 1–2 | Selcan | Tavba yo'li — uzun munosabat chizig'i |
| Goncagul tuzog'i | 2 | Banu Chechak | Dodurgadagi fitna dalillari |
| Geyikli Baba hikmatlari | 2 | Geyikli Baba | Meditatsiya/"Iymon" maksimal shkalasini oshirish |
| Artuk Bey dorixonasi | 2 | Artuk Bey | O'simlik to'plash, crafting retseptlari |
| Dündarning birinchi qilichi | 2 | Dündar | Ukani o'rgatish — sparring minigame |
| Hamzaning vijdoni | 2 | Hamza | Xoinga ikkinchi imkon berish (Epizod 6 ta'siri) |

---

# VII QISM — DUSHMAN TURLARI (Bestiary)

**Templarlar (1-mavsum):** Oddiy ritsar (qalqon+qilich), Og'ir ritsar (gurzi), Kamonchi, Ruhoniy-ofitser (buff), Josus (stealth dushman), Bisol (mini-boss), Titus (boss).
**Halab soqchilari:** Nayzachi, Saroy gvardiyasi (o'ldirmaslik tavsiya — sharaf).
**Qaroqchilar:** zaif, ko'p sonli — ochiq olam.
**Mo'g'ullar (2-mavsum):** Otliq kamonchi, Keshik nayzachi, Lasso askar, Qalqonli piyoda, It (stealth xavfi), Shomon Ulubilge (mini-boss), Tangut (mini-boss), Bayju Noyan (boss).

---

# VIII QISM — UI/UX VA AUDIO ASOSLARI

- **HUD:** sog'lik (qizil), chidam (sariq), **Iymon** (yashil/oltin — 2-mavsumdan), ko'rinuvchanlik ko'zi, hamroh buyruq g'ildiragi.
- **Xarita:** qo'lda chizilgan XIII asr uslubi; noma'lum hududlar — tuman.
- **Kodeks:** har yangi personaj/joy → tarixiy ma'lumot (serialdan va tarixdan).
- **Musiqa:** Ertuğrul lyeytmotivi (qo'biz + nay), Templar — lotin xor, Mo'g'ul — bo'g'iz qo'shig'i + baraban; zikr sahnasi — a cappella jo'r, qatlamli.
- **Til:** O'zbek/Turk/Ingliz subtitr; dialoglar turk tilida (original his).

---

# IX QISM — ISHLAB CHIQISH BOSQICHLARI (Roadmap)

1. **Pre-production (0–3 oy):** GDD tasdiqlash, greybox Oba + o'rmon, jang prototipi.
2. **Vertical Slice (3–8 oy):** Epizod 1 to'liq (1.1–1.3), Halab bozori greybox, Titus boss prototipi.
3. **Production 1-mavsum (8–18 oy):** Epizod 2–3, oba tizimi, Halab shahri.
4. **Production 2-mavsum (18–28 oy):** yarador holat tizimi, temirchilik minigame, Mo'g'ul lageri, Noyan boss.
5. **Polish/Alpha/Beta (28–34 oy):** balans, lokalizatsiya, optimallashtirish (World Partition, Nanite, Lumen).

---

*Hujjat oxiri. Keyingi bosqich: har bir kvest uchun alohida "Quest Spec Sheet" (maqsadlar ro'yxati, trigger zonalari, dialog jadvallari) va personaj "Character Bible".*
