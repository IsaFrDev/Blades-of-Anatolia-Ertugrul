# DIRILISH: ERTUG'RUL — 3D o'yin (C++20 / Win32 / OpenGL)

Tashqi kutubxonasiz PC o'yini: menyu, til tanlash, 48 epizodli katalog,
ovozli kino sahnalar (cutscene) va uchinchi shaxs erkin yurish rejimi.

| Qism | Texnologiya | Holat |
|---|---|---|
| Yadro | C++20, CMake + Ninja, MinGW-w64 g++ 16 | ishlaydi |
| Grafika | OpenGL 1.1 fixed-function + GLU, WGL | ishlaydi |
| Rasm dekodlash | GDI+ (JPG/PNG → GL teksturasi) | ishlaydi |
| Shrift | GDI glif atlasi (UTF-8, uz/tr belgilar) | ishlaydi |
| Ovoz | waveOut aralashtirgich + SAPI TTS zaxirasi | ishlaydi |
| Kontent | 48 epizod, **48 cutscene**, 3 daraja, 3 til (840 kalit) | ishlaydi |
| Ovoz banki | 309 replika x 3 til = **927 WAV** | tayyor |
| Dizayn | «Temir va Firuza» tizimi (GDD mokaplaridan) | qo'llangan |
| Parkur | AC uslubidagi holat mashinasi (31 holat) + zond fizikasi | ishlaydi |
| Jang | 3 resurs, 6 dushman turi, AI, parry/blok/kombo | ishlaydi |
| Kamon | ballistik o'q, tana zonalari, yashirin otish | ishlaydi |
| Iymon | 4 daraja, mexanik ta'sir, «Sukunat» sekinlashuvi | ishlaydi |
| Zarba qaytarmasi | muzlash, kamera silkinishi, uchqun, protsedural tovush | ishlaydi |
| Epizod halqasi | maqsadlar, to'lqinlar, nazorat nuqtasi, o'lim va qayta tug'ilish | ishlaydi |

> **Eslatma:** `game/legacy/` — eski prototip (300 qatorli `main.cpp`), qurilishga kirmaydi.
> `ertugrul/` papkasi — alohida UE5 + Spring loyihasi konsepti, bu qurilishga aloqasi yo'q.

---

## 1. Yig'ish va ishga tushirish

```powershell
cd D:\My_apps\Ertugrul
.\scripts\build.ps1
.\build\ertugrul.exe
```

Talab: **MinGW-w64 g++ 13+**, **CMake 3.15+**, **Ninja** (`D:\gcc\mingw64\bin` da).
Ishchi papka loyiha ildizi bo'lishi **shart** — `data/`, `assets/`, `localization/` shu yerdan o'qiladi.

### Buyruq qatori

| Bayroq | Ma'nosi |
|---|---|
| `--lang uz\|tr\|en` | interfeys tili (menyudagi tanlovni bekor qiladi) |
| `--episode EP001` | menyuni o'tkazib, to'g'ridan-to'g'ri epizodni boshlaydi |
| `--width N --height N` | oyna o'lchami (ish stoli sohasiga sig'diriladi) |
| `--fullscreen` | to'liq ekran |
| `--quality 0\|1\|2` | model sifati (skinning chastotasi: 20 / 30 / 60 Hz) |
| `--no-console` | konsol oynasini yashiradi |
| `--check` | kontent diagnostikasi va chiqish (GPU shart emas) |
| `--shots <papka>` | interfeys bo'ylab avtomatik sayohat, har bosqichda PNG saqlaydi |
| `--level <id>` | menyusiz to'g'ridan-to'g'ri darajaga tushish (`sogut_village`, `oba_camp`, ...) |

### Boshqaruv

**Barcha klavishlar sozlanadi:** Sozlamalar -> BOSHQARUV bo'limi. Qatorni tanlab
`Enter` bosing, keyin yangi klavishni bosing — uning nomi darhol o'sha qatorda ko'rinadi.
`Delete` bog'lamani bo'shatadi, `Esc` bekor qiladi. Klavish band bo'lsa eskisi
avtomatik bo'shatiladi va qizil bilan ogohlantiriladi. Natija `saves/bindings.json` ga saqlanadi.

Standart qiymatlar (GDD `07_SETTINGS_HOTKEYS` bo'yicha):

| Guruh | Amal | Tugma |
|---|---|---|
| Harakat | Oldinga / Orqaga / Chapga / O'ngga | `W` `S` `A` `D` |
| | Yugurish / Cho'kkalash / Chetlanish | `Shift` `C` `Q` |
| Jang | Yengil zarba / Parry-blok | `Mouse 1` / `Mouse 2` |
| | Tepish / Kamon / Yashirin o'ldirish | `R` `G` `V` |
| Dunyo | Aloqa / Ot / Bilge Ko'z | `E` `F` `H` |
| | Safar daftari / Vaqt chizig'i / Yil kartasi | `J` `N` `Y` |
| | Qo'lni bog'lash / yashirish / Zikr / Lochin | `Z` `Alt` `X` `L` |
| Tizim | Pauza / Sahnani o'tkazish / Replikani tezlatish | `Esc` `Esc` `Space` |

Kamera — sichqoncha (o'yin rejimida qulflanadi), g'ildirak masofani o'zgartiradi.
Menyu: strelkalar / `Enter` / `Tab` (bo'lim) / sichqoncha. Chiqish: `Ctrl+Q`.

---

## 2. Nima ishlaydi

**Menyu oqimi** — Splash → **til tanlash (uz / tr / en)** → asosiy menyu →
epizod tanlash → yuklash → **cutscene** → o'yin → pauza.
Fon sifatida jonli 3D lager sekin aylanib turadi. Sozlamalar `saves/settings.json` ga saqlanadi.

**Epizod katalogi** — `data/episodes/episodes_v2.json` dan 48 epizod, 4 mavsum yorlig'i,
har biri uchun sarlavha / qisqacha mazmun / cliffhanger / yil / mintaqa / arxetip / qiyinlik
(uchala tilda to'liq tarjima qilingan).

**Dizayn tili «Temir va Firuza»** — GDD mokaplaridan olingan:
fon `#0E1316` · panel `#161D21` · chiziq `#2A353A` · matn `#E4EAEA` · so'nik `#7C8B8F` ·
feruza `#48A9B5` (tanlov) · zarhal `#C09660` (qiymat) · yara `#BC5A44` (ogohlantirish).
Chapga tekislangan tahririy joylashuv, katta serif sarlavhalar, mono yorliqlar,
va yagona takrorlanuvchi motiv — kichik kvadrat («mix boshi»).

**Cutscene** — **48 epizodning HAMMASI uchun**, ma'lumotga asoslangan (`data/cutscenes/*.json`):
- aktyorlar kalit nuqtalar bo'ylab **haqiqatan yuradi** (Catmull-Rom yo'l, tezlikdan Walk/Run avtomatik tanlanadi)
- gapiruvchi `Talk` klipiga o'tadi, boshqalar `Listen` bilan unga qaraydi
- kamera kalitlari silliq interpolyatsiya qilinadi, kalitlar bo'lmasa gapiruvchiga qaratiladi
- **ovoz**: `assets/audio/vo/<til>/<id>.wav`, topilmasa jonli SAPI TTS
- subtitr + gapiruvchi ismi, letterbox, fade in/out

**Parkur — Assassin's Creed uslubi.** Klassik AC ning ikki profil modeli:

| | Past profil | Yuqori profil (`Shift`) |
|---|---|---|
| Harakat | yurish / jog | yugurish + **erkin yurish (free-run)** |
| To'siq | to'xtaydi | **avtomatik sakrab o'tadi** |
| Devor | to'xtaydi | **chekkani ushlab tepaga chiqadi** |
| Shovqin | 0.04–0.35 | 0.6–1.0 (qorovullar sezadi) |

21 ta harakat holati: Idle · Walk · Jog · Sprint · CrouchIdle · CrouchWalk · Slide ·
**Vault** (past to'siq) · **Mantle** (chekkadan tepaga) · **Climb** (devorga yopishib) ·
**Hang** (osilib turish) · **Shimmy** (yon siljish) · **Eject** (orqaga sakrash) ·
JumpUp · Fall · Land · **RollLand** (dumalab qo'nish — shovqin kam) · WallRun ·
Dodge · Assassinate · LeapOfFaith.

Holatlar AVTOMATIK tanlanadi — `world/Physics.h` daraja rekvizitlaridan vertikal
qutilar quradi va har kadr zond tashlaydi: `probeVault` (past to'siq),
`probeLedge` (ushlab olsa bo'ladigan chekka), `probeWall` (chiqib bo'ladigan yuza),
`mantleClear` (tepada joy bormi), `supportBelow` / `platformAbove` (tom, zina).

**Bilge Ko'z** (`H`) — AC dagi Eagle Vision: dunyo rangi so'nadi va parkur
geometriyasi yonadi (feruza — chiqib bo'ladigan chekka, zarhal — sakrab o'tiladigan
to'siq). Bu shunchaki bezak emas: qaysi tomga o'tish mumkinligini ko'rsatadi.

**Parkur darajasi** — `data/levels/sogut_village.json`: modulli yog'och binolar
(bir va ikki qavatli), tomlar, ~9 m minora, ko'prik, bozor va yerdan tomga
ko'tariladigan "narvon" yo'llari (arava → quti → tom). 371 rekvizit, 880 to'qnashuv qutisi.

**Yurish — masofaga bog'langan qadam.** Bu tizimning eng nozik qismi.
Odatdagi protsedural yurishda qadam fazasi VAQTdan olinadi (`ph = t x rate`) —
natijada tezlik o'zgarganda oyoq yerdan sirg'aladi. Bizda faza **bosib o'tilgan
MASOFAdan** integrallanadi:

```
S(v) = 0.97 + 0.35 x v          (to'liq sikl uzunligi, metr)
faza += bosibOtilganMasofa / S(v)
```

Tayanch fazasida panja **dunyoda qotadi** (ildizga nisbatan chiziqli orqaga siljiydi),
oyoq burchaklari esa ikki bo'g'inli **analitik IK** bilan hisoblanadi. Sinusoidal
oyoq bilan sirg'alishni nolga tushirib bo'lmaydi: tayanch panjaning dunyo tezligi
`v + A*2pi*f*cos(ph)`, uni nolga tenglash `A = S/2pi` ni talab qiladi, geometriya esa
`A = S/2` ni — ular nomuvofiq.

Natija (`ERT_MOVE_LOG=1` bilan o'lchanadi, `SLIP` ustuni):

| Tezlik | Qadam/s | Real odam | SLIP |
|---|---|---|---|
| yurish 1.35 m/s | **1.87** | 1.85-1.90 | **0.0%** |
| yugurish 3.30 m/s | **3.11** | 3.0-3.2 | **0.0%** |
| sprint 6.40 m/s | **3.99** | 3.9-4.2 | **0.0%** |

Ilgari bu ko'rsatkichlar 4.6 / 4.6 / 8.8 qadam/s edi — personaj tezlikdan qat'i nazar
mayda-mayda tipirchilardi.

**Inersiya.** Tezlik endi VEKTOR (`vel_`), yo'nalishi bir kadrda o'zgarmaydi.
Burilish tezligi fizikadan cheklanadi: `a_lat = v x omega <= 12 m/s^2` — sprintda
110 deg/s (radius 3.4 m), yurishda 509 deg/s. Yo'nalish jarimasi (180 gradusda x0.25)
tezlikni tushiradi, tezlik tushgani uchun burilish qattiqlashadi — aynan AC hissi.
180 gradus qaytishda **pivot** oynasi (0.25 s) tezlikni nolga tushiradi.
Sekinlanish masofa bo'yicha chiziqli: sprintdan 1.04 m / 0.33 s "run-out".
Burilishda tana yon egiladi (bank, 14 gradusgacha), tezlanishda oldinga (lean).

**Tezlik pog'onalari:** `Alt` yurish (past profil) · standart yugurish ·
`Shift` sprint. Klaviaturada analog yo'q, shuning uchun tugmani ushlash
DAVOMIYLIGI pog'ona beradi: birinchi 0.28 s yurish, keyin yugurish.

**Kamon.** `G` ni ushlang — kamera o'ng yelka ustiga siljiydi, ko'rish maydoni
48 dan 36 gradusga torayadi, nishon halqasi chiziladi. Tortish 0.85 s.
Nafas sarflanadi (to'la tortib ushlaganda 11/s) — nafas kamaygach qo'l titraydi
va halqa kengayadi; nafas tugasa **majburiy reliz**.

O'q ballistik: gravitatsiya bilan uchadi, 0.40 m qadamlar bilan integrallanadi va
har qadamda uch narsa tekshiriladi — yer, devor va dushman silindri.
Zarar tana zonasiga bog'liq: oyoq x0.55, ko'krak x1.00, **bosh x2.60**.
Dubulg'asiz dushmanni boshdan bir o'q o'ldiradi — va u **qichqirmaydi**,
ya'ni qo'shnilari bilmaydi. Qalqonli serjantga oldindan otish befoyda (x0.15) —
yon tomondan otish kerak. Masofa 25 m dan keyin zararni so'ndiradi.
Devorga tekkan o'q qorovulni o'sha yoqqa yuboradi — **diqqatni chalg'itish**.
Sadoq 12 ta, har to'lqin oralig'ida +4.

**Iymon — jangdagi joriy formangiz.** Bu "yig'iladigan ochko" emas:

| Daraja | Oraliq | Mexanik ta'sir |
|---|---|---|
| Adashgan | 0-25 | nafas x0.75, poza x0.80 |
| Shubha | 26-50 | neytral |
| Sobit | 51-75 | nafas x1.50 |
| **Sukunat** | 76-100 | nafas x1.50, poza x1.50, **vaqt sekinlashuvi** |

Ko'tarilish MAHORATLI harakatlardan: mukammal parry +0.4, yakunlovchi zarba +2.0,
zarba yemasdan tugatilgan to'lqin +5.0, epizod +6.0.
Tushish: har zarba -0.6, o'lim -8.0. Ya'ni maksimalga chiqarib qo'yib beparvo
o'ynab bo'lmaydi. Sukunat darajasida parry yoki finisher **1.2 s davomida
vaqtni 0.45x ga sekinlashtiradi** (9 s sovish vaqti bilan — spam qilib bo'lmaydi).
Iymon `saves/progress.json` da saqlanadi va epizodlar orasida davom etadi.

**Animatsiya nuqsoni — eng katta yashirin xato.** `setClip()` bir xil klip
qayta so'ralganda bir martalik klipni QAYTA BOSHLARDI. Character va Enemy esa
klipni HAR KADR so'raydi, ya'ni klipning mahalliy vaqti doim `dt` ga teng
bo'lib qolardi:

```
setClip(c)   ->  c == clip_  ->  oneShotSet(time_)     [vaqt = T]
update(dt)   ->  time_ = T + dt
                 oneLocal = time_ - oneShotGet() = dt  <-- AYNAN dt, har kadr
```

Natijada BARCHA bir martalik kliplar birinchi kadrida muzlab qolardi:

| Klip | Nima ko'rinardi |
|---|---|
| Hurt | bosh 21-33 gradus orqaga tashlangan va yonga qiyshaygan holda 0.35 s QOTIB turardi — "kaltak yesa boshi qiyshayadi" |
| AttackLight1 | `u = 0.03` da qilich burchagi 1 gradus — qilich UMUMAN ko'tarilmasdi |
| Vault / Mantle / Slide / Roll / Dodge / Execute | hammasi muzlagan |
| Death | jasad TIK turardi |

Ustiga: `dirty_` har kadr yonib, 30 Gts skinlash darvozasi chetlab o'tilardi —
har bir jangchi uchun 50 ming verteks HAR KADR qayta skinlanardi.

Tuzatish: `setClip()` endi qayta boshlamaydi, buning uchun alohida `playClip()`
bor va u FAQAT holat almashganda chaqiriladi. `ERT_SKIN_STATS=1` bilan
o'lchanadi — `oneLocal` qiymati klip davomiyligi bo'ylab o'zgarib turishi kerak
(tuzatishdan oldin u doim `0.016` edi).

**Bosh riggingi.** Ikkinchi sabab skinlashda edi: og'irlik DAM OLISH pozasi
bilan aralashardi (`v + (transformed - v) * w`), ya'ni boshning pastki yarmi
45%, salla uchi 100% burilardi — bosh aylanmasdan QIRQILARDI. Endi og'irlik
OTA SUYAK bilan aralashadi (klassik ikki suyakli skinlash): bo'yin silliq
egiladi, bosh qattiq jism bo'lib qoladi.

Bo'yin balandligi ham qat'iy `0.86 x bo'y` edi. Endi u MESHDAN o'lchanadi:
pastdan yuqoriga birinchi LOKAL MINIMUM (shunchaki "eng tor kesim" ishlamaydi -
u doim salla uchini topadi), ustiga yelka kengligiga nisbatan chegara.
O'lchangan: ottoman 0.86, crusader 0.90 (ilgari ikkalasi ham 0.86 deb olinardi).
Bundan tashqari `computeXforms()` da anatomik chegaralar bor: bo'yin +-70/50/40,
tizza faqat orqaga, tirsak faqat oldinga.

**Zarba yeyish - endi YO'NALISHLI.** `poseHurt` zarba burchagini oladi:

| Zarba | Reaksiya |
|---|---|
| oldindan | tana ORQAGA, orqaga yarim qadam |
| orqadan | tana OLDINGA cho'ziladi |
| yondan | tananing yon egilishi + o'sha tomon qo'li ko'proq yig'iladi |

Amplituda zarba kuchidan keladi (`hitWeight`: o'q 0.35, og'ir zarba 1.00).
Bosh burchagi -40 dan -26 gradusga tushirildi va yon siljish geometrik
byudjetga solindi: bo'yin radiusi 9.6 sm, `TORSO.rz = 5 gradus` -> 5.5 sm.
Orqadan zarbada personaj 180 gradusga pirillamaydi (burilish +-55 bilan
cheklangan) - aks holda yo'nalishli poza hech qachon ko'rinmasdi.
Muvozanat buzilishi va O'LIM ham yo'nalishli: jasad endi zarba kelgan
tomonga QARAB emas, undan TESKARI yiqiladi.

**Zarba berish - endi qaytarma bor.** Ilgari zarba natijasi `(void)hr;` bilan
tashlanardi. Endi:

| Natija | Muzlash | Uchqun |
|---|---|---|
| o'ldirdi | 0.100 s | katta feruza halqa |
| tegdi | 0.055 s | feruza kvadrat |
| bloklandi | 0.035 s | zarhal kvadrat |
| parry (o'yinchi) | 0.130 s | zarhal |

Muzlash HAQIQIY `dt` bilan so'nadi — aks holda o'zi sekinlashtirgan vaqt bilan
kamayib, cheksiz cho'zilardi. Kamera muzlamaydi (u `lastDt` bilan yuradi).

**Dushman endi haykal emas.** `Attack::knockback` maydoni bor edi, lekin butun
loyihada faqat o'yinchi tomonida o'qilardi. Endi dushman zarbadan orqaga
suriladi: yengil 0.21 m, 3-kombo 0.27, og'ir 0.42, tepish 0.52 m.
Devorga tiralganda o'tib ketmaydi.

**Halollik tuzatishlari.** Dushman zarbasi ilgari masofa ham, burchak ham
tekshirmasdi — og'ir zarbaning 0.38 s tayyorgarligi davomida qochib ketsangiz
ham zarba tegardi. Endi zarba faqat yetib boradigan masofada va oldingi
sektorda tegadi. Qalqonli serjant esa endi ORQADAN kelgan zarbani to'sa
olmaydi (blok faqat oldingi 120 gradus sektorda).

**Zarba qaytarmasi — nima uchun jang «bo'sh» edi.** Uchta uzilgan sim topildi:

1. `setClip()` bir martalik klipni **har kadr qayta boshlardi**. `Character::update()`
   va `Enemy::update()` klipni har kadr so'raydi, `setClip` esa mahalliy vaqtni nolga
   qaytarardi — natijada zarba, zarba yeyish, muvozanat buzilishi, parkur va
   yakunlovchi zarba animatsiyalari **birinchi kadrida muzlab** turardi. Poza `lt = dt`
   da qotgani uchun natija kadr chastotasiga ham bog'liq edi: 60 Гц da bosh 21°,
   30 Гц da 33° orqaga tashlangan holda 0.35 s qotardi — «kaltak yesa boshi qiyshayadi»
   shikoyatining asosiy sababi shu edi. Endi qayta boshlash faqat `playClip()` orqali,
   ya'ni holat almashganda.
   O'lchash: `ERT_SKIN_STATS=1` → `oneLocal` **o'zgarib turishi** kerak. Doim `0.016`
   bo'lsa tuzatish ishlamagan.
2. `HitResult` `(void)hr;` bilan **tashlab yuborilardi** — zarba tekkani hech qayerga
   bildirilmasdi.
3. `Attack::knockback` butun loyihada **hech qachon o'qilmasdi** — dushman zarba
   yeganda haykal edi.

Endi har zarba to'rt kanaldan qaytaradi:

| Kanal | Tegdi | Bloklandi | O'ldirdi | O'zi zarba yedi | Parry |
|---|---|---|---|---|---|
| Muzlash (hitstop) | 55 ms | 35 ms | 100 ms | 70 ms | 130 ms |
| Kamera turtkisi | 0.28 | 0.18 | 0.45 | **0.65** | 0.40 |
| Uchqun | feruza | zarhal | ikki qavat | feruza | zarhal |
| Tovush | past «chuq» | metall jarang | og'ir zarba | kuchliroq | yorqin jarang |

Muzlash HAQIQIY `dt` bilan so'nadi (`tickFeedback`) — aks holda o'zi sekinlashtirgan
vaqt bilan kamayib, cheksiz cho'zilardi. Kamera silkinishi `camSmooth` ga
**yozilmaydi**: agar yozilsa keyingi kadrda damp silkigan joydan tortadi va turtkilar
qo'shilib kamerani joyidan surib yuborardi.

**Zarba yeyish endi YO'NALISHLI.** `poseHurt` zarba burchagini oladi: oldindan zarba —
tana orqaga, yon tomondan — tanani chetga, orqadan — oldinga cho'zilish. Bosh siljishi
**geometrik byudjet** bilan cheklangan: `.obj` dan o'lchangan bo'yin radiusi
9.6 sm (ottoman) / 7.8 sm (crusader), `TORSO.rz = 5°` esa boshni 5.5–5.8 sm suradi —
ya'ni bosh bo'yindan chiqmaydi. Ilgari `rz = 6°` va **yo'nalishsiz** edi: zarba
qayerdan kelsa ham bosh doim bir tomonga qiyshayardi.
Bundan tashqari `computeXforms()` da anatomik chegara qo'yildi — bo'yin ±50/70/40°,
tizza faqat orqaga, tirsak faqat oldinga bukiladi. Hech qanday poza buni buza olmaydi.

**Tovushlar protsedural — tovush fayli yo'q.** `game/src/audio/Sfx.cpp` 9 ta jang
tovushini sintez qiladi (jami 289 KB, ishga tushganda bir marta):

| Tovush | Qurilishi | Spektral markaz |
|---|---|---|
| o'lim | past pasayuvchi sinus + filtrlangan shovqin | 1108 Гц |
| o'ldiruvchi zarba | 230→48 Гц pasayish + past qismlar | 1387 Гц |
| tanaga zarba | 190→72 Гц + quruq shovqin | 1922 Гц |
| qalqonga zarba | **garmonik bo'lmagan** qismlar 1 : 1.52 : 2.14 : 2.84 | 2227 Гц |
| parry | o'sha nisbat, yuqoriroq va uzunroq ring | 3291 Гц |
| qilich yoyi | o'rtada ochiladigan filtr + qo'ng'iroq konvert | 3524 Гц |

Metall jarangi aynan **butun sonli bo'lmagan** nisbatdan chiqadi — garmonik qismlar
«musiqiy» eshitiladi va zarbaga o'xshamaydi. Shovqin determinatsiyalangan (qat'iy seed),
shuning uchun tovushlar har ishga tushirishda bir xil. Ovoz masofa bilan so'nadi va
har chalinishda ohangi jadval bo'yicha biroz o'zgaradi — takrorlanish quloqni charchatmaydi.
`ERT_SFX_DUMP=<papka>` ularni WAV qilib yozadi (`assets/audio/sfx/` da tayyor turibdi).

**Jang — uch resurs.** GDD (04_CORE_SYSTEMS) jadvali bo'yicha, jang OG'IR va
JAZOLOVCHI: uch kishi o'ldirishi mumkin, beshtasi — albatta.

| Resurs | Ma'nosi | Tugasa |
|---|---|---|
| **Sog'liq** | zarba zarari | o'lim |
| **Nafas** | har harakat sarflaydi | hujum qilib bo'lmaydi, blok buziladi |
| **Poza** | zarba/blok to'playdi | **muvozanat buziladi** — yakunlovchi zarbaga ochiq |
| **Qo'l** (Mix) | og'ir zarba va parry yeydi | parry oynasi 180 ms dan 110 ms ga qisqaradi |

Harakatlar: yengil zarba (3 ta kombo) · og'ir zarba (`Shift`+`Mouse1`) ·
parry (`Mouse2` bosish, 180 ms oyna) · blok (`Mouse2` ushlash) ·
tepish (`R` — qalqonni ochadi) · chetlanish (`Q`) · yashirin o'ldirish (`V`) ·
nishonni qulflash (`Mouse3`).
Muvaffaqiyatli parry raqibning pozasiga +35 beradi — bu jangning yuragi.

**Dushmanlar — 6 tur**, har biri o'z statistikasi va xulqi bilan:
Footman · Sergeant (qalqonli, tepish kerak) · Crossbow · Assassin ·
HorseArcher · Elite. Ular mavsum va qiyinlikka qarab tanlanadi.

Sezish AC modelida ikki kanal: **ko'rish konusi** (o'yinchi cho'kkalasa 55%,
past profilda 85% masofa) va **eshitish** (Character chiqargan shovqin).
Ogohlik: tinch → shubha (bosh ustida to'lib boruvchi zarhal kvadrat) → ogoh (qizil).
AC dagidek bir vaqtda faqat **2 ta dushman hujum qiladi**, qolganlari aylanib kutadi.

**Epizod halqasi — to'liq mantiq:**

```
Epizod tanlandi -> yuklash -> CUTSCENE (intro)
   |
   v
BRIFING (2.5 s) -> JANG -> to'lqin tozalandi -> NAZORAT NUQTASI -> keyingi to'lqin
   |                                                                    |
   | o'yinchi halok bo'ldi                                              v
   v                                                             EPIZOD BAJARILDI
HALOK BO'LDINGIZ                                                        |
   |-- Nazorat nuqtasidan qayta urinish  (joriy to'lqin yangidan)       v
   |-- Epizodni boshidan                                        keyingi epizod ochiladi
   `-- Bosh menyuga                                             saves/progress.json
```

Maqsadlar epizod ARXETIPIDAN quriladi (episodes_v2.json dagi `objectives` bo'sh):
SIEGE/DEFENSE/SURVIVAL → barcha dushmanlarni yo'q qilish + omon qolish;
INFILTRATION → sezilmaslik (ixtiyoriy) + nuqtaga yetish + jang;
CHASE → nuqtaga yetish + N dushman; INVESTIGATION/COURT → nuqta + jang.
To'lqinlar soni qiyinlik darajasidan, dushmanlar soni `max_simultaneous` dan olinadi.

**Taraqqiyot** — `saves/progress.json`: bajarilgan epizodlar, o'limlar, jami
o'ldirilganlar. Epizod ro'yxatida bajarilgani feruza kvadrat bilan, ochilmagani
so'nik ko'rsatiladi va uni boshlab bo'lmaydi.

**Animatsiya** — modellarda skelet yo'q (statik OBJ), shuning uchun
`SkinnedModel` chegaraviy quti bo'yicha **avtomatik rigging** qiladi: 12 virtual suyak,
chegaralarda yumshoq og'irlik, CPU skinning (50k verteks ≈ 0.8 ms, standart 30 Hz).
35 ta klip: harakat (Idle/Walk/Run/Crouch*), muloqot (Talk/Listen/Point/Salute),
parkur (Vault/Mantle/ClimbUp/Hang/Shimmy/Slide/Roll/Fall/WallRun/Dodge) va
jang (AttackLight1-3/AttackHeavy/Kick/Block/ParryHit/Hurt/Stagger/Death/
BowAim/BowShoot/Execute/Assassinate).

**Xarita** — `data/levels/*.json`: fbm shovqinli relyef (markazda tekis maydon),
verteks rangi qiyalik va balandlikka qarab o't/tuproq/tosh orasida aralashadi,
protsedural teksturalar, 68 ta qo'lda qo'yilgan rekvizit + determinatsiyalangan "scatter"
(daraxt, tosh, buta, gul), doiraviy to'qnashuv, yumshoq soyalar, kun vaqti va ob-havo presetlari.

**Yoritish** — yo'naltirilgan quyosh + osmon gumbazidan to'ldiruvchi yorug'lik (GL_LIGHT1),
chiziqli tuman, gradient osmon gumbazi va quyosh diski.

---

## 3. Papka tuzilishi

```
game/include/ertugrul/     header'lar = modullar orasidagi kontrakt
  core/Math.h              Vec3, Mat4, easing, Rng, fbm shovqini
  gfx/Texture.h            GDI+ dekodlash, kesh, protsedural teksturalar
  gfx/Font.h               glif atlasi, UTF-8, 2D chizish yordamchilari
  gfx/Mesh.h               OBJ + MTL, verteks massivlari, display list
  gfx/Skin.h               avtomatik rigging + animatsiya klipari
  audio/Audio.h            waveOut aralashtirgich (5 shina)
  audio/Voice.h            VO banki + SAPI zaxirasi
  loc/Loc.h                CSV lokalizatsiya (uz/tr/en)
  game/Episodes.h          48 epizod bazasi
  game/Cutscene.h          sahna formati + rejissyor
  world/Terrain.h          balandlik xaritasi
  world/Level.h            rekvizitlar, to'qnashuv, osmon
  world/Physics.h          parkur zondlari (chekka, to'siq, devor, tom)
  game/Character.h         AC uslubidagi holat mashinasi
  game/Combat.h            resurslar (sog'liq/nafas/poza/qo'l) va zarba jadvali
  game/Enemy.h             6 dushman turi, AI, sezish, EnemyManager
  game/Encounter.h         maqsadlar, to'lqinlar, nazorat nuqtasi, taraqqiyot
  game/Projectile.h        ballistik o'q, tana zonalari, ArrowPool
  audio/Sfx.h              protsedural jang tovushlari (9 ta)
  ui/Menu.h                menyu ekranlari
  app/App.h, Input.h       holat mashinasi, kiritish
  app/Bindings.h           sozlanadigan klavish bog'lamalari
game/src/                  amalga oshirish (24 ta .cpp)
game/legacy/               eski prototip (qurilishga kirmaydi)
data/episodes/             episodes_v2.json (48 epizod)
data/cutscenes/            48 ta sahna (har epizod uchun)
data/levels/               oba_camp, forest_pass, aleppo_road, sogut_village
localization/              4 ta CSV, 840 kalit x 3 til
assets/models/             OBJ modellar (ottoman, crusader, nature, town)
assets/audio/vo/           927 ta WAV (309 replika x 3 til)
scripts/                   build.ps1, run-game.ps1, gen_voice.ps1
tools/                     fix_cutscene_staging.py
```

---

## 4. Ovoz fayllarini tayyorlash

```powershell
.\scripts\gen_voice.ps1 -Lang uz
.\scripts\gen_voice.ps1 -Lang "tr,en" -Force
```

Skript cutscene replikalarini o'qib, Windows SAPI orqali WAV yaratadi.
Bu kompyuterda faqat **Microsoft Zira (en-US)** va **Microsoft Irina (ru-RU)** ovozlari bor —
o'zbek va turk ovozi yo'q. Shuning uchun `uz`/`tr` uchun matn lotindan kirillga
transliteratsiya qilinib, rus ovozi bilan o'qiladi (fonetik jihatdan ancha yaqin).
WAV bo'lmasa o'yin jonli TTS ga tushadi, u ham bo'lmasa faqat subtitr ko'rsatiladi.

---

## 5. Diagnostika

```powershell
.\build\ertugrul.exe --check          # kontent, tarjima va ovoz qamrovi
python tools\fix_cutscene_staging.py --check   # kamera rekvizitga urilmasligini tekshirish
```

Nosozliklarni izlash uchun muhit o'zgaruvchilari:

| O'zgaruvchi | Nima qiladi |
|---|---|
| `ERT_MENU_LOG=1` | menyu ekrani va tanlov o'zgarishini yozadi |
| `ERT_MOUSE_LOG=1` | har bir `WM_MOUSEMOVE` ni yozadi |
| `ERT_NO_SKIN=1` | skinningni o'chiradi (xom mesh) — artefaktlarni ajratish uchun |
| `ERT_SKIN_STATS=1` | har suyak uchun eng katta verteks siljishini yozadi |
| `ERT_MOVE_LOG=1` | personaj holati, pozitsiyasi, profili va shovqinini yozadi |
| `ERT_PHYS_DRAW=1` | parkur qutilarini simli chizadi (feruza — chiqiladigan, zarhal — sakraladigan) |
| `ERT_MESH_DUMP=1` | har mesh uchun material/tekstura ma'lumotini yozadi |
| `ERT_NO_SHADOWS=1` `ERT_NO_PROPS=1` `ERT_FLAT_PROPS=1` `ERT_NO_LISTS=1` | render nosozliklarini ajratish uchun |

### Interfeys skrinshotlari

```powershell
.uild\ertugrul.exe --lang uz --shots shots
```

O'yin o'zi menyu -> til -> epizodlar -> sozlamalar -> boshqaruv -> cutscene -> o'yin
bo'ylab yuradi va har bosqichda `shots\NN_*.png` saqlaydi.
Oynani old planga chiqarmaydi, shuning uchun fonda boshqa dastur ishlayotgan
bo'lsa ham ishlaydi.

## Ovoz — personajga qarab erkak/ayol/bola

Ilgari Windows SAPI ishlatilardi: bu kompyuterda faqat Zira (en) va Irina (ru)
ovozlari bor, shuning uchun o'zbek va turk matnlari KIRILLGA o'girilib RUS ovozi
bilan o'qilardi. Natijada talaffuz noto'g'ri edi va **hamma personaj — hatto
Ertug'rul ham — ayol ovozida gapirardi**.

Endi `tools/gen_voice.py` ishlatiladi:

| Til | Erkak | Ayol | Bola |
|---|---|---|---|
| uz | uz-UZ-SardorNeural | uz-UZ-MadinaNeural | Madina, ohang +22 Gts |
| tr | tr-TR-AhmetNeural | tr-TR-EmelNeural | Emel, ohang +22 Gts |
| en | en-US-AndrewNeural | en-US-AvaNeural | en-US-AnaNeural (haqiqiy bola ovozi) |

Ovozlar `edge-tts` (Microsoft neural) orqali olinadi — **bepul, API kalit kerak emas**.

**Tarmoq kerak emas.** Ovozlar bir marta yaratilib `assets/audio/vo/<til>/`
ga WAV qilib yoziladi va o'yin faqat shu fayllarni diskdan o'qiydi.
edge-tts yoki AISHA FAQAT generatsiya paytida kerak, o'yin ishlaganda emas.

| Til | Fayl | Davomiylik |
|---|---|---|
| uz | 304 | 24.6 daqiqa |
| tr | 304 | 28.1 daqiqa |
| en | 304 | 18.5 daqiqa |

Jami 912 fayl, 71 daqiqa, 198 MB (24 kGts mono 16-bit). Tekshirildi: 0 nosoz
fayl, uchala tilda ham SAPI zaxira yo'liga hech qachon tushmaydi.
Agar biror WAV yo'qolsa, o'yin konsolda ochiq ogohlantirish beradi —
jimgina noto'g'ri ovoz bilan o'qimaydi.

`data/voice_cast.json` har personajning rolini va ohangini belgilaydi:
Ertug'rul bosiq (-8 Gts, -4%), Turgut qalin (-16 Gts), Bamsi shovqinli (+7, +9%),
No'yon sovuq va sekin (-11, -9%), Ibn Arabiy juda sekin (-13%). Ro'yxatda yo'q
personajning ohangi uning id sidan determinatsiyalangan tarzda hisoblanadi —
shunda 40 ta erkak personaj bir xil ovozda gapirmaydi.

**AISHA (aisha.group)** ham qo'llab-quvvatlanadi (`--engine aisha`), lekin
uning o'zbek TTS sida **faqat bitta ayol ovozi bor (Gulnoza)** va turkcha
umuman yo'q. Shuning uchun standart engine — edge-tts. Kalitlar repoga
yozilmaydi, `--aisha-keys <fayl>` orqali beriladi.

## Epizod tartibi va cutscene bibliyasi

Epizodlar `data/episodes/episodes_v2.json` da qat'iy xronologik: 1227 → 1261,
orqaga sakrash yo'q. Menyu endi butun serial bo'ylab **01–48** raqamlaydi (ilgari
har mavsum 01 dan boshlanib, to'rtta "01" ko'rinardi) va har qatorda yil turadi.

`tools/apply_scene_bible.py` har cutscene'ga epizod ma'lumotidan **joy, vaqt va
ob-havo** beradi va **kanonik aktyor jadvalini** (`data/cast_models.json`) qo'llaydi:

| Nima aralashgan edi | Endi |
|---|---|
| Söğüt epizodlari (39, 42, 44, 48) Qayi obasida | `sogut_village`, qishloq ko'chasi |
| Mo'g'ul / harbiy lagerlar (32, 34, 36, 46, 47) Qayi obasida | `oba_camp` (begona lager) |
| 14 aktyor sahnadan sahnaga model almashtirardi | har aktyor bitta model, bo'y va tint |
| Ko'p aktyor Kenney'ning **zamonaviy** kubik odamchalari (gamepad futbolkasi, politsiya formasi) | 10 ta tarixiy teksturali model |
| EP006 (Sultan Han) cutscene'i yo'q edi | yozildi: 4 aktyor, 7 replika, 3 tilda ovoz |

Model fayl nomlari tashqi ko'rinishga mos kelmasligi aniqlandi (masalan
`medieval+warrior` — ko'k kamonchi, `fantasy+warrior` — boltali pahlavon), shuning
uchun jadval fayl nomi bo'yicha emas, **o'yin ichida chizib ko'rib** tuzildi:
Turgut — boltali pahlavon, Dog'on — kamonchi, beklar — qizil kaftan, sultonlar —
tojli zirh, shayxlar — sallali keksa, Templarlar — oq surko, No'yon — mo'ynali jangchi.

### Ayollar va bolalar — alohida modellar

Ilgari to'rt ayol bitta `ornate+queen` modelini faqat tint bilan bo'lishar, bolalar
esa kattalar modelining 1.1 m ga kichraytirilgani edi (bosh ham, oyoq ham katta odam
nisbatida — "mitti erkak" ko'rinardi). Endi:

| Aktyor | Model | Farq |
|---|---|---|
| Halima Sulton | `assets/models/woman_halime.obj` | firuza ko'ylak, tilla hoshiya |
| Oyqiz | `woman_aykiz.obj` | binafsha-qizil ko'ylak, yosh qiz |
| Hayma Ona | `woman_hayme.obj` | jigarrang keng rido, ro'mol (keksa) |
| Gulbahor | `woman_gulbahor.obj` | yashil rido (keksa) |
| Bolalar (yetim bola, bola, toshkash bola, Usmon, al-Aziz) | kattalar modeli + `"proportions": "child"` | oyoq ×0.72, gavda ×0.90, **bosh ×1.28**, en ×0.92 |

Har ayol modeli o'z 2048² basecolor teksturasiga ega (`*_basecolor.jpg`) — bir xil
mesh, lekin rang/gazlama boshqa, shuning uchun bir sahnada ikki ayol ajralib turadi.
Bola nisbatlari `Mesh::getVariant(path, "child")` orqali CPU da bir marta deformatsiya
qilinadi va keshlanadi (`Mesh.cpp: deformChild`); cutscene JSON da aktyorga
`"proportions": "child"` yoziladi, buni `tools/apply_scene_bible.py` avtomatik qo'yadi.

Tekshiruv: `--film` bilan EP010 (Oyqiz), EP015 (Halima), EP005 (Hayma Ona),
EP027 (Gulbahor + bola), EP016 (yetim bola), EP048 (Usmon) suratga olindi — har
ayol boshqa rangda, bolalar kattalar yonida qisqa va katta boshli.

Cutscene kamerasi endi **yer sathiga nisbatan** cheklanadi — tepalikli darajalarda
kamera yer ostiga tushib, ekranda faqat o't ko'rinardi.

## Demo xarita — "Qayi obasi, vodiy"

`data/levels/oba_valley.json` — kinematik demo xarita, `tools/make_valley_level.py`
bilan generatsiya qilinadi.

| Element | Tafsilot |
|---|---|
| Kompozitsiya | Markazda tekis maydon, atrofda tepaliklar — ufq ramkalanadi |
| O'tovlar | Ikki halqa (12 + 16), eshiklar markazga qaraydi, markazda bey chodiri |
| Yoritish | Yangi `golden` (oltin soat) preseti: quyosh past, muhit yorug'ligi kuchli |
| Hajm | 134 prop + 1 750 scatter = ~1 884 obyekt, 1 457 to'qnashuv qutisi |

Yo'l-yo'lakay ikkita aktiv nuqsoni tuzatildi:

1. **Material ranglari.** Kenney nature to'plamining `grass`, `leafsGreen`,
   `leafsDark`, `stone`, `stoneDark` materiallarida Kd ning R va B kanallari
   almashib ketgan edi — o't feruza (0.17, 0.85, 0.72), barglar ko'kimtir chiqardi.
   `tools/fix_material_colors.py` 51 faylda 54 ta materialni tuzatdi
   (zaxira nusxa: `<fayl>.mtl.bak`).
   Daraja JSON dagi `tint` bu muammoni yecha olmasdi: `Mesh::draw()` har submesh
   uchun `glColor4f(sm.kd)` chaqirib prop tintini bekor qiladi, ustiga mesh
   display list ga "pishiriladi".
2. **Chodir rangi.** Kenney chodirining materiali yorqin qizil (`colorRed`).
   `tools/make_felt_tents.py` kigiz rangli variantlar yaratadi
   (`yurt_felt_*.obj/.mtl`) — geometriya o'zgarmaydi, faqat ranglar.

Relyef ranglari ham yorqinlashtirildi: oba maydonining tuprog'i ilgari past
muhit yorug'ligida qop-qora bo'lib ko'rinardi.

## Taqdimot va brend

| Fayl | Nima |
|---|---|
| `docs/Blades_of_Anatolia_Ertugrul.pptx` | Loyiha taqdimoti, 24 slayd |
| `docs/Blades_of_Anatolia_demo.mp4` | Demo video, 2:55, 1280x720, 30 k/s |
| `docs/brand/logo_mark.png` | Belgi (kvadrat) |
| `docs/brand/logo_full.png` | Vertikal qulf |
| `docs/brand/logo_wordmark.png` | Gorizontal qulf |
| `docs/brand/logo_generate.py` | Logoni qayta yaratadi |
| `docs/brand/deck_generate.py` | Taqdimotni qayta quradi |
| `docs/brand/video_generate.py` | Videoni qayta yig'adi |

Video o'yin ichidan yozib olinadi: `--film <papka> --filmpart 1|2|3 --filmlen <s>`.
Kinematik planlar (kran, o'rta, yaqin, past burchak) film ssenariysida
`filmCamera()` orqali boshqariladi. 2-qismda `filmSustain()` o'yinchining
sog'lig'ini ushlab turadi — bu FAQAT trailer yozish uchun, o'yin balansiga tegmaydi.
Bu rejim vaqtni QAT'IY 1/30 s qadam bilan yuritadi, shuning uchun natija
mashina tezligiga bog'liq emas va har safar bir xil chiqadi.

## Unity'ga ko'chirish

`D:\My project` (Unity 6, URP) ga kontent oqimi tayyor: modellar, ovozlar, JSON'lar, CSV'lar
nusxalandi; `--export-unity` buyrug'i darajani relyef + rekvizit + spawn + osmon JSON'iga
chiqaradi, [tools/unity/ErtugrulLevelImporter.cs](tools/unity/ErtugrulLevelImporter.cs) undan
Unity sahnasi quradi. Holat va yo'l xaritasi: [docs/UNITY_PORT.md](docs/UNITY_PORT.md).

## Ma'lum cheklovlar

Bular hali bajarilmagan — halol ro'yxat.

| Cheklov | Tafsilot |
|---|---|
| Fayldan skelet import yo'q | .obj da suyak yo'q; skelet o'yin ichida ta'riflanadi (`<model>.rig.json`, pastga qarang). FBX/glTF skelet importi yo'q — bu ataylab (tashqi kutubxonasiz) |
| Metall/roughness/normal xaritalari yo'q | Modellarda faqat basecolor bor (Tripo). Yorug'lik modeli esa endi PBR (pastga qarang); material xaritalari paydo bo'lsa shaderga qo'shish 20 qator |
| Faqat Windows | Ataylab (tashqi kutubxonasiz). Nima kerakligi aniq o'lchandi: kodning ~25% i (oyna, audio chiqishi, rasm dekodlash, shrift), ~1 hafta — [docs/PORTING.md](docs/PORTING.md) |

Tuzatilganlar (shu bosqichda): jang jadvalidagi yashirin `x0.70` poza koeffitsiyenti,
bepul parry, dushman blokining burchak tekshiruvi va zarbaning balandlik tekshiruvi.

Yopilganlar (2026-09):

| Edi | Endi |
|---|---|
| O'q yig'ib bo'lmasdi — yerga qadalgani 6 s da yo'qolar, tanaga tekkani shunchaki o'chardi | Yerdagi va **jasad yonidagi** o'q 40 s yotadi (so'nggi 5 s so'nadi), yorug'lik bilan "nafas oladi"; ustidan o'tsangiz sadoqqa qaytadi — HUD hisoblagichi chaqnaydi, yog'och "tak" tovushi (`SfxId::Pickup`) |
| Tepalikda qadam tekis yerdagidek — oldingi panja yerga botar, orqadagisi havoda qolardi | `Character` harakat yo'nalishida 0.9 m bazada nishabni o'lchaydi (`slope_`), `Skin` panja nishonini `fy += fz·slope` ko'taradi, tana qiyalikka egiladi. Faza hamon GORIZONTAL masofadan haydaladi — nishabda ham SLIP 0% |
| "Sukunat kamerani sekinlashtiradi" | Aslida allaqachon yo'q edi: kamera `im.lastDt` (haqiqiy dt) bilan, dunyo `gdt` bilan yuradi — jadval eskirgan ekan |
| Git Bash dan ishga tushirilsa segfault | Sabab topildi: exe `libstdc++-6.dll` ni PATH dan qidirar, Git Bash esa `C:/Program Files/Git/mingw64/bin` dagi BOSHQA versiyani berardi (App::loadConfig ichida std::string ABI mos kelmay yiqilardi). Endi runtime statik (`-static -static-libstdc++`), exe MinGW DLL siz ishlaydi — Git Bash, PowerShell, Explorer |
| Shader yo'q — soya xaritasi imkonsiz | **Shadersiz soya xaritasi**: `GL_ARB_depth_texture` + `GL_ARB_shadow` + multitexture (GL 1.4). Quyosh nuqtai nazaridan chuqurlik orqa buferga chiziladi, `glCopyTexSubImage2D` bilan teksturaga olinadi, EYE_LINEAR texgen orqali 1-birlikda apparat solishtiruvi (2x2 PCF), 2-birlik rangni koeffitsiyentga ko'paytiradi. Chodir, devor, daraxt, personaj — hammasi bir-biriga va relyefga soya tashlaydi. `ERT_NO_SHADOWMAP=1` eski yumshoq disklarga qaytaradi ([ShadowMap.cpp](game/src/gfx/ShadowMap.cpp)) |
| Quti (Y-band) rigi: qo'l tana bilan, son chanoq bilan bir gorizontal chiziqda ajralib, yelka va sonda qirqilish | **Segment rigi** ([Skin.cpp](game/src/gfx/Skin.cpp)): har suyak — bo'g'imdan uchigacha KESMA; verteks eng yaqin kesmaga, ikkinchi og'irlik esa unga qo'shni (ota/bola) suyakka masofa farqi bo'yicha beriladi (`blend·h` kenglikda silliq). Bo'g'im va uch nuqtalari har model uchun `<model>.rig.json` da (misol: [ottoman.rig.json](assets/models/ottoman/ottoman.rig.json)), bo'lmasa standart jadval. `ERT_RIG=box` eski rigga qaytaradi, `ERT_RIG_LOG=1` verteks taqsimotini chiqaradi |
| Shader yo'q — PBR imkonsiz | **GLSL 1.20 orqali PBR-lite**, tashqi kutubxonasiz: shader funksiyalari `wglGetProcAddress` bilan drayverdan olinadi ([Pbr.cpp](game/src/gfx/Pbr.cpp)). Piksel darajasida Cook-Torrance GGX (D·G·F), Schlick Fresnel, energiya saqlovchi diffuz, soya xaritasi shader ichida (`sampler2DShadow`, PCF), tuman. Fixed-function HOLATI (gl_LightSource, gl_Fog, gl_Color, texgen) o'zgarishsiz — display listlar, Level::applyLighting, ShadowMap avvalgidek. Material: relyef mot (0.92), rekvizit 0.72, personaj 0.55. Soyada quyosh hadi 0, osmon/ambient hadi `shadowLevel` gacha (sky occlusion taqlidi). `ERT_NO_PBR=1` → GL 1.1 yo'li, `ERT_PBR_DBG=1` soya koeffitsiyenti, `=2` soya koordinatalari |

O'q yig'ish mantig'i o'yin tashqarisida ham sinaladi (soxta fizika, tekis yer):

```bash
D:/gcc/mingw64/bin/g++ -std=c++20 -Igame/include tools/tests/arrow_pickup_test.cpp game/src/game/Projectile.cpp game/src/core/Math.cpp -lopengl32 -o build/arrow_pickup_test.exe && build/arrow_pickup_test.exe
```
