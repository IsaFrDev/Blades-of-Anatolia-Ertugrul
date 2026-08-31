# 04 — CORE GAMEPLAY SYSTEMS

---

# 1. JANG TIZIMI (Combat)

## 1.1 Falsafa
> *"Uch kishi sizni o'ldirishi mumkin. Beshtasi — albatta."*

Referens uchburchagi: **Ghost of Tsushima** (oqim) × **Kingdom Come: Deliverance** (og'irlik) × **Sekiro** (poza/parry).
**Bo'lmaydi:** olomon kesish, cheksiz counter-kill, «hitbox sponge» dushmanlar.

## 1.2 Uchta o'lchov (resurs)

```
  ♥ QON (Health)      — sekin tiklanadi, dori kerak, o'lim yaqin
  ▓ NAFAS (Stamina)   — har harakat sarflaydi, tez tiklanadi
  ⚡ POZA (Posture)    — muvozanat; to'lsa — ochiq qolasiz (dushmanda ham bor)
  🖐 QO'L (Integrity)  — ⭐ EP24 dan keyin (05_MIH_SYSTEM.md)
```

| Harakat | Nafas | Poza (o'zingiz) | Poza (dushman) |
|---|---|---|---|
| Yengil zarba | −8 | — | +12 |
| Og'ir zarba | −22 | −5 | +30 |
| **Parry** (muvaffaqiyatli) | −4 | −2 | **+35** |
| Blok (parry emas) | −15 | **+25** | +5 |
| Dodge (masofa) | −12 | — | — |
| Tepish (chap qo'l davri) | −10 | — | +18 |
| Kamon tortish | −18/sek | — | — |

## 1.3 Parry — tizimning yuragi

```
  Parry oynasi:
  ┌──────────────────────────────────────────────┐
  │  EP01–23 (sog'lom qo'l)          180 ms      │
  │  Sovuqda                         150 ms      │
  │  EP24+ (mix, chap qo'l)          110–165 ms  │  ← HandIntegrity'ga bog'liq
  │  Charchoq (nafas < 20%)           −20 ms     │
  │  «Yengil» qiyinlik sozlamasi      +60 ms     │  ♿
  └──────────────────────────────────────────────┘
```

**Muhim qoida:** muvaffaqiyatli parry ham EP24 dan keyin **qo'lni og'ritadi** (`−3 HandIntegrity`). Bu — o'yinchini parry-spam'dan qaytaradi va **har parry qaror** bo'ladi.

## 1.4 Dodge — i-frame emas, masofa

```cpp
// ❌ An'anaviy: dodge = 0.4 sek jarohatsizlik
// ✅ Bizda:     dodge = fizik ko'chish. Agar zarba yetsa — tegadi.
void UErtGA_Dodge::ActivateAbility(...)
{
    const FVector Dir = GetInputDirection();
    const float   Dist = 240.f * (Wound->Phase == EErtHandPhase::Intact ? 1.f : 0.9f);
    // Root motion bilan ko'chish; hech qanday invulnerability tag qo'yilmaydi
    LaunchWithRootMotion(Dir, Dist, /*Duration=*/0.42f);
    // ⚠️ Faqat "Accessibility.EasyDodge" tag'i bo'lsa 0.15s i-frame qo'shiladi
}
```

## 1.5 Qurollar (14 dona — «200 ta +3 qilich» emas)

| Qurol | Davr | Tarixiy izoh | Xususiyat |
|---|---|---|---|
| **Turk-mo'g'ul sabli** | Butun o'yin | ⚠️ **Yassi *yalman*li klassik "kılıç" — XV asr, anaxronizm.** To'g'ri model: **yumshoq egilgan**, 75–100 sm, uchi nayzali, dastasi pastda egilgan, g'ilofda *tūnkǒu* | Balansli |
| **Kompozit kamon** | Butun | Bambuk/yog'och o'zak, ichida **shox**, tashqarida **paycha**, hayvon yelimi, **qayin po'stlog'i** bilan o'ralgan. **Bosh barmoq halqasi** bilan otiladi | Yomg'irda zaif |
| **Nayza** | S1+ | Otda ustunlik | Masofa |
| **Boltа** | S1+ | Qalqon sindirish | Sekin |
| **Xanjar** | Butun | Stealth takedown | Yashirin |
| **Yalın** ⭐ | EP26+ | Chap qo'l uchun maxsus, zikr ritmida yasalgan | Yengil, tez |
| **Bilakka bog'langan qilich** | EP26+ | Tashlab yubormaysiz, lekin qo'yib ham yubormaysiz | Tanlov |
| **Templar uzun qilichi** | S1 trofey | Og'ir, EP24 dan keyin **ishlatib bo'lmaydi** | Kuchli |
| **Arbalet** | S1 trofey | ⚠️ Sekin qayta o'qlash — tarixiy | Zirh teshadi |
| **Mo'g'ul kamoni** | S3 trofey | Otda otish uchun | Tez |
| **Chumoqli tayoq** | S4 | Axiy hunarmandlari qurol | Poza buzuvchi |
| **Sopqon** | Butun | Arzon, shovqinsiz | Chalg'itish |
| **Lasso (arqon)** | S3+ | ⭐ Chavandozni yiqitish — bir qo'l bilan | Mo'g'ulga qarshi |
| **Olov idishi** | S1+ | Naft (neft) — davrda ishlatilgan | Maydon nazorati |

**⚠️ Anaxronizm nazorati:** har qurolning kodeksida qaysi asr ekani yozilgan. `Xronika rejimi`da anaxronistik qurollar **o'chiriladi**.

## 1.6 Dushman arxetiplari (12 dona, mavsumlar bo'yicha)

| Arxetip | Mavsum | Xatti-harakat | Yechim |
|---|---|---|---|
| Qaroqchi | S1 | Tartibsiz, qo'rquvchan | Har qanday |
| Bo'ri (guruh) | S1 | O'rab olish, birma-bir hujum | Olov, tor joy, orqa devor |
| Templar sержant | S1 | Qalqon + guruh saf | Boltа, tepish, poza sindirish |
| **Arbaletchi** | S1 | Masofadan bosim, sekin qayta o'qlash | Panaga, qayta o'qlashda hujum |
| Templar ritsar | S1-2 | **Parry majburiy**, og'ir zirh | Parry → poza → takedown |
| Xorazmiy nayzachi | S2 | Masofa ustunligi, guruh | Yon tomondan, yaqinlashish |
| **Saroy qotili** | S2 | Zahar, olomonda yashirinish | Ijtimoiy stealth, `BilgeGöz` |
| **Mo'g'ul otliq kamonchi** | S3 | ⭐ Yaqin jangga **kirmaydi**, doimiy masofa | Otni yiqitish (lasso), yopiq joy |
| **Nerge doirasi** | S3 | 60 ta agent kollektiv o'rab olish | Doiradagi zaif nuqta |
| Qamal mashinasi | S3-4 | Devorni buzadi, harakatsiz | Yong'in, muhandisni o'ldirish |
| **Keshikten** (elita) | S3-4 | ⭐ **Parry qilmaydi — kutadi.** Sizni charchatadi | Sabr, poza o'yini |
| Bek/Boss | Har mavsum | Adaptive AI — naqshingizni o'rganadi | Uslubni o'zgartirish |

## 1.7 ⭐ Adaptive Boss AI (Noyan misolida)

```cpp
// 40 soniyada o'yinchi naqshini o'rganadi va qarshi chora ko'radi
void UErtBossBrain::LearnPlayerPattern(float DT)
{
    // Oxirgi 40 soniyadagi harakatlarni histogramma qilamiz
    for (const FErtPlayerAction& A : RecentActions)
        Histogram.FindOrAdd(A.Tag) += A.Weight;

    const FGameplayTag Dominant = Histogram.GetMaxKey();

    if (Dominant == TAG_Player_SpamsDodge)
        Blackboard->SetValue("Strategy", EStrategy::WideSweeps);   // dodge foydasiz
    else if (Dominant == TAG_Player_SpamsParry)
        Blackboard->SetValue("Strategy", EStrategy::FeintHeavy);   // soxta zarba
    else if (Dominant == TAG_Player_KeepsDistance)
        Blackboard->SetValue("Strategy", EStrategy::ClosingRush);
    else if (Dominant == TAG_Player_UsesBow)
        Blackboard->SetValue("Strategy", EStrategy::TargetTheHand); // ⭐ qo'lga hujum
}
```

---

# 2. STEALTH TIZIMI

## 2.1 Ikki xil stealth

| Tur | Qayerda | Mexanika |
|---|---|---|
| **Klassik stealth** | Qal'a, lager, o'rmon, tunda | Ko'rish konusi, ovoz, soya, o't/somon, yuqori nuqta |
| ⭐ **Ijtimoiy stealth** | Bozor, saroy, karvonsaroy, shahar | **Olomon**, kiyim, xulq, tezlik, "kim bilan turasiz" |

## 2.2 Ijtimoiy stealth — 5 o'lchov (AC'dan chuqurroq)

```
   FOSH BO'LISH DARAJASI = f(
       kiyim_mosligi,       // turkman / yunon / arab / darvesh / mo'g'ul
       xulq_mosligi,        // yurish tezligi, qo'l holati, qarash
       joy_huquqi,          // bozor = ochiq, saroy hovlisi = cheklangan
       hamrohlik,           // kim bilan turasiz (Axiy a'zosi bilan = ishonch)
       jarohat_ko'rinishi   // ⭐ EP24+ o'ng qo'l — TANIB OLISH BELGISI
   )
```

⭐ **Mix stealth'ga ta'sir qiladi:** EP24 dan keyin Ertug'rulning qo'li — **belgisi**. Uni yashirish kerak:

| Yashirish usuli | Samara | Narx |
|---|---|---|
| Qo'lqop | 60% | Qo'l 20% sekinroq |
| Latta bog'lash | 40% | Bepul |
| Qo'lni yenga yashirish | 75% | ⚠️ Qurol chiqara olmaysiz |
| Yolg'on protez (S4) | 90% | Qimmat, EP43 dan keyin |

## 2.3 `BilgeGöz` — ikki rejim

| Rejim | Nima ko'rsatadi |
|---|---|
| **Tarix** (`H`) | Kodeks obyektlari, oltin nur *(02_HISTORY_LAYER.md)* |
| **Iz** (`H` ikki marta) | Izlar, qon, ovoz manbai, dushman yo'nalishi, qamal zaifligi |

⚠️ **Cheklov:** `BilgeGöz` **dushman pozitsiyasini devor orqasidan ko'rsatmaydi** (Detective Vision emas). Faqat **iz va tovush** — bu tarixiy va gameplay jihatdan halolroq.

---

# 3. ⭐ HARAKAT: 11 TIZIM (siz so'ragan «ot monopoliyasini sindirish»)

| # | Tizim | Mexanika | Qaysi epizodlarda |
|---|---|---|---|
| **1** | 🚶 **Yayov parkur** | Devor, tom, arqon, mo'ri; **realistik** — 3 m dan baland sakramaysiz | 14 epizod |
| **2** | 🏇 **Ot** | Rivojlanadi (EP18): tezlik, chidam, qo'rquv; otda kamon (Parthian shot); **ot o'ladi va qayta tiklanmaydi** | 7 epizod (**15%**) |
| **3** | 🕳 **Kanalizatsiya / g'or** | Mash'al, zulmat, klaustrofobiya, ovoz aks-sadosi | 4 |
| **4** | 💧 **Suv / suzish** | Nafas, oqim, zirh og'irligi (⚠️ lamellyar zirhda **cho'kasiz**) | 3 |
| **5** | 🧗 **Arqon / krük** | Qal'a burji, jar; ⭐ EP24 dan keyin **bir qo'lda** — tishlab ushlash | 6 |
| **6** | ❄️ **Qor / muz** | Chuqurlik, sirg'anish, ko'chki, qalqonda tushish | 5 |
| **7** | 🛻 **Harakatlanuvchi platforma** | Arava/karvon ustida jang, sakrash, muvozanat | 2 |
| **8** | ⛵ **Qayiq** | Sakarya, Fyrot; oqim, eshkak, yashirin yaqinlashish | 2 |
| **9** | 🦅 **Lochin** | ⭐ **O'ynaladigan** (kinematik emas) — lochinni uchirasiz, dushmanni belgilaysiz; ⚠️ **charchaydi, cheklangan** | 5 |
| **10** | 🐫 **Tuya** | Cho'l, sekin, chayqaladi, yuk ko'p | 2 |
| **11** | 🏰 **Vertikal qamal** | Narvon, taran, mancınık, minora; **jamoa bilan** | 6 |

## 3.1 Ot — endi transport emas, personaj

```
  OT PROFILI (har otning o'z ismi va tarixi bor)
  ├── Tezlik        ← poroda + o'rgatish
  ├── Chidamlilik   ← ozuqa + dam
  ├── Qo'rquv       ← ⭐ olov, qichqiriq, o't; o'rgatilmagan ot JANG QILMAYDI
  ├── Bog'liqlik    ← siz bilan qancha vaqt → jangda o'zi qaytadi
  └── ⚠️ O'lim      ← DOIMIY. Ot o'lsa — yangisini topish/o'rgatish kerak
```

⭐ *Tarixiy asos:* mo'g'ul chavandozida **16 tagacha almashtiriladigan ot**, kuniga **95–120 km**. Sizda 1 ta ot bor — **bu tengsizlikni o'yinchi his qilishi kerak.**

---

# 4. PROGRESSION

## 4.1 To'rt shkalali o'sish

```
  ⚔️  ALP        — jang mahorati (skill tree, 3 shoxobcha)
  🕌  IYMON      — ⭐ sizning g'oyangiz, endi mexanika bilan
  👑  OBRO'      — 5 fraksiya bo'yicha alohida
  🏘  OBA        — jamoa darajasi (base building)
```

## 4.2 ⭐ IYMON shkalasi — mexanikaga aylantirilgan

Sizning JSON'ingizda «Iymon shkalasi» bor edi, lekin mexanikasi yo'q edi. Mana u:

| Nima oshiradi | +/tur | Nima tushiradi | − |
|---|---|---|---|
| Namoz/zikr (marosim, ixtiyoriy) | +3 | Asirni o'ldirish | −8 |
| Yaradorni davolash | +5 | Va'dani buzish | −10 |
| Asirni ozod qilish | +8 | Talon-taroj | −6 |
| Kodeks o'qish (ilm) | +2 | Afyun ishlatish | −4 |
| Sadaqa / Axiy kassasi | +4 | Begunohni o'ldirish | −20 |
| Kechirish (EP45) | +15 | Qasoskorlik (EP47 mixlash) | −25 |

**Iymon nima beradi (mexanik!):**

| Daraja | Effekt |
|---|---|
| 0–25 | ⚠️ Alplar buyruqni sekin bajaradi; flashback ×1.5 |
| 26–50 | Neytral |
| 51–75 | `Sabr` regen +50%; dialogda yangi variantlar |
| 76–100 | ⭐ **«Sukunat»** — jang oldidan 3 soniya vaqt sekinlashadi (1 marta/jang); Axiy va darveshlar bepul yordam beradi; EP48 da eng yaxshi final ochiladi |

> **Muhim:** Iymon **diniy ball emas**, u **ichki barqarorlik**. O'yin hech kimga va'z qilmaydi — u faqat *"o'z qadriyatlaringga sodiq bo'lish kuch beradi"* deydi. Bu — universal va hurmatli.

## 4.3 Alp skill daraxti

```
                    ┌─── QILICH YO'LI ────┐
                    │  Parry ustasi       │
                    │  Poza sindirish     │
                    │  Ikki zarba         │
                    └─────────────────────┘
   ⚔️ ALP ──────────┼─── KAMON YO'LI ─────┼──── EP26 dan keyin:
                    │  Nafasni ushlash    │     ┌── «SOL YOL» ──────┐
                    │  Otda otish         │     │ Tepish            │
                    │  Zirh teshish       │     │ Bog'langan qilich │
                    └─────────────────────┘     │ Tishlab ushlash   │
                    ┌─── SOYA YO'LI ──────┐     │ Kamsitilish       │
                    │  Ovozsiz yurish     │     │ Bir qo'lli grapple│
                    │  Ijtimoiy niqob     │     │ Og'riqni yutish   │
                    │  Zaharlash          │     └───────────────────┘
                    └─────────────────────┘        (14 mahorat)
```

⚠️ **EP24 dan keyin:** «Qilich yo'li»ning 6 ta mahorati **o'chadi** (o'ng qo'l kerak). O'yinchi **kompensatsiya sifatida 6 ta bepul «Sol Yol» ochilishi** oladi — bu **jazо emas, transformatsiya**.

---

# 5. OBA BOSHQARUVI (Base Building)

```
   ┌────────────────────────────────────────────────────────┐
   │  OBA — 9 bino, har biri 3 daraja                       │
   ├────────────────────────────────────────────────────────┤
   │  🔨 Temirxona     → qurol/zirh, ta'mir, ⭐ mix          │
   │  🏪 Bozor         → oltin, savdo yo'llari, ma'lumot     │
   │  🕌 Masjid-madrasa→ Iymon, kodeks tarjimasi, savodxonlik│
   │  ⛺ Chodirlar     → aholi sig'imi, ruhiyat              │
   │  🐑 Qo'ra         → ozuqa, teri, jun                    │
   │  🌿 Shifoxona     → ⭐ HandIntegrity davolash, dori      │
   │  🏹 Mashq maydoni → alp o'rgatish, sparring             │
   │  🛡 Devor/ariq    → ⭐ EP44 mudofaasida hal qiluvchi     │
   │  📜 Kotibxona     → hujjat, soliq, ittifoq shartnomasi  │
   └────────────────────────────────────────────────────────┘
```

**Resurslar:** `Dirham` (⚠️ **akçe emas** — u usmonli tangasi, 100 yil keyingi) · `Temir` · `Teri` · `Don` · `Yun` · `Yog'och`

**⭐ Oba jangga ta'sir qiladi:**
```
   EP44 mudofaa jangida:
   Devor 3-daraja       → dushman 4 daqiqa kechikadi
   Ariq                 → otliqlar tushishga majbur
   Shifoxona 2+         → o'lgan alplarning 40% "og'ir yarador" bo'ladi
   Kotibxona            → vizantiyalik qo'shnilar yordamga keladi (EP39 tanlovi)
   Axiy ittifoqi        → +60 hunarmand jangchi
```

---

# 6. FRAKSIYALAR VA DIPLOMATIYA

| Fraksiya | Kim | Nima xohlaydi | Sizga nima beradi |
|---|---|---|---|
| 🟢 **Saljuqiy** | Konya sultonligi | Sodiqlik, soliq, askar | Yer, huquq, himoya |
| 🟡 **Ayyubiy** | Halab/Damashq | Savdo, chegara tinchligi | Oltin, bilim, tibbiyot |
| ⚪ **Templar** | Amanos qal'alari | Karvon yo'li nazorati | Temir, qurol, razvedka |
| 🔴 **Mo'g'ul** | Ilxoniylar | Bo'ysunish, o'lpon, ro'yxat | Tinchlik, *paiza*, yo'l |
| 🟣 **Axiy** | Hunarmand-so'fiy | Adolat, hunar, mehmondo'stlik | ⭐ Iqtisod, odamlar, obro' |
| 🔵 **Vizantiya** (S4) | Nikeya | Chegara barqarorligi | Tibbiyot, savdo, nikoh |

**⭐ Muvozanat qoidasi:** har fraksiya obro'si `−100 … +100`.
**Neytral qola olmaysiz** — birida +20 bo'lsa, uning dushmanida −10.

```cpp
// Fraksiya munosabatlari matritsasi
static const float FactionEnmity[6][6] = {
//         Selj   Ayyu   Temp   Mong   Ahi    Byz
/*Selj*/ {  0.0,  -0.3,  -0.4,  -0.9,  +0.5,  -0.2 },
/*Ayyu*/ { -0.3,   0.0,  -0.8,  -0.7,  +0.3,  -0.1 },
/*Temp*/ { -0.4,  -0.8,   0.0,  -0.2,   0.0,  -0.5 },
/*Mong*/ { -0.9,  -0.7,  -0.2,   0.0,  -0.6,  -0.4 },
/*Ahi */ { +0.5,  +0.3,   0.0,  -0.6,   0.0,  +0.2 },
/*Byz */ { -0.2,  -0.1,  -0.5,  -0.4,  +0.2,   0.0 }
};
// ⚠️ Templar–Mo'g'ul faqat -0.2: tarixan ular ittifoq izlagan (EP31)
```

---

# 7. IQTISOD

## 7.1 Tanga tizimi — tarixiy aniq

| Tanga | Metall | Tarixiy izoh |
|---|---|---|
| **Dirham** | Kumush | ⭐ Saljuqiy Rumning standart tangasi; Konya, Sivas, Kayseride zarb qilingan; **XIII asr boshida Anadoluda kumush konlari topilgan** → "kumush toshqini" → dirham mintaqaviy **etalon valyuta** bo'lgan |
| **Dinor** | Oltin | Katta savdo, o'lpon |
| **Fals** | Mis | Kundalik mayda savdo |
| ❌ ~~Akçe~~ | — | **USMONLI tangasi (~1327, Bursa) — ANAXRONIZM** |

## 7.2 Savdo yo'llari (haqiqiy)

⚠️ *Tarixiy tuzatish:* **"Konya–Halab magistral karvonsaroy zanjiri" mavjud emas edi.** Shamga boradigan trafik **Konya–Aksaray–Kayseri–Malatya** o'qi orqali yurgan. O'yinda yo'l tarmog'i shunga mos qurilishi kerak.

| Yo'l | Haqiqiy karvonsaroylar | Tovar |
|---|---|---|
| Konya–Aksaray | **Zazadin Han (1235-36, Köpek qurdirgan)** | Don, jun |
| Aksaray–Kayseri | **Sultan Han (1229)**, **Ağzıkara Han (1231/1240)** | Ipak, ziravor |
| Kayseri–Sivas | **Sultan Han Kayseri (1232-36)** | Temir, qul |
| Alanya–Antalya | **Alara Han (1231-32)** | Dengiz savdosi |
| Eğirdir–Konya | **Ertokuş Han (1223)** | Teri |

> ⭐ **Bularning hammasi bugun ham turibdi.** Ular o'yin darajasi sifatida qurilsa — o'yinchi haqiqiy binoda yuradi, va Discovery Tour'da uning fotosini ko'radi. Bu — kuchli.

---

# 8. SIDE-QUEST ZANJIRLARI (sizning 11 tangizni qayta ishlash)

Sizning side quest'laringiz **bir martalik** edi. Yangi model: **zanjir** (3–5 bosqich, mavsumlar bo'ylab, oxirida asosiy hikoyaga ta'sir).

| Zanjir | Beruvchi | Bosqich | Asosiy hikoyaga ta'siri |
|---|---|---|---|
| **«Aykizning ko'z yoshi»** | Aykiz | 4 (EP03→EP12) | Kurdo'g'lu fosh bo'lishi tezlashadi |
| **«Klavdiydan Umarga»** | Ibn Arabiy | 5 (EP08→EP31) | ⭐ Titus bilan ittifoq imkoniyati |
| **«Bo'ri ovi»** | Deli Demir | 3 (EP05→EP15) | Zirh retsepti, EP44 mudofaasi |
| **«Eftelyaning izi»** | Afshin Bey | 4 (EP06→EP20) | Köpekning tarmog'i ochiladi |
| **«Selcanning tavbasi»** | Selcan | 5 (EP09→EP42) | Merosxo'r tanlovi ochiladi |
| **«Gonchagulning tuzog'i»** | Banu Chichek | 3 (EP16→EP27) | Beklikni qaytarish osonlashadi |
| **«Bektosh hikmati»** | Hoji Bektosh | 5 (EP26→EP48) | ⭐ Iymon 76+ ga chiqish imkoni |
| **«Artuq Beyning dorixonasi»** | Artuq Bey | 4 (EP18→EP43) | ⭐ HandIntegrity davolash retseptlari |
| **«Dundarning birinchi qilichi»** | Dundar | 3 (EP15→EP42) | Merosxo'r sifati |
| **⭐ «Hamzaning vijdoni»** | Hamza | **5 (EP17→EP45)** | 🔴 **EP47 da Hamza sizni qutqaradi yoki o'ladi** |
| **«Axiy kassasi»** | Axiy ustasi | 4 (EP40→EP48) | Iqtisodiy g'alaba yo'li |
| **«Yunon shifokor»** ⭐yangi | Nikeforos | 3 (EP39→EP43) | 🔴 **Protez → `MaxIntegrity +15`** |

⚠️ **Sizning `sq_selcan_secrets` da `"season": 0` bug'i bor edi** — tuzatildi.

---

# 9. QIYINLIK VA ACCESSIBILITY

## 9.1 Qiyinlik darajalari

| Daraja | Parry oynasi | Dushman zarari | Mix pasayishi | Kimga |
|---|---|---|---|---|
| **Rivoyat** | +80 ms | ×0.5 | ×0.4 | Hikoya uchun keladiganlar |
| **Alp** (default) | Bazaviy | ×1.0 | ×1.0 | — |
| **Chegara** | −20 ms | ×1.4 | ×1.3 | Tajribali |
| **Xronika** | −35 ms | ×1.8 | ×1.6 | ⭐ NG+ da ochiladi; **permadeath alplar** |

## 9.2 Alohida sozlamalar (qiyinlikdan mustaqil)

Har biri alohida — o'yinchi o'z tajribasini quradi:
`Parry oynasi` · `Dushman soni` · `Mix tezligi` · `Nishonga yordam` · `QTE→ushlab turish` · `Ritm minigame→avtomatik` · `Stealth ogohlantirish vaqti` · `O'lim jazosi`

---

# 10. RETENTION MEXANIKALARI

| Mexanika | Qanday ishlaydi | Kutilgan ta'sir |
|---|---|---|
| **Cliffhanger** | Har epizod javobsiz savol bilan tugaydi | +12% keyingi sessiya |
| ⭐ **Dinamik rekap** | O'yinchining **o'z tanlovlaridan** montaj | +8% qaytish |
| **7 soniyalik teaser** | Keyingi epizoddan, tanlovga mos | +6% |
| ⭐ **Safar Daftari** | Shaxsiy yozuv, eksport, ulashish | Viral + qaytish |
| **Mavsum tugashi** | Dunyo o'zgaradi — yangi mintaqa/mexanika | +15% mavsumlararo |
| ⭐ **Mix** | O'yinchi qo'lini "sog'aytirmoqchi" | Doimiy motivatsiya |
| **Kodeks kolleksiyasi** | 180 yozuv, 100% completion | Tugatgandan keyin ham |
| **Shubha sahnalari** | Boshqa tanlov = boshqa o'yin | NG+ sababi |
| **Side-zanjirlar** | 5 bosqich, mavsumlar bo'ylab | Kontent chuqurligi |
| **Merosxo'r tizimi** | O'g'illar EP48 ga ta'sir qiladi | Uzoq muddatli maqsad |
