# DİRİLİŞ: THE LAST MARCH
### Game Design Document + Texnik arxitektura · v2

> XIII asr Anadolu chegarasi · 48 epizod · 4 mavsum · **1227–1261**
> Unreal Engine 5 (C++) + Java backend (online meta-servislar)

---

## Bu nima

Sizning `episodes.json` faylingiz (2 mavsum, 23 epizod) auditdan o'tkazildi, tarixiy jihatdan tuzatildi va **48 epizodli to'liq production hujjatiga** kengaytirildi.

**Asosiy o'zgarish:** timeline **1225–1226 → 1227–1261** ga ko'chirildi. Bitta bu qaror 11 ta anaxronizmning 9 tasini hal qiladi va 48 epizodni tabiiy ravishda beradi.

---

## Fayl tuzilishi

```
ertugrul/
├── docs/                                  ← GAME DESIGN DOCUMENT
│   ├── 00_AUDIT.md                        57 ta kamchilik, P0/P1 bo'yicha
│   ├── 01_VISION_PILLARS.md               5 dizayn ustuni, anti-ustunlar, KPI
│   ├── 02_HISTORY_LAYER.md              ⭐ Haqiqat/Rivoyat, Bilge Göz, Kodeks
│   ├── 03A_NARRATIVE_S1_S2.md             EP001–EP024 to'liq
│   ├── 03B_NARRATIVE_S3_S4.md             EP025–EP048 to'liq
│   ├── 04_CORE_SYSTEMS.md                 Jang, stealth, 11 harakat, iqtisod
│   ├── 05_MIH_SYSTEM.md                 ⭐ Mix — 48 epizodda 48 ko'rinish
│   ├── 06_INTERACTIVE_INTRO.md          ⭐ O'ynaladigan intro (.mp4 emas)
│   ├── 07_SETTINGS_HOTKEYS.md             Kirish oqimi, sozlamalar, hotkeys
│   ├── 08_TECH_UE5.md                     9 modul, validator, Mass, CI/CD
│   └── 10_NOYAN_DIALOGUE.md             ⭐ 67 replika, «shakal» motivi
│
├── data/                                  ← PRODUCTION MA'LUMOT
│   ├── episodes_v2.json                   48 epizod, 165 KB
│   ├── schema/episode.schema.json         JSON Schema draft 2020-12
│   └── build_episodes.py                  Generator + 14 CI tekshiruvi
│
├── ue5/                                   ← UNREAL ENGINE 5 (C++)
│   ├── Source/ErtugrulGameplay/…/ErtWoundComponent.h  ⭐ Mix tizimi
│   ├── Source/ErtugrulGameplay/…/ErtWoundComponent.cpp
│   ├── Source/ErtugrulNarrative/…/ErtIntroDirector.h  ⭐ O'ynaladigan intro
│   └── Config/DefaultGameplayTags.ini     Izohlangan tag ierarxiyasi
│
├── backend/                               ← JAVA (Spring Boot 3.3, 101 fayl)
│   ├── README.md                          O'zbekcha — arxitektura, endpointlar
│   ├── pom.xml · docker-compose.yml
│   └── src/main/java/com/fayzinc/ertugrul/…
│
└── gdd.html                               Vizual GDD (Artifact sifatida chop etilgan)
```

---

## Tez boshlash

### Ma'lumotni qurish va tekshirish
```bash
cd data
python3 build_episodes.py            # quradi + 14 ta tekshiruvni bajaradi
python3 build_episodes.py --check    # faqat tekshiradi (CI uchun)
```

Tekshiruvlar (xato bo'lsa `exit 1` → build to'xtaydi):

| # | Tekshiruv |
|---|---|
| 1 | 48 epizod mavjudmi |
| 2 | ID va indeks izchilligi *(sizning `"season": 0` bug'ingiz)* |
| 3 | Sanalar xronologik o'sib boradimi |
| 4 | ⭐ **Har epizodda `mih_beat` bormi** — sizning talabingiz |
| 5 | ⭐ `max_integrity` monotonik pasayadimi (faqat EP043 da ko'tarilish) |
| 6 | Arxetip 3 marta ketma-ket takrorlanmaydimi |
| 7 | ⭐ **Ot 12 epizoddan ko'pida asosiy emasmi** — sizning talabingiz |
| 8 | `max_simultaneous ≤ 8` (o'lim tuzog'i) |
| 9 | Qiynoq sahnasi bo'lsa kontent ogohlantirishi bormi |
| 10 | BAHSLI/RIVOYAT uchun `scholar_note` bormi |
| 11 | `intro_video` maydoni yo'qmi (o'ynaladigan intro) |
| 12 | Shubha sahnalari epizodlarga bog'langanmi |
| 13 | Side-quest zanjirlari xronologikmi |
| 14 | Dangling `prerequisites` / `unlocks` yo'qmi |

### JSON Schema validatsiyasi
```bash
pip install jsonschema --break-system-packages
python3 -c "
import json; from jsonschema import Draft202012Validator
Draft202012Validator(json.load(open('data/schema/episode.schema.json'))) \
  .validate(json.load(open('data/episodes_v2.json')))
print('OK')"
```

### Backend
```bash
cd backend && docker compose up -d      # postgres, redis, kafka, minio
./mvnw spring-boot:run                  # http://localhost:8080/swagger-ui.html
```

---

## Asosiy qarorlar (nima uchun shunday)

| Qaror | Sabab |
|---|---|
| **Timeline 1227–1261** | Bayju Noyan 1241-da tayinlangan; Köse Dağ 1243; Köpek terrori 1237–38; Ibn Arabiy 1223-dan Damashqda. 9 ta anaxronizm hal bo'ldi. |
| **Mixni saljuqiy jallodi qoqadi** | *Ṣalb/tasmīr* — saljuqiy xronikalarida eng ko'p tilga olinadigan qatl usuli (Lange). Mo'g'ullarda bunday individual usul hujjatlashtirilmagan. Noyan uni keyin **o'rganadi** — dramatik jihatdan kuchliroq. |
| **Ot 15%** | Siz aytdingiz. 11 xil harakat tizimi qurildi; CI testi 12 dan oshmasligini tekshiradi. |
| **`.mp4` intro olib tashlandi** | Sizning "o'yinchi o'zi harakatlanadi" talabingiz bilan zid edi. O'ynaladigan intro ~$820K arzonroq va 85% o'rniga 8% skip qilinadi. |
| **Haqiqat/Rivoyat ikki qatlami** | Serial auditoriyasini ham, tarixchilarni ham saqlaydi. Hech kim xafa bo'lmaydi, va bu ta'lim bozorini ochadi. |
| **Köpek avval do'st** | Antagonistni dushman sifatida tanishtirish — arzon. Uni yoqimli do'st sifatida tanishtirib, 10 epizoddan keyin xiyonatga uchratish — esda qoladi. |
| **Noyan — buxgalter, zolim emas** | Mo'g'ul imperiyasi **davlat** edi: yozuv, pochta, soliq, diniy erkinlik. Befarqlik nafratdan qo'rqinchliroq. |
| **Single-player + online meta** | Co-op/PvP hikoya sifatini pasaytiradi. Backend faqat: cloud save, kodeks sinxroni, telemetriya, live-ops balans. O'yin **to'liq offline ishlaydi**. |

---

## Eng katta risklar

| Risk | Ta'sir | Yumshatish |
|---|---|---|
| 🔴 **Ikkita jang animseti** (o'ng + chap qo'l) | 6–8 kun mocap, ~1.1 GB xotira, 2× QA | **Prototipda hal qiling.** Mirror qilmang — o'yinchi sezadi. Chap qo'lli qilichboz bilan alohida mocap. |
| 🟠 EP025–027 "jazо" hissi | O'yinchi bir qo'lni jazо deb his qilsa — ketadi | EP026 oxirida chap qo'l **kuchliroq** bo'lishi kerak: yangi takedown, tepish, +20% tezlik |
| 🟠 EP024 kontent og'irligi | Reyting, ba'zi bozorlarda taqiq | 3 pog'onali qiynoq sozlamasi + majburiy kontent ogohlantirishi |
| 🟠 Tarixiy bahs | Siyosiy reaksiya | Shubha sahnalari + Kodeks: o'yin hech kimni ayblamaydi, faqat manbalarni ko'rsatadi |
| 🟡 48 epizod ko'lami | Production hajmi | Hub + kengaytirilgan levellar (ochiq dunyo **emas**). ~63 odam-oy muhandislik. |

---

## Tarixiy manbalar

Barcha sanalar va da'volar tekshirilgan:

- **TDV İslâm Ansiklopedisi** — *Ertuğrul Gazi*, *Söğüt*, *Ahî Evran*
- **Cemal Kafadar**, *Between Two Worlds* (1995) — Qayi shajarasi tanqidi
- **Halil İnalcık**, **İlber Ortaylı**, **Osman Turan** — Gunduz Alp masalasi
- **Sara Nur Yıldız** (2013) — Sa'd al-Din Köpek terrori, 1237–38
- **Christian Lange**, *Torture and Public Executions in the Islamic Middle Period* — *ṣalb* / *tasmīr*
- **Ibn Bibi** an'anasi — saljuqiy saroy tarixi
- Usmon tangalari (Yenişehir-Bursa) — *«Osman bin Ertuğrul bin Gündüz Alp»*

> Ertug'rul haqida bizga qolgan **yagona ishonchli dalil** — o'g'li Usmonning tangasi. Vizantiya tarixchilari (Pachymeres, Gregoras) va XIV asr islom mualliflari (al-Umariy, Ibn Battuta) uni umuman tilga olmaydi.
>
> O'yin bu haqiqatni yashirmaydi — **u shu haqiqat bilan tugaydi.**

---

*FayzInc · 2026*
