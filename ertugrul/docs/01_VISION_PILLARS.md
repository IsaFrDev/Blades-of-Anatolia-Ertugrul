# 01 — VISION & DESIGN PILLARS

## Bir jumlada (Elevator Pitch)

> **"DİRİLİŞ: THE LAST MARCH"** — XIII asr Anadolu chegarasida, mo'g'ul dahshati va salibchi qal'alari orasida qolgan bir turkman jangchisi qanday qilib **bitta qo'l bilan** davlat asos soladi.

## Uch jumlada

*1227-yil. Xorazm quladi, dashtdan qochqinlar oqib keladi. Amanos tog'larida templar ritsarlari qal'alarda o'tiribdi, Halabda 14 yoshli amir nomiga hukmronlik qiladi, Konyada Sulton Alauddin imperiya quradi — va sharqdan bir qora bulut yaqinlashadi.*

*Siz — Ertug'rul. Sizning qabilangizning nomi tarixda yozilmagan. Sizning otangizning ismi tanga ustida chalkash. Sizga hech kim davlat va'da qilmagan.*

*Sizga faqat bitta narsa berilgan: chegara. Va bir kun kelib **o'ng qo'lingizga mix qoqiladi** — va siz qilichni chap qo'lingizga olishni o'rganishingiz kerak bo'ladi.*

---

## 5 ta DIZAYN USTUNI (Design Pillars)

Har bir dizayn qarori shu 5 ustunga tekshiriladi. Ustunga xizmat qilmaydigan feature — **kesiladi**.

---

### 🏛️ USTUN 1 — «TARIX O'RGATADI, VA'Z QILMAYDI»
> *"Tarix — o'yinning fon rasmi emas, o'yinning mexanikasi."*

**Ma'nosi:**
- O'yinchi tarixni **matn o'qib emas, o'ynab** o'rganadi. Karvonsaroyda tunash — savdo tizimini o'rgatadi. Tanga sanash — Saljuqiy iqtisodini o'rgatadi. Kamon torini yomg'irda himoya qilish — kompozit kamon texnologiyasini o'rgatadi.
- Har bir epizod **1–3 ta `codex_unlock`** beradi va ular **avtomatik ochilmaydi** — o'yinchi biror joyga borsa, biror narsani ko'rsa, biror odam bilan gaplashsa ochiladi.
- **Rivoyat va Haqiqat ajratiladi** (pastda batafsil). O'yin hech qachon afsonani fakt deb ko'rsatmaydi.

**Tekshiruv savoli:** *Bu feature o'yinchiga XIII asr haqida biror haqiqiy narsa o'rgatadimi?*

**Anti-namuna:** ❌ "Ertug'rul 1226-yilda Bayju Noyanni yengdi" — bu yolg'on va o'yinchini aldash.
**Namuna:** ✅ "Bagras qal'asini 1229-yilda Halab amiri qamal qildi va mag'lub bo'ldi" — bu haqiqat, va siz uni **o'ynaysiz**.

---

### 🩸 USTUN 2 — «TANA ESLAB QOLADI» (The Body Remembers)
> *"Jarohat — sog'ayib ketadigan HP emas, o'zingiz bilan olib yuradigan tarix."*

**Ma'nosi:**
- **Mix (Mıh) tizimi** — o'yinning yuragi. 24-epizodda o'ng qo'lga qoqilgan mix **qolgan 24 epizodda ham** ta'sir qiladi.
- Sovuqda qo'l qotadi. Yomg'irda parry oynasi qisqaradi. Uzoq jangdan keyin qo'l titraydi. Kamon tortish og'riydi.
- Har bir jiddiy jarohat `PermanentScar` yaratadi — vizual + mexanik.
- **Bu qiyinlik uchun emas, ma'no uchun.** O'yinchi "menda mix bor" degan hissiyot bilan yashaydi.

**Tekshiruv savoli:** *Bu feature o'yinchi tanasini his qildiradimi, yoki faqat raqamni o'zgartiradimi?*

---

### 🐺 USTUN 3 — «CHEGARA ODAMI» (Man of the March)
> *"Siz sulton emassiz. Siz — ikki imperiya orasidagi 400 chodir."*

**Ma'nosi:**
- Siz **hech qachon** eng kuchli tomon emassiz. Har bir g'alaba — ayyorlik, tanlov va qurbonlik natijasi.
- To'g'ridan-to'g'ri jang **oxirgi chora**. Diplomatiya, savdo, josuslik, nikoh siyosati — bularning barchasi haqiqiy mexanika.
- **4 fraksiya** (Saljuqiy / Ayyubiy / Templar / Mo'g'ul) + 6 kichik guruh. Ular **bir-biri bilan ham urushadi** — siz muvozanatni ishlatasiz.
- Obro' (reputation) bir tomonda oshsa, boshqasida tushadi. Neytral qololmaysiz.

**Tekshiruv savoli:** *O'yinchi bu vaziyatni kuch bilan hal qila oladimi? Agar ha — dizayn xato.*

---

### ⚔️ USTUN 4 — «HAR ZARBA — QAROR» (Weight over Speed)
> *"Bu Dynasty Warriors emas. Uch kishi sizni o'ldirishi mumkin."*

**Ma'nosi:**
- Jang **og'ir, aniq, jazolovchi**. Stamina (nafas) bor. Qurollar sinadi, o'tmaslashadi.
- **Parry oynasi 180ms** (Mix'dan keyin 120ms). Dodge — i-frame emas, **masofa**.
- 5 ta dushman = o'lim. Yechim: pozitsiya, tor joy, muhit, alplar.
- **Assassin's Creed'dan olingan:** ijtimoiy stealth, sinxronlash nuqtalari, parkur oqimi.
  **AC'dan olinmagan:** cheksiz olomon kesish, "counter-kill spam".
- Yaqinroq referens: *Ghost of Tsushima* + *Kingdom Come: Deliverance* + *AC Origins* oralig'i.

**Tekshiruv savoli:** *O'yinchi tugmani bosishdan oldin o'ylaydimi?*

---

### 🔥 USTUN 5 — «HAR EPIZOD BIR HIKOYA, HAR MAVSUM BIR DUNYO»
> *"Serial formatidan retention oling, o'yin sifatidan chuqurlik oling."*

**Ma'nosi:**
- 48 epizod. Har biri **45–70 daqiqa**, o'zining boshi-oxiri bor, **cliffhanger** bilan tugaydi.
- Har epizod boshida **"Oldingi epizodda..."** — o'yinchining O'Z o'yin yozuvidan avtomatik montaj (o'z tanlovlari ko'rsatiladi).
- Har mavsum oxirida **dunyo o'zgaradi** — yangi xarita mintaqasi, yangi fasl, yangi dushman arxetipi, yangi mexanika.
- **Retention hook**: har epizod oxirida keyingisining **7 soniyalik teaser**i — lekin faqat o'yinchi tanlovlariga mos keladigan variant.

**Tekshiruv savoli:** *O'yinchi "yana bitta epizod" deydimi?*

---

## Nima BO'LMAYDI (Anti-Pillars)

| ❌ Bo'lmaydi | Sabab |
|---|---|
| Ochiq dunyo (open world) | 48 epizodli hikoya + ochiq dunyo = ikkisi ham yomon bo'ladi. **Hub + kengaytirilgan levellar** (AC Mirage modeli). |
| Loot cheksizligi | Har qurolning tarixi va nomi bor. 200 ta "+3 qilich" emas, **14 ta esda qoladigan qurol**. |
| Zamonaviy siyosiy allegoriya | Tarix o'z-o'zidan dramatik. |
| Millat ulug'lash / dushmanni hayvonlashtirish | Templar ham, mo'g'ul ham — sabab va mantiqqa ega odamlar. Noyan **falsafasi** bor (10-hujjat). |
| Mikro-to'lovlar, lootbox | Premium, bir martalik. Live-ops = kontent, savdo emas. |
| Multiplayer (1.0 da) | Single-player + online meta. Co-op — 2.0 uchun ochiq eshik. |

---

## Maqsadli auditoriya

| Segment | % | Nima uchun keladi |
|---|---|---|
| **Diriliş serial muxlislari** (TR, AZ, UZ, PK, MENA, Balkan) | ~45% | Hikoyani **o'ynash** uchun |
| **Tarixiy action-adventure o'yinchilari** (AC, GoT, KCD) | ~35% | Yangi davr, yangi geografiya |
| **Tarix ixlosmandlari / o'qituvchilar** | ~10% | Discovery Tour rejimi |
| **Souls-lite jang muxlislari** | ~10% | Mix mexanikasi va og'ir jang |

**Platformalar:** PC (Steam/Epic) → PS5 / Xbox Series X|S → (keyinroq) Cloud
**Reyting:** PEGI 18 / ESRB M (zo'ravonlik, qiynoq sahnalari) — **qonli sahnalarni o'chirish opsiyasi** bilan PEGI 16 ga tushirish mumkin

---

## Muvaffaqiyat metrikalari (KPI)

| Metrika | Maqsad | Nima uchun |
|---|---|---|
| **D1 retention** | ≥ 55% | Onboarding sifatini o'lchaydi |
| **Ep.3 completion** | ≥ 70% | "Ilgak" ishladimi |
| **Ep.12 (S1 final)** | ≥ 45% | 1-mavsum tugatganlar |
| **Ep.48 (final)** | ≥ 18% | Sanoat o'rtachasi 12-15% |
| **Codex ochilishi** | ≥ 60 dona/o'yinchi | ⭐ Tarix maqsadi bajarildimi |
| **O'rtacha sessiya** | 75–95 daqiqa | ~1.5 epizod |
| **Discovery Tour** | ≥ 8% | Ta'lim segmenti |
| Metacritic | ≥ 80 | |
