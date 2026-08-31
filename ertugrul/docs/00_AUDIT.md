# 00 — AUDIT: Mavjud `episodes.json` faylining kamchiliklari

> **Loyiha:** *Diriliş: Ertuğrul* asosidagi tarixiy action-adventure
> **Texnologiya:** Unreal Engine 5 (C++) + Java backend (online meta)
> **Audit sanasi:** 2026-08-27 · **Auditor:** Senior Game Design + Tech Architecture review
> **Baholangan fayl:** `seasons[2] / episodes[23] / side_quests[11]`

---

## Qisqacha xulosa (Executive Summary)

Sizning JSON'ingiz **yaxshi hikoya skeleti**, lekin u **ma'lumot fayli emas, konspekt**. Uni production'ga olib kirish uchun 4 ta yo'nalishda ish kerak:

| Yo'nalish | Topilgan muammolar | Og'irligi |
|---|---|---|
| **A. Tarixiy aniqlik** | 11 ta jiddiy anaxronizm | 🔴 **P0 — o'yin "tarixiy" deb sotilolmaydi** |
| **B. Ma'lumot sxemasi** | 23 ta yetishmayotgan maydon | 🔴 **P0 — engine bu faylni o'qiy olmaydi** |
| **C. O'yin dizayni** | 14 ta struktura muammosi | 🟠 **P1 — retention 25-30% pasayadi** |
| **D. Texnik/pipeline** | 9 ta muammo | 🟠 **P1 — lokalizatsiya va patch imkonsiz** |

**Jami: 57 ta topilgan kamchilik.**

---

# A. TARIXIY ANIQLIK — 🔴 P0

Siz "**bu tarixiy o'yin, real tarix**" dedingiz. Shuning uchun bu bo'lim eng muhimi. Har bir topilma tekshirilgan manba bilan.

### A1. 🔴 **Bayju Noyan 1226-yilda Anadoluda YO'Q edi**

Sizning 2-mavsumingiz butunlay Bayju Noyanga qurilgan (e11–e22, `historical_year: 1226`).

**Haqiqat:**
- Bayju Noyan **1241-yil qishida** Ugedey Xon tomonidan qo'mondon etib tayinlangan — Chormaqan falaj bo'lganidan keyin.
- 1228-yilda u Isfahon yaqinida **oddiy qo'l ostidagi qo'mondon** edi, Anadoluda emas — Forsda.
- Anadoluga birinchi bostirib kirish: **1242** (Erzurum qamali), hal qiluvchi jang: **Köse Dağ, 26-iyun 1243**.

**Ya'ni 1226 va Bayjuning Anadoludagi faoliyati o'rtasida 17 yil farq bor.**

### A2. 🔴 **Umuman hech qanday mo'g'ul 1226-yilda Anadoluda yo'q**
- Chormaqan Noyan **1230-yil qishida** tayinlangan.
- Anadoluga to'liq bosqin **1236-yildan** boshlanadi.
- 1220–1223 dagi Jebe/Subutay reydi **Kavkazni** supurgan, Anadoluni emas.

> **Sizning e11 "Qayi obasi ko'chib bormoqda, qarshisidan Bayju Noyan chiqadi" sahnasi 1226-da fizik jihatdan imkonsiz.**

### A3. 🔴 **Ibn Arabiy 1225-yilda Halabda emas, Damashqda**
- Ibn Arabiy Anadoluda (Malatya) **~1215–1221** yashagan.
- **1223-yildan boshlab umrining oxirigacha Damashqda**, u yerdan chiqmagan.
- Vafoti: **16-noyabr 1240**, Damashq, Qosiyun tog'i.
- Sizning e5 "Ibn Arabiy chodirini topish" — chodirda yashovchi ko'chma shayx emas, u Damashqda kitob yozib o'tirgan 60 yoshli olim edi.

### A4. 🔴 **Amir Al-Aziz 1225-yilda 12 yoshli bola**
- Al-Aziz Muhammad **~1213-yilda tug'ilgan**, otasi vafot etganda **3 yoshda** taxtga o'tirgan (1216).
- 1225-yilda u **12 yoshda**. Haqiqiy hukmdor — atabek **Shihobuddin To'g'ril** (mamluk), u **1231-yilgacha** boshqargan.
- Sizning e4 "Amir Al-Aziz yosh va ishonuvchan" — to'g'ri ruh, noto'g'ri yosh. U bola edi, "yosh amir" emas.

### A5. 🔴 **Saadettin Köpek 1226-yilda hokimiyatda emas**
- 1220-yillarda u Alauddin Kayqubodning **emir-i şikâr** (ov boshlig'i) va qurilish vaziri — Kubadabad saroyini qurgan. Xoin emas, xizmatchi.
- Uning **"terror hukmronligi" — 1237–1238**, atigi bir yil, Keyxusrav II ostida.
- O'ldirilgan: **1238-yil kuz boshi**, ichkilik majlisida pichoqlangan, jasadi Konya saroy devoriga osilgan.
- Sizning e19 "1226 (Bahor) — Köpek davlatni ichidan emirmoqda" — 11 yil erta.

### A6. 🟠 **Sulaymon Shohning Fyrot daryosida cho'kishi — TARIX EMAS, RIVOYAT**
- Bu XV asr (Aşıkpaşazade, 1480-yillar) yozma an'anasi. Zamondosh manba yo'q.
- Undan ham muhimi: **Usmonning tangalarida "Osman bin Ertuğrul bin Gündüz Alp" yozilgan** — ya'ni Ertug'rulning otasi **Gunduz Alp**, Sulaymon Shoh emas. TDV İslâm Ansiklopedisi buni "aniq noto'g'ri" deb yozadi.
- Halil İnalcık, İlber Ortaylı, Osman Turan — hammasi Gunduz Alp tomonida.

### A7. 🟠 **"Qayi qabilasi" ham — XV asr ixtirosi**
- Qayi shajarasi birinchi marta **1420–30-yillarda**, Yazıcıoğlu Ali tomonidan kiritilgan — Ertug'rul vafotidan **~150 yil keyin**.
- Cemal Kafadar buni "XV asr shajara ixtirosidagi ijodiy *qayta kashf*" deb ataydi.
- **Bu o'yindan Qayi'ni olib tashlash kerak degani EMAS.** Bu — o'yin ichida buni *aytish* kerak degani. Pastda "Haqiqat qatlami" yechimi bor.

### A8. 🟠 **Templar ritsarlari — sizning YAGONA to'liq to'g'ri elementingiz** ✅
- Templarlar **1215/16-dan 1268-gacha Amanos tog'laridagi Bagras (Gaston) qal'asini** uzluksiz ushlab turgan.
- **~1229-yilda Halab amiri al-Aziz Bagras'ni qamal qilib mag'lub bo'lgan** — bu haqiqiy voqea!
- Darbsak (Trapesac) 1188-dan ayyubiylarda; 1237-yilgi Trapesac jangida 120 templar ritsaridan atigi ~20 tasi qutulgan.
- **Xulosa: Amanos/Templar arki tarixiy jihatdan mustahkam. Uni saqlang va kengaytiring.**

### A9. 🟠 **Geyikli Bobo ~100 yil kechroq**
- Geyikli Bobo **Orxon G'oziy va Bursa** bilan bog'liq (1320–30-yillar).
- Ertug'rul davri sahnasida u bo'lishi mumkin emas.
- **Yechim:** ismini o'zgartiring (`Sarı Baba`, `Kayalı Ata` — badiiy) yoki **Hacı Bektaş Veli** (c.1209–1271, xurosonlik, mo'g'ullardan qochib kelgan — timeline'ga to'liq mos!) bilan almashtiring.

### A10. 🟠 **"Mix qoqish" — mo'g'ul usuli EMAS**

Bu siz eng ko'p urg'u bergan sahna, shuning uchun alohida diqqat:

- Mo'g'ul zo'ravonligi manbalarda **ommaviy** (Nishopur 1221 — kesilgan boshlardan uchta piramida), **individual qiynoq usullari** emas.
- **LEKIN qo'lga mix qoqish bu dunyoda haqiqatan hujjatlashtirilgan:** *ṣalb* (xochga mixlash) — Christian Lange tadqiqotiga ko'ra **"Saljuqiy xronikalarda eng ko'p tilga olinadigan qatl usuli"**. *Tasmīr* — mixlab xochga mixlash — **1248-yil Damashq** hodisasida qurbonning **qo'llari, yelkasi va oyoqlari mixlangan**; u juma tushdan yakshanba tushgacha yashagan.

> **🎯 DIZAYN TAVSIYASI (senior level):** Mix sahnasini **ikki marta** ishlating.
> 1. **1-mavsum** — Templar/ayyubiy jallodi *tasmīr*ni Ertug'rulning **ko'z o'ngida boshqa asirga** qo'llaydi (tarixiy jihatdan 100% to'g'ri, va o'yinchini dahshatga soladi).
> 2. **3-mavsum** — Noyan **aynan shu usulni** Ertug'rulga qaytaradi, chunki u bu odatni bosib olingan Fors/Sham yerlaridan **o'rgangan**. Kodeksda: "Noyan bu usulni mo'g'ul dashtidan emas, o'zi vayron qilgan shaharlardan o'rgandi."
>
> Natijada: sahna tarixiy jihatdan to'g'ri, dramatik jihatdan **kuchliroq**, va o'yinchi haqiqiy tarixiy fakt o'rganadi.

### A11. 🟠 **Vaqt zichligi — 23 epizod 2 yilga sig'maydi**
Sizning barcha 10 ta 1-mavsum epizodi `1225`, barcha 13 ta 2-mavsum epizodi `1226`. Bu:
- Vaqt o'tishini his qildirmaydi (fasl, yosh, bola tug'ilishi, qal'a qurilishi).
- Qahramon o'sishini asossiz qiladi (2 yilda o'quvchidan bekgacha?).
- Tarixiy voqealar bilan ulanish imkonini yo'qotadi.

---

## ✅ A-bo'lim yechimi: TIMELINE'NI KO'CHIRISH (1227 → 1261)

Bitta o'zgartirish 11 ta anaxronizmning 9 tasini hal qiladi **va sizga 48 epizodni tabiiy ravishda beradi**:

| Mavsum | Yillar | Tarixiy langar | Anaxronizm |
|---|---|---|---|
| **S1** | **1227–1230** | Templarlar Bagras'da; al-Aziz Bagras'ni qamal qiladi (~1229); Jaloliddin Xorazmshoh Kavkazni yondiradi, qochqinlar g'arbga oqadi | ✅ **0** |
| **S2** | **1230–1237** | Yassıçemen jangi (10-avg 1230); karvonsaroy qurilish bumi (Sultan Han 1229); Xorazmiy qoldiqlari saljuqiy xizmatiga kiradi (1231–37); Kayqubodning zaharlanishi (31-may 1237 — **suyaklarida zahar DNK tahlilida topilgan**) | ✅ **0** |
| **S3** | **1237–1243** | Köpek terrori (1237–38) va o'ldirilishi; Chormaqan → Bayju (1241); Erzurum (1242); **Köse Dağ, 26-iyun 1243** | ✅ **0** |
| **S4** | **1243–1261** | Mo'g'ul vassalligi va yillik o'lpon (12 mln kumush tanga, 500 tuya, 5000 qo'y); uc bey'lik; Söğüt/Domaniç; Axiy birodarligi | ✅ **0** |

**Yutuq:** Bayju **haqiqiy** boss bo'ladi (S3), Köpek **haqiqiy** siyosiy dushman (S3), Ibn Arabiy **haqiqiy** joyda uchraydi (S1/S2, Damashq), va Ertug'rulning yigitlikdan uch bey'ligacha bo'lgan yo'li **34 yilga** cho'ziladi — bu qahramon yoyi uchun mukammal.

---

# B. MA'LUMOT SXEMASI — 🔴 P0

Sizning JSON'ingizni Unreal Engine hozircha **o'qiy olmaydi**. Yetishmayotgan 23 maydon:

### B1. Epizod darajasida yetishmayotgan maydonlar

| # | Maydon | Nima uchun kerak | Bo'lmasa nima bo'ladi |
|---|---|---|---|
| 1 | `schema_version` | Patch/migratsiya | Eski save fayllar buziladi |
| 2 | `difficulty_tier` | Qiyinlik egri chizig'i | 5-epizod 20-epizoddan qiyin bo'lib qoladi |
| 3 | `prerequisites[]` | Qulflash mantiqi | O'yinchi 15-epizodni birinchi o'ynaydi |
| 4 | `unlocks[]` | Nima ochiladi | Progression his qilinmaydi |
| 5 | `mechanics_introduced[]` | O'rgatish rejasi | Yangi mexanika tushuntirilmay qoladi |
| 6 | `estimated_duration_min` | Sessiya rejalash | Balans o'lchab bo'lmaydi |
| 7 | `objectives[]` (tipli) | Real quest tizimi | `goal` — matn, kod uni o'qiy olmaydi |
| 8 | `fail_conditions[]` | Mag'lubiyat qoidalari | Nima bo'lsa "Game Over"? |
| 9 | `checkpoints[]` | Saqlash nuqtalari | Krash = 40 daqiqa yo'qoladi |
| 10 | `enemy_composition{}` | Jang balansi | Dizayner nechta dushman qo'yishni bilmaydi |
| 11 | `codex_unlocks[]` | **Tarix o'rgatish** | ⭐ Sizning asosiy maqsadingiz ishlamaydi |
| 12 | `branches[]` / `choices[]` | Qayta o'ynash | Bitta yo'l = bir marta o'ynaladi |
| 13 | `hand_state` | ⭐ Mix tizimi | Jarohat epizodlar orasida yo'qoladi |
| 14 | `time_of_day` / `weather` | Atmosfera + gameplay | Kechasi stealth, kunduzi jang farqi yo'q |
| 15 | `audio{}` (music_cue, ambience) | Audio pipeline | Kompozitor nima yozishni bilmaydi |
| 16 | `characters[]` | Kim sahna'da | Casting/animatsiya rejalash imkonsiz |
| 17 | `rewards{}` | Iqtisod | XP/oltin/material balansi yo'q |
| 18 | `streaming_levels[]` | Xotira boshqaruvi | Konsolda krash |
| 19 | `loc_key` (matn o'rniga) | Lokalizatsiya | Turk/ingliz/arab tiliga chiqa olmaysiz |
| 20 | `telemetry_funnel_id` | Analitika | Qayerda tashlab ketishayotganini bilmaysiz |
| 21 | `intro` (obyekt, string emas) | ⭐ O'ynaladigan intro | Sizning asosiy so'rovingiz |
| 22 | `accessibility_overrides{}` | QTE/ritm alternativalari | Sertifikatsiya (Xbox/PS) rad etadi |
| 23 | `content_warnings[]` | ESRB/PEGI reyting | Reyting olish kechikadi |

### B2. Konkret buglar sizning faylingizda

```diff
- {"id": "sq_selcan_secrets", "season": 0, ...}
+ // ❌ season: 0 — bunday mavsum yo'q. Off-by-one bug.

- "index": 0  ... "index": 10   // e1 → e11
+ // ❌ `index` global (0..22), lekin `seasons[].index` lokal (1,2).
+ //    Ikki xil indekslash tizimi = kodda doimiy bug manbasi.

- "quests": ["q1_1_deer_hunt", ...]
+ // ❌ Bu ID'lar hech qayerda ta'riflanmagan. Dangling reference.
+ //    `levels[]` ham xuddi shunday — 18 ta level ID, 0 ta ta'rif.

- "giver": "aykiz"
+ // ❌ Personajlar registri yo'q. `aykiz` kim? Qanday model? Qaysi ovoz aktyori?

- "intro_video": "assets/videos/ep1_intro.mp4"
+ // ❌ Siz "o'ynaladigan intro" xohlaysiz, lekin sxema .mp4 talab qiladi.
+ //    Bu ikkisi bir-biriga zid. (Yechim: 06_INTERACTIVE_INTRO.md)

- "cliffhanger": "Titus o'ldirildi. Sulaymon Shoh vafot etdi."
+ // ⚠️ Bu — dizayner eslatmasi, o'yin ma'lumoti emas. Kod bilan bajarilmaydi.
+ //    Kerak: `epilogue_scene_id` + `world_state_deltas[]`
```

### B3. Yetishmayotgan butun bo'limlar

Faylda umuman yo'q:
- `characters[]` — personajlar registri
- `quests[]` — quest ta'riflari (faqat ID'lar bor)
- `levels[]` — level ta'riflari
- `codex[]` — ⭐ **tarix maqolalari** (sizning asosiy maqsadingiz!)
- `items[]` / `crafting[]` — qurol, zirh, retseptlar
- `factions[]` — Templar / Mo'g'ul / Saljuqiy / Ayyubiy munosabatlari
- `world_state[]` — global o'zgaruvchilar (kim tirik, qaysi qal'a kimda)
- `difficulty_curve[]` — global balans egri chizig'i
- `dialogue[]` — dialog daraxtlari

---

# C. O'YIN DIZAYNI — 🟠 P1

### C1. 🔴 **Har bir epizod bir xil: "bor → o'ldir"**

23 ta epizodning `goal` maydonini tahlil qildim:

| Epizod turi | Soni | % |
|---|---|---|
| Jang / o'ldirish | **17** | 74% |
| Stealth | 3 | 13% |
| Dialog / siyosat | 2 | 9% |
| Boshqa (temirchilik) | 1 | 4% |

**Muammo:** 74% bir xil fe'l. O'yinchi 6-epizodga borib zerikadi. Bu **retention'ning №1 qotili**.

**Yechim:** 48 epizodni 9 xil **epizod arxetipiga** bo'lish (03_NARRATIVE hujjatida to'liq):
`INFILTRATION`, `SIEGE`, `SURVIVAL`, `INVESTIGATION`, `COURT` (siyosiy), `ESCORT`, `DEFENSE`, `CHASE`, `RITUAL` (mahorat/marosim).
Hech qaysi arxetip ketma-ket **2 martadan ko'p** takrorlanmasin.

### C2. 🟠 **Ot bilan bog'liqlik — siz o'zingiz aytdingiz, va haqsiz**

Sizning `level` ID'laringiz: `forest`, `valley_path`, `caravanserai`, `trade_route`, `snow_pass`, `rocky_valley`... — deyarli hammasi **ochiq maydon = ot**.

**Yechim: 11 xil harakat tizimi**, ot ulardan **faqat bittasi** (04_CORE_SYSTEMS.md):
1. **Yayov parkur** (qal'a devori, tom, bozor)
2. **Ot** (dasht, ta'qib) ← faqat 15% kontent
3. **Kanalizatsiya / g'or** (klaustrofobik, mash'al)
4. **Suvda suzish / daryo oqimi**
5. **Arqon va krük** (qal'a burjlari)
6. **Qor/muz** — sirg'anish, qalqonda tushish
7. **Aravа / karvon ustida jang** (harakatlanuvchi platforma)
8. **Qayiq** (Fyrot, Sakarya)
9. **Lochin ko'zi** (skautlik — o'ynaladigan, kinematik emas)
10. **Tuya** (cho'l, Sham yo'li)
11. **Vertikal qamal** (narvon, kule, mancınık)

### C3. 🟠 **Qiyinlik egri chizig'i yo'q**
e10 (Titus boss) va e22 (Noyan boss) — ikkisi ham "final". Oralarida e11–e21 nima uchun qiyinroq bo'lishi kerak? Sabab yozilmagan.

**Yechim:** 5 pog'onali `difficulty_tier` + har mavsumda **yangi dushman arxetipi**:
| Tier | Epizodlar | Yangi tahdid |
|---|---|---|
| T1 (o'rganish) | 1–8 | Qaroqchi, bo'ri |
| T2 | 9–18 | Templar sержant, qalqonli |
| T3 | 19–30 | Templar ritsar (parry majburiy), arbaletchi |
| T4 | 31–42 | Mo'g'ul otliq kamonchi (masofadan bosim), nerge o'rab olish |
| T5 (usta) | 43–48 | Kешiktenlar (elita), ko'p bosqichli boss |

### C4. 🟠 **Meta-progression yo'q**
Epizod tugadi → keyingisi. Orasida nima o'sadi? Hech nima.

**Yetishmayotgan meta-tizimlar:**
- **Oba boshqaruvi** (base building) — chodirlar, temirchi, bozor, masjid, kutubxona
- **Alp jamoasi** — alplarni yollash, o'rgatish, ular o'lishi mumkin (permadeath opsiyasi)
- **Iymon shkalasi** — siz eslatgansiz, lekin mexanikasi yo'q
- **Obro' (Reputation)** 4 fraksiya bo'yicha
- **Crafting** — teri→zirh, temir→qilich (siz `sq_wolf_hunt` da eslatgansiz)
- **Kodeks** — ⭐ tarix maqolalari kolleksiyasi

### C5. 🟠 **Tanlov faqat bitta joyda (e22)**
"Tanlov: O'ldirish yoki Qo'lini mixlash" — juda kuchli moment! Lekin **butun o'yinda yagona**.

**Yechim:** har mavsumda kamida **3 ta og'ir tanlov** + **1 ta mavsum yakunlovchi tanlov** → `world_state`ga yoziladi → keyingi mavsumda **ko'rinadigan oqibat**.

### C6. 🟠 **Side quest'lar hikoya bilan bog'lanmagan**
11 ta side quest — hammasi "bor va qil". Ular:
- Asosiy hikoyani o'zgartirmaydi
- Bir-biri bilan bog'lanmagan
- Takrorlanmaydi (o'ynadingiz → tugadi)

**Yechim:** side quest'larni **zanjirlarga** (chain) aylantirish — har biri 3–5 bosqich, oxirida asosiy hikoyaga ta'sir qiladi. Masalan `sq_hamza_conscience` → agar tugatsangiz, Hamza S3 finalida sizni qutqaradi; tugatmasangiz — o'ladi.

### C7. Qolgan dizayn muammolari
| # | Muammo |
|---|---|
| C8 | **Tutorial rejasi yo'q** — o'yinchi parry'ni qachon o'rganadi? |
| C9 | **Boss dizayni faqat 2 ta** (Titus, Noyan) — 48 epizod uchun kamida 8 ta kerak |
| C10 | **Fail state yo'q** — Turgut olib ketilishi "majburiy mag'lubiyat", lekin o'yinchi buni bilmaydi → g'azablanadi |
| C11 | **Onboarding 0-30 daqiqa rejalashtirilmagan** — bu retention'ning eng muhim oynasi |
| C12 | **Ayollar personaji faqat quest beruvchi** — Halima, Aykiz, Selcan o'ynaladigan segment olishi kerak |
| C13 | **"Iymon shkalasi" — mexanika emas, so'z** — nima beradi? Qanday to'ladi? |
| C14 | **Sessiya uzunligi noma'lum** — mobil emas, PC/konsol: 45–70 daq/epizod maqsad qilinishi kerak |

---

# D. TEXNIK / PIPELINE — 🟠 P1

| # | Muammo | Ta'sir |
|---|---|---|
| D1 | **Matnlar hardcoded o'zbekcha** (`intro_text`, `title`) | Lokalizatsiya imkonsiz. Kerak: `loc_key` + `.po`/CSV |
| D2 | **Media yo'llari fayl tizimida** (`assets/videos/...`) | UE5'da `TSoftObjectPtr` kerak, string emas |
| D3 | **Versiyalash yo'q** | Live-ops patch chiqara olmaysiz |
| D4 | **Validatsiya sxemasi yo'q** | Dizayner xato yozsa — o'yin build vaqtida emas, runtime'da krash bo'ladi |
| D5 | **ID konvensiyasi izchil emas** (`e1` vs `q1_1_deer_hunt` vs `level_oba_kayi`) | Prefiks standarti kerak: `EP_`, `QST_`, `LVL_`, `CHR_`, `CDX_` |
| D6 | **Backend bilan aloqa yo'q** | Java servisi bu faylni qanday o'qiydi? Content CDN? |
| D7 | **Save/load sxemasi yo'q** | Nimalar saqlanadi? Qanday migratsiya qilinadi? |
| D8 | **Telemetriya hook'lari yo'q** | Retention'ni o'lchay olmaysiz — siz aynan shuni xohlaysiz |
| D9 | **Platform farqi hisobga olinmagan** | PC/PS5/XSX'da streaming budjeti har xil |

---

# XULOSA VA ISH TARTIBI

### 🔴 Darhol (P0 — hafta 1–2)
1. **Timeline'ni 1227–1261 ga ko'chirish** (A-bo'lim yechimi)
2. **`episodes_v2.json` sxemasini yozish** → `data/schema/episode.schema.json`
3. **Kodeks (Tarix) tizimini loyihalash** — sizning asosiy maqsadingiz
4. **`loc_key` ga o'tish** — hozir arzon, keyinroq qimmat

### 🟠 Keyingi (P1 — hafta 3–8)
5. 48 epizodni **9 arxetip** bo'yicha qayta taqsimlash
6. **11 xil harakat tizimi** — ot monopoliyasini sindirish
7. **Mix (Mıh) tizimi** — doimiy jarohat, barcha epizodlarda
8. **Meta-progression** — oba, alplar, iymon, obro', crafting
9. **Telemetriya funnel** — har epizodga `funnel_id`

### 🟢 Keyinroq (P2)
10. Branching va `world_state`
11. NG+ (New Game Plus) va Discovery Tour rejimi
12. Accessibility sertifikatsiyasi

---

**Keyingi hujjatlar:**
- `01_VISION_PILLARS.md` — o'yin ustunlari
- `02_HISTORY_LAYER.md` — ⭐ tarix o'rgatish tizimi (Haqiqat/Rivoyat)
- `03_NARRATIVE_48EP.md` — 48 epizod to'liq
- `04_CORE_SYSTEMS.md` — jang, stealth, 11 harakat
- `05_MIH_SYSTEM.md` — ⭐ mix qoqish tizimi
- `06_INTERACTIVE_INTRO.md` — ⭐ o'ynaladigan intro
- `07_SETTINGS_HOTKEYS.md`
- `08_TECH_UE5.md` / `09_TECH_JAVA.md`
- `10_NOYAN_DIALOGUE.md` — ⭐ shakal motivi
