# Diriliş: The Last March — Backend (Online Meta-Xizmatlar)

> **Loyiha:** *Diriliş: The Last March* — Ertug'rul haqidagi tarixiy action-adventure.
> 48 epizod, 4 mavsum, 1227–1261-yillar.
> **Dvijok:** Unreal Engine 5 (C++) · **Backend:** Java 21 + Spring Boot 3.3
> **Paket:** `com.fayzinc.ertugrul`

---

## 0. Eng muhim qoida

O'yin **to'liq single-player**. Bu servis **gameplay netcode emas** — unda
matchmaking ham, PvP ham, dedicated server ham yo'q.

> ### ⚠️ Backend hech qachon o'yinni to'xtatmaydi.
> Server butunlay o'chib qolsa ham, o'yinchi 48 epizodni boshidan oxirigacha
> o'ynay oladi. U faqat quyidagilarni yo'qotadi: qurilmalararo sinxron,
> Safar Daftari eksporti, live-ops balansi va global statistika.

Bu qoida arxitekturaning har bir qatlamiga singdirilgan:

| Komponent | Yiqilsa nima bo'ladi |
|---|---|
| Kafka | Telemetriya yo'qoladi, so'rov **muvaffaqiyat** qaytaradi |
| Redis | Rate limit **ochiq** qoladi (fail-open), kesh chetlab o'tiladi |
| MinIO/S3 | Save yuklanmaydi, klient lokal save bilan davom etadi |
| Butun servis | O'yin offline rejimda to'liq ishlaydi |

---

## 1. Arxitektura

```
                    ┌──────────────────────────────────┐
                    │   UE5 KLIENT (PC / PS5 / Xbox)   │
                    │   offline-first, lokal save      │
                    └────────────────┬─────────────────┘
                                     │ HTTPS + Bearer JWT
                                     ▼
        ┌────────────────────────────────────────────────────────┐
        │              ertugrul-backend (Spring Boot)            │
        │                                                        │
        │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────┐ │
        │  │ identity │  │   save   │  │  codex   │  │journey │ │
        │  │ JWT,     │  │ vector   │  │ 180 ta   │  │ Safar  │ │
        │  │ device,  │  │ clock,   │  │ yozuv,   │  │ Daftari│ │
        │  │ store    │  │ 8+1 slot │  │ union    │  │ PDF    │ │
        │  └──────────┘  └──────────┘  └──────────┘  └────────┘ │
        │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────┐ │
        │  │telemetry │  │ liveops  │  │  stats   │  │integr. │ │
        │  │ funnel,  │  │ remote   │  │ tanlov   │  │ HMAC,  │ │
        │  │ mix      │  │ config,  │  │ taqsimoti│  │ rate   │ │
        │  │ balansi  │  │ A/B      │  │          │  │ limit  │ │
        │  └──────────┘  └──────────┘  └──────────┘  └────────┘ │
        └───┬─────────────┬────────────┬────────────┬───────────┘
            │             │            │            │
            ▼             ▼            ▼            ▼
     ┌────────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐
     │ PostgreSQL │ │  Redis   │ │  Kafka   │ │ MinIO / S3   │
     │ 16         │ │  7       │ │  3.8     │ │              │
     │            │ │          │ │          │ │ ertugrul-    │
     │ metadata,  │ │ rate     │ │ ert.tele │ │  saves       │
     │ progress,  │ │ limit,   │ │ metry.raw│ │  (xususiy)   │
     │ rollup'lar │ │ config   │ │          │ │ ertugrul-    │
     │            │ │ keshi    │ │          │ │  exports     │
     │ Flyway     │ │          │ │          │ │  (ommaviy)   │
     └────────────┘ └──────────┘ └────┬─────┘ └──────────────┘
                                      │
                                      ▼
                              ┌───────────────┐
                              │  Warehouse    │
                              │  (ETL, tashqi)│
                              └───────────────┘
```

### 1.1 Ma'lumot qayerda saqlanadi va nima uchun

| Ma'lumot | Qayerda | Nega aynan u yerda |
|---|---|---|
| Save **blob** (8 MiB gacha) | S3/MinIO | Postgres'da bo'lsa WAL shishadi, backup soatlab cho'ziladi va blob trafigi metadata so'rovlarini sekinlashtiradi |
| Save **metadata** + vector clock | PostgreSQL | Kichik, indekslangan, tranzaksion |
| Kodeks progressi | PostgreSQL | O'suvchi to'plam, konfliktsiz |
| Kodeks **matnlari** | CDN (JSON) | Live-ops yangi yozuvlarni **patchsiz** qo'shadi |
| Xom telemetriya | Kafka → Warehouse | Postgres'ga yozilsa, analitika *save* xizmatini bo'g'adi |
| Telemetriya rollup'lari | PostgreSQL | Live-ops paneli o'qiydi, hajmi kichik |
| Rate limit hisoblagichlari | Redis | Efemer, qayta tiklanadi |
| Safar Daftari PDF | S3 (ommaviy bucket) | Havola o'yin sotib olmagan odamga ham ochilishi kerak |

---

## 2. Modullar

### 2.1 `identity` — akkaunt va autentifikatsiya

O'yinchi **"O'ynash" tugmasini bosadi va hech narsa yozmasdan** anonim,
qurilmaga bog'langan akkaunt oladi. Ro'yxatdan o'tish oynasi ko'rsatilmaydi —
bu birinchi 60 soniyada o'yinchini yo'qotadigan eng keng tarqalgan sabab.

- **Access token** — RS256 JWT, 30 daqiqa. Ichida o'yin holati **yo'q**.
- **Refresh token** — JWT emas, oddiy tasodifiy satr, 30 kun. Serverda faqat
  SHA-256 hash'i saqlanadi.
- **Rotatsiya:** har refresh yangi token beradi. Allaqachon ishlatilgan token
  qayta kelsa — o'g'irlik deb hisoblanadi va **butun oila** bekor qilinadi.
- **Entitlement** (Steam/PSN/Xbox) — DRM emas. U anonim akkauntni
  *tiklanadigan* qiladi. ⚠️ Tekshirish hozircha **stub**
  (`EntitlementVerifier`), chunki platforma SDK kalitlari nashriyot
  shartnomasidan keyin beriladi.

### 2.2 `save` — bulutli saqlash ⭐

Servisdagi **eng nozik mantiq**. 8 ta qo'lda saqlash sloti + 0-slot avtosave.

**Muammo:** o'yinchi PC'da EP023 gacha o'ynadi. PS5 esa internetsiz qolib,
EP021 dan davom etdi. Qaysi biri "to'g'ri"?

**Yechim — ikki bosqichli:**

```
   1. VECTOR CLOCK hukm qiladi
      ├── DESCENDS   → oddiy davomi, qabul qilinadi
      ├── PRECEDES   → klient orqada, "avval tortib ol" deyiladi
      ├── IDENTICAL  → takroriy yuklash, hech narsa yozilmaydi
      └── CONCURRENT → tarixlar ajralgan → 2-bosqich
                                              │
   2. LAST-WRITE-WINS (faqat CONCURRENT da)   ▼
      ├── a) PROGRESS  — epizod raqami, keyin o'ynalgan vaqt
      ├── b) VAQT      — progress teng bo'lsa
      └── c) DETERMINIZM — hammasi teng bo'lsa (takroriy urinish
                           har doim bir xil natija berishi shart)
```

> ### 🔴 Eng muhim qoida: LWW hech qachon ma'lumot o'chirmaydi.
> Mag'lub tomonning blob'i **saqlanib qoladi** va o'yinchiga
> *"PS5 dagi saqlash — EP021, 9 soat 12 daqiqa. Tiklaysizmi?"* deb ko'rsatiladi.
> Single-player o'yinda 40 daqiqalik progressni jimgina yo'qotish — tuzatib
> bo'lmaydigan xato.

**Nega avtomatik birlashtirish (merge) emas?** Save blob'i — bog'langan grafik:
kim tirik, qaysi qal'a kimda, `hand_integrity` qancha. Ikki tarixni maydon
bo'yicha birlashtirsak, mantiqan **imkonsiz dunyo** hosil bo'ladi: Titus ham
kechirilgan, ham o'ldirilgan.

**Tranzaksiya chegarasi.** Blob object store'ga **tranzaksiyadan tashqarida**
yoziladi, metadata esa qisqa, qulflangan tranzaksiyada
(`SaveService` → `SaveCommitService`). Tartib qat'iy: **avval blob, keyin
metadata** — teskarisi o'yinchi uchun buzilgan save degani.

### 2.3 `codex` — tarix qatlami

~180 ta tarixiy yozuv, 8 kategoriya. Har birida **ishonchlilik darajasi**:

| Belgi | Daraja | Ma'nosi |
|---|---|---|
| ✅ `DOCUMENTED` | Hujjatli | Zamondosh yoki ishonchli manbada tasdiqlangan |
| ⚠️ `DISPUTED` | Bahsli | Manbalar ziddiyatli, olimlar rozi emas |
| 📖 `LEGEND` | Rivoyat | Keyingi asr an'anasi yoki badiiy ixtiro |

Sinxron **arzimas darajada sodda**, chunki kodeks progressi — *o'suvchi to'plam*
(grow-only set): yozuv ochilgach hech qachon qayta qulflanmaydi. Matematik
jihatdan bu CRDT, ya'ni **konflikt bo'lishi mumkin emas** va vector clock
kerak emas — oddiy birlashma yetadi.

> **Eng muhim metrika:** *ochilgan, lekin o'qilmagan* yozuvlar soni. Agar
> o'yinchilar kodeksni ochib, o'qimasa — tarix qatlami ishlamayapti.

### 2.4 `journey` — Safar Daftari

O'yinchining **o'z ovozida** avtomatik yoziladigan shaxsiy kundaligi.

- **Nega serverda:** qurilmalararo sinxron, PDF eksport, va **ommaviy havola** —
  havolani ochgan odam o'yinni sotib olmagan bo'lishi mumkin. Uchinchisi
  marketing xususiyati va uni save blob ichida saqlab bo'lmaydi.
- **Ikki taqvim:** har sahifada hijriy va milodiy sana yonma-yon.
- **PDF eksport** asinxron (`JourneyExportWorker` navbatni bo'shatadi).
  In-process hand-off ataylab ishlatilmagan: server qayta ishga tushsa ish
  yo'qolardi.

> ⭐ **EP024 dan keyingi sahifalar PDF'da boshqa, beqaror qo'lyozma bilan
> chiziladi.** O'yinchi daftarini qayta o'qiganda qaysi sahifadan keyin qo'l
> o'zgarganini **ko'radi**. Butun jarohat tizimining ma'nosi shu bitta vizual
> detalda yakunlanadi.

### 2.5 `telemetry` — hodisalar oqimi

Hodisa turlari: `EPISODE_START` · `EPISODE_COMPLETE` · `EPISODE_ABANDON` ·
`DEATH` · `CHOICE_MADE` · `CODEX_UNLOCK` · `WOUND_STATE` · `SETTINGS_CHANGED` ·
`INTRO_SKIPPED`

**Ikki qat'iy qoida:**
1. Telemetriya **hech qachon o'yinchini kutdirmaydi** — javob doim `202 Accepted`.
2. Telemetriya **hech qachon so'rovni yiqitmaydi** — Kafka o'chgan bo'lsa ham
   klient muvaffaqiyat oladi.

**Rozilik (consent)** ingest chegarasida qo'llaniladi, quyi oqimda emas:
`OFF` bo'lsa hodisa Kafka'ga **umuman** yozilmaydi (GDPR).

#### ⭐ `WOUND_STATE` — jarohat balansi

Bu — o'yinning imzo mexanikasi va eng katta balans xavfi. U hech qachon to'liq
tuzalmaydi, ya'ni bitta epizoddagi juda tik egri chiziq **o'tib bo'lmaydigan
devorga** aylanadi.

```json
{
  "type": "WOUND_STATE",
  "episodeId": "EP029",
  "wound": {
    "handIntegrity": 23.4, "maxIntegrity": 55.0, "sabr": 41.0,
    "phase": "CHRONIC", "flashbacksThisSession": 3,
    "opiumUsesTotal": 2, "deathsThisEpisode": 4
  }
}
```

**Avtomatik aniqlash** (`EpisodeFunnelService`):

> Agar epizodda o'rtacha `handIntegrity < 15` **VA** o'rtacha o'lim `> 6`
> bo'lsa → epizod `flagged_too_punishing = true` bilan belgilanadi va
> live-ops panelida **qizil** chiqadi.

`below_15_count` alohida sanaladi, chunki **bimodal** epizod ("yarmi
qiynalmoqda, yarmi umuman emas") sog'lom o'rtacha ortida yashirinib qoladi.

### 2.6 `liveops` — masofaviy balans

Konsol patch'i sertifikatsiyadan **1–2 hafta** o'tadi. Live-ops bu vaqtni
**bir soatga** qisqartiradi.

`EpisodeBalanceOverride` har epizod uchun:

| Maydon | Chegara | Nima uchun |
|---|---|---|
| `parryWindowMsDelta` | −60 … +120 ms | Parry — jang tizimining yuragi (bazaviy 180 ms) |
| `enemyDamageScale` | 0.25 … 2.5 | Dushman zarari |
| `woundDecayScale` | 0.2 … 2.0 | ⭐ Epizod "juda qattiq" deb belgilanganda tortiladigan richag |
| `enemyCountScale` | 0.5 … 2.0 | Dushman soni |
| `rolloutPercent` | 0 … 100 | A/B kogorti |

**Chegaralar SQL'da ham qo'yilgan** (`CHECK`): panel orqali kiritilgan xato
qiymat epizodni o'tib bo'lmas qilib qo'ymasligi kerak.

**Kogort taqsimoti barqaror:** `SHA-256(playerId + salt) mod 100`. Saqlanmaydi,
hisoblanadi — shuning uchun o'yinchi sessiyalar orasida A dan B ga sakramaydi.

> ⚠️ `ertugrul.liveops.cohort-salt` ni tajriba davomida **o'zgartirmang** — u
> barcha taqsimotni qaytadan aralashtiradi.

### 2.7 `stats` — "sizning tanlovingiz boshqalarnikiga qanday?"

**Bu leaderboard emas.** Raqobat ham, reyting ham yo'q. Maqsad — epizod
tugagach o'yinchiga **oyna** tutish: *"o'yinchilarning 62% i Titusni kechirgan"*.

Eng qimmatlisi — **7 ta Shubha sahnasi** (`SS_1`..`SS_7`), u yerda o'yinchi
bir-biriga zid tarixiy talqinlar orasidan tanlaydi:

| # | Epizod | Savol |
|---|---|---|
| `SS_1` | EP004 | Otangiz kim? — Sulaymon Shoh / Gunduz Alp |
| `SS_2` | EP012 | Qabilangiz nomi — Qayi / nomsiz turkman jamoasi |
| `SS_3` | EP019 | Kayqubodni kim zaharladi? |
| `SS_4` | EP026 | Sog'ut berildimi yoki olindimi? |
| `SS_5` | EP033 | Köse Dağ'da nechta askar edi? |
| `SS_6` | EP041 | Karacahisar kimniki? |
| `SS_7` | EP048 | Shajarangizni qanday yozasiz? |

**Har o'yinchi bitta ovoz.** NG+ da qayta o'ynash — dizayn maqsadi, lekin agar
har o'tish sanalsa, "62%" raqami o'yinchilar emas, *o'yin seanslari* haqida
bo'lib qoladi. Dedupe `choice_vote_ledger` jadvalida, atomik
`ON CONFLICT DO NOTHING` bilan.

### 2.8 `integrity` — butunlik va suiiste'molga qarshi

**Ikki xil HMAC, butunlay boshqa maqsad uchun:**

| | Server HMAC | Klient HMAC |
|---|---|---|
| Kalit | Faqat serverda | O'yin binary'si ichida |
| Ishonchlilik | ✅ **Ishonchli** | ❌ **Ishonchsiz** (ajratib olish mumkin) |
| Nimani himoya qiladi | Object store'da blob almashtirilishi | — |
| Mos kelmasa | So'rov **rad etiladi** | Faqat **shubha bali** oshadi |

> ### Nega buzilgan save rad etilmaydi?
> O'yin single-player. O'yinchi o'z save'ini o'zgartirsa, u faqat **o'z
> tajribasini** o'zgartiradi — bu uning haqqi. Yagona haqiqiy zarar — buzilgan
> ma'lumot **global statistikani** ifloslantirishi. Shuning uchun jazo bitta va
> aniq: bunday o'yinchining ovozi `ChoiceAggregate` da **sanalmaydi**. Ban yo'q.

**Rate limit** o'yinchi bo'yicha, **IP bo'yicha emas**: konsol o'yinchilari
operator NAT'i ortida o'tiradi va IP bo'yicha cheklash butun mintaqani bitta
suiiste'molchi tufayli bloklardi. Redis + Lua (atomik), **fail-open**.

---

## 3. Ishga tushirish

### 3.1 Talablar

- Docker + Docker Compose
- JDK 21 (lokal ishga tushirish uchun)
- Maven 3.9+ (yoki `./mvnw`)

### 3.2 Tez boshlash (hammasi Docker'da)

```bash
cd backend
docker compose up -d --build

# Sog'liqni tekshirish
curl -s http://localhost:8080/actuator/health | jq
```

### 3.3 Infratuzilma Docker'da, ilova lokal

```bash
# 1. Faqat infratuzilmani ko'tarish
docker compose up -d postgres redis kafka minio minio-init

# 2. Ilovani lokal ishga tushirish
./mvnw spring-boot:run -Dspring-boot.run.profiles=local
```

### 3.4 Xizmatlar

| Xizmat | Manzil | Login |
|---|---|---|
| API | http://localhost:8080 | — |
| Swagger UI | http://localhost:8080/swagger-ui.html | — |
| OpenAPI JSON | http://localhost:8080/v3/api-docs | — |
| Prometheus | http://localhost:8080/actuator/prometheus | `SCOPE_ops` kerak |
| MinIO konsoli | http://localhost:9001 | `ertugrul` / `ertugrul-secret` |
| PostgreSQL | `localhost:5432` | `ertugrul` / `ertugrul` |
| Kafka | `localhost:29092` | — |

### 3.5 Testlar

```bash
./mvnw test                    # unit testlar
./mvnw verify                  # + Testcontainers integratsiya testlari
```

> Integratsiya testlari haqiqiy PostgreSQL va MinIO konteynerlarini ko'taradi.
> Kafka va Redis **ataylab ko'tarilmaydi** — save yo'li ularga bog'liq emas, va
> bu offline-first qoidasining test darajasidagi tasdig'i.

### 3.6 Migratsiyalar

Flyway ilova ishga tushganda avtomatik yuguradi.

| Fayl | Nima yaratadi |
|---|---|
| `V1__init_identity.sql` | `player`, `player_device`, `refresh_token`, `entitlement` |
| `V2__saves_and_codex.sql` | `save_slot`, `save_version`, `codex_progress` |
| `V3__telemetry_and_liveops.sql` | `episode_funnel_daily`, `wound_balance_daily`, `choice_aggregate`, `choice_vote_ledger`, `remote_config`, `episode_balance_override`, `seasonal_event`, `telemetry_rejection` |
| `V4__journey_log.sql` | `journey_entry`, `journey_share`, `journey_export` |

---

## 4. Endpoint'lar

Barcha yo'llar `/api/v1` bilan boshlanadi.
🔓 = autentifikatsiyasiz · 🔒 = Bearer JWT · 👑 = `liveops:write` huquqi

### 4.1 Auth — `/auth`

| Metod | Yo'l | Tavsif | Kirish |
|---|---|---|---|
| `POST` | `/auth/device` | Qurilmaga bog'langan jimgina kirish. Tanish qurilma — mavjud akkaunt, notanish — **yangi anonim akkaunt** | 🔓 |
| `POST` | `/auth/refresh` | Token rotatsiyasi. Ishlatilgan token qayta kelsa — butun oila bekor | 🔓 |
| `GET` | `/auth/me` | Profil va bulut sozlamalari | 🔒 |
| `PATCH` | `/auth/me` | Profilni qisman yangilash (`null` = o'zgartirilmasin) | 🔒 |
| `POST` | `/auth/entitlements` | Steam/PSN/Xbox akkauntini bog'lash | 🔒 |
| `POST` | `/auth/logout-all` | Barcha qurilmalardagi sessiyalarni bekor qilish | 🔒 |
| `DELETE` | `/auth/me` | **GDPR** — ma'lumotlarni o'chirish so'rovi | 🔒 |

### 4.2 Saves — `/saves`

| Metod | Yo'l | Tavsif | Kirish |
|---|---|---|---|
| `GET` | `/saves` | 9 ta slot (0 = avtosave, 1–8 = qo'lda). Bo'shlari ham qaytariladi. **S3'ga bormaydi** | 🔒 |
| `PUT` | `/saves/{slot}` | Save yuklash. Javobdagi `outcome` klientning qarorini belgilaydi | 🔒 |
| `GET` | `/saves/{slot}` | Save yuklab olish (kichik — inline, katta — presigned URL) | 🔒 |
| `POST` | `/saves/{slot}/restore-conflict` | Konfliktda saqlangan nusxani tiklash | 🔒 |
| `POST` | `/saves/{slot}/acknowledge-conflict` | Konflikt oynasini yopish | 🔒 |

**`PUT /saves/{slot}` javob kodlari:**

| Kod | `outcome` | Klient nima qiladi |
|---|---|---|
| `200` | `ACCEPT_INITIAL` / `ACCEPT_FAST_FORWARD` / `ACCEPT_IDENTICAL` | Jimgina davom etadi |
| `409` | `REJECT_STALE` | **Jimgina** serverdan tortib oladi |
| `409` | `CONFLICT_INCOMING_WINS` / `CONFLICT_HEAD_WINS` | ⚠️ **Faqat shu yerda** o'yinchiga oyna ko'rsatiladi |
| `413` | — | Blob hajmi chegaradan oshgan |
| `422` | — | Hash mos kelmadi (buzilgan blob) |

### 4.3 Codex — `/codex`

| Metod | Yo'l | Tavsif | Kirish |
|---|---|---|---|
| `POST` | `/codex/sync` | Ikki yo'nalishli sinxron, bitta so'rovda. Idempotent | 🔒 |
| `GET` | `/codex` | To'liq surat (yangi o'rnatish yoki progressni tiklash) | 🔒 |
| `POST` | `/codex/{codexId}/read` | Yozuv ochilganini qayd etish | 🔒 |
| `PUT` | `/codex/{codexId}/bookmark` | Xatcho'p qo'yish/olish | 🔒 |

### 4.4 Journey (Safar Daftari) — `/journey`

| Metod | Yo'l | Tavsif | Kirish |
|---|---|---|---|
| `POST` | `/journey/entries` | Yangi sahifa yozish. Idempotent | 🔒 |
| `GET` | `/journey/diaries` | O'yinchining barcha daftarlari (NG+ yangi daftar boshlaydi) | 🔒 |
| `GET` | `/journey/diaries/{playthroughId}` | Bitta daftarni to'liq o'qish | 🔒 |
| `PUT` | `/journey/entries/{entryId}/hidden` | Sahifani ulashishdan yashirish | 🔒 |
| `POST` | `/journey/diaries/{playthroughId}/share` | Ommaviy havola yaratish | 🔒 |
| `GET` | `/journey/shares` | Havolalar ro'yxati | 🔒 |
| `DELETE` | `/journey/shares/{shareId}` | Havolani bekor qilish | 🔒 |
| **`GET`** | **`/journey/shared/{token}`** | **Ulashilgan daftarni o'qish** | **🔓** |
| `POST` | `/journey/diaries/{playthroughId}/export` | PDF eksportni navbatga qo'yish | 🔒 |
| `GET` | `/journey/exports/{exportId}` | Eksport holati + yuklab olish havolasi | 🔒 |

> `GET /journey/shared/{token}` — servisdagi **yagona autentifikatsiyasiz
> yo'l**. U ataylab ochiq (marketing) va shuning uchun ataylab juda tor:
> faqat o'qish, faqat token bo'yicha, faqat yashirilmagan sahifalar.
> O'yinchi ID'si, save ma'lumoti yoki boshqa shaxsiy narsa **hech qachon
> chiqmaydi**.

### 4.5 Telemetry — `/telemetry`

| Metod | Yo'l | Tavsif | Kirish |
|---|---|---|---|
| `POST` | `/telemetry/events` | Hodisalar paketi. Doim `202`. `playerId` **token'dan** olinadi | 🔒 |

### 4.6 Live-ops — `/liveops`

| Metod | Yo'l | Tavsif | Kirish |
|---|---|---|---|
| `GET` | `/liveops/config` | Kogortga mos to'liq hujjat. `If-None-Match` bilan `304` | 🔒 |
| `GET` | `/liveops/balance/{episodeId}` | Epizod balansi. Bazaviy bo'lsa `204` | 🔒 |
| `PUT` | `/liveops/admin/balance/{episodeId}` | Balansni o'zgartirish | 👑 |

### 4.7 Stats — `/stats/choices`

| Metod | Yo'l | Tavsif | Kirish |
|---|---|---|---|
| `GET` | `/stats/choices/{choiceId}` | Bitta tanlovning taqsimoti | 🔒 |
| `GET` | `/stats/choices/uncertainty-scenes` | 7 ta Shubha sahnasi — yakuniy ekran | 🔒 |
| `GET` | `/stats/choices/episode/{episodeId}` | Epizoddagi barcha tanlovlar | 🔒 |

> ⚠️ Klient `totalVotes` ni tekshirishi **shart**: namuna kichik bo'lganda
> ("3 ovozda 67%") foiz ishonchli ko'rinadi, lekin hech narsani anglatmaydi.
> Chegara: `ChoiceSplitResponse.MIN_VOTES_TO_DISPLAY`.

---

## 5. Xato formati

Har bir 2xx bo'lmagan javob bir xil konvertda keladi:

```json
{
  "code": "SAVE_CONFLICT",
  "message": "concurrent clocks; resolved on progress",
  "path": "/api/v1/saves/3",
  "traceId": "c0ffee1234abcd",
  "timestamp": "2026-08-27T12:00:00Z"
}
```

> **`code` — API kontraktining bir qismi.** Klient (UE5) xato *matnini*
> o'qimaydi, u **kodga** qarab qaror qabul qiladi. Kodni hech qachon qayta
> nomlamang — yangisini qo'shing.

Muhim kodlar: `SAVE_STALE` · `SAVE_CONFLICT` · `SAVE_CORRUPT` ·
`SAVE_CLOCK_SKEW` · `REFRESH_TOKEN_REUSED` · `RATE_LIMITED` ·
`SHARE_LINK_EXPIRED` · `TELEMETRY_REJECTED`

---

## 6. Domen konstantalari

Bu qiymatlar **o'yin dizaynidan** keladi va serverda ham qat'iy tekshiriladi.

```
Epizodlar     EP001 … EP048          Mavsumlar    S1 … S4
                                     (S1: EP001-012, S2: EP013-024,
                                      S3: EP025-036, S4: EP037-048)

Jarohat fazalari (05_MIH_SYSTEM.md):
  INTACT    EP001–EP023   shift = 100    ← mix hali qoqilmagan
  🩸 EP024  ────────────────────────────  MIX QOQILADI
  FRESH     EP024–EP027   shift = 55     ← va hech qachon qaytmaydi
  CHRONIC   EP028–EP042   shift = 55     ← eng qattiq qism
  ADAPTED   EP043–EP048   shift = 70     ← EP043 protezi: +15

Qiyinlik     LEGEND (Rivoyat) · ALP (default) · FRONTIER (Chegara) · CHRONICLE (Xronika)
Fraksiyalar  SELJUK · AYYUBID · TEMPLAR · MONGOL · AHI · BYZANTINE   (har biri −100…+100)
Shkalalar    hand_integrity 0–100 · max_integrity 0–100 · sabr 0–100 · iman 0–100
Kodeks       CDX_* · DOCUMENTED | DISPUTED | LEGEND · 180 yozuv, 8 kategoriya
Shubha       SS_1 … SS_7
Slotlar      0 = avtosave · 1–8 = qo'lda
```

**Server tekshiradigan imkonsiz holatlar** (`TelemetrySanityChecker`):

| Tekshiruv | Nega imkonsiz |
|---|---|
| `handIntegrity > maxIntegrity` | Shift — ta'rifi bo'yicha shift |
| EP024 dan oldin `phase != INTACT` | Mix hali qoqilmagan |
| EP024 dan keyin `maxIntegrity > 55` (EP043 gacha) | Shift qaytmaydi |
| `INTACT` fazada `maxIntegrity < 100` | Hali hech narsa yo'qotilmagan |
| Epizod ↔ mavsum mos emas | 12 epizod × 4 mavsum |
| `SS_n` noto'g'ri epizodda | Shubha sahnalari qat'iy nuqtalarga bog'langan |

---

## 7. Konfiguratsiya

Barcha sozlamalar `ertugrul.*` prefiksi ostida (`ErtugrulProperties`).

| Kalit | Default | Izoh |
|---|---|---|
| `ertugrul.save.max-manual-slots` | `8` | Qo'lda saqlash slotlari |
| `ertugrul.save.max-blob-bytes` | `8388608` | 8 MiB |
| `ertugrul.save.retained-versions-per-slot` | `10` | Tarix dumi |
| `ertugrul.save.max-clock-skew` | `PT10M` | Klient soatiga tolerantlik |
| `ertugrul.telemetry.max-batch-size` | `200` | Bir paketdagi hodisalar |
| `ertugrul.telemetry.max-event-age` | `P3D` | Undan eski hodisa tashlanadi |
| `ertugrul.liveops.cohort-salt` | `dirilis-2026` | ⚠️ Tajriba davomida **o'zgartirmang** |
| `ertugrul.ratelimit.auth-per-minute` | `10` | Eng qattiq chelak |
| `ertugrul.ratelimit.telemetry-per-minute` | `120` | Eng yumshoq chelak |
| `ertugrul.journey.share-link-ttl` | `P90D` | Ommaviy havola muddati |

### Maxfiy qiymatlar (production'da secret manager'dan)

```bash
ERTUGRUL_JWT_PRIVATE_KEY              # RSA PEM (berilmasa — dev uchun vaqtinchalik generatsiya)
ERTUGRUL_JWT_PUBLIC_KEY
ERTUGRUL_SECURITY_SAVE_HMAC_SECRET    # server imzosi — hech qachon klientga chiqmaydi
ERTUGRUL_SECURITY_CLIENT_HMAC_SECRET  # klient bilan baham ko'riladi (yumshoq signal)
ERTUGRUL_S3_ACCESS_KEY / ERTUGRUL_S3_SECRET_KEY
ERTUGRUL_COHORT_SALT
```

---

## 8. Kuzatuv (observability)

Prometheus metrikalar `/actuator/prometheus` da.

| Metrika | Nima haqida ogohlantiradi |
|---|---|
| `ertugrul_save_upload_total{outcome="conflict"}` | ⭐ O'sish — sinxron oqimi buzilgan |
| `ertugrul_save_upload_total{outcome="stale"}` | Klientlar orqada qolyapti |
| `ertugrul_telemetry_events_total{result="failed"}` | Kafka muammosi |
| `ertugrul_telemetry_events_total{result="rejected"}` | ⭐ Sakrash — odatda **klient bug'i**, cheat emas |
| `ertugrul_ratelimit_throttled_total` | Suiiste'mol yoki juda tor chegara |

**Live-ops paneli uchun SQL:**

```sql
-- Juda qattiq epizodlar (05_MIH_SYSTEM.md §8 chegarasi)
SELECT episode_id, phase, sample_count,
       ROUND((hand_integrity_sum / sample_count)::numeric, 1) AS avg_integrity,
       ROUND((deaths_sum::numeric / sample_count), 2)         AS avg_deaths
FROM wound_balance_daily
WHERE flagged_too_punishing
  AND bucket_date > now() - INTERVAL '7 days'
ORDER BY avg_integrity ASC;

-- Epizod funneli: qayerda tashlab ketishyapti
SELECT episode_id,
       SUM(starts)    AS starts,
       SUM(completes) AS completes,
       ROUND(100.0 * SUM(completes) / NULLIF(SUM(starts), 0), 1) AS completion_pct
FROM episode_funnel_daily
WHERE bucket_date > now() - INTERVAL '30 days'
GROUP BY episode_id
ORDER BY completion_pct ASC
LIMIT 10;
```

---

## 9. Ma'lum cheklovlar (production'gacha bajarilishi kerak)

| # | Nima | Qayerda |
|---|---|---|
| 1 | 🔴 **Entitlement tekshiruvi stub** — platforma SDK kalitlari kerak | `EntitlementVerifier` · `TODO(FAYZ-214)` |
| 2 | 🟠 Anonim telemetriya pseudonym'i `playerId` dan hosil qilinadi — aylanadigan tuz bilan almashtirilishi kerak | `TelemetryIngestService` · `TODO(FAYZ-231)` |
| 3 | 🟠 Kafka **consumer** yozilmagan — `EpisodeFunnelService.accumulate()` chaqiruvchisiz turibdi | `telemetry/funnel` |
| 4 | 🟠 Tungi joblar (versiya tozalash, GDPR o'chirish, eksport muddati) yozilmagan — mantiq servislarda tayyor, scheduler yo'q | `SaveService.pruneOldVersions` va h.k. |
| 5 | 🟡 JWT kalit rotatsiyasi bitta kalit bilan; ikki kalitli JWKS kerak | `SecurityConfig` |
| 6 | 🟡 PNG eksport formati e'lon qilingan, lekin faqat PDF chiziladi | `JourneyExportService` |
| 7 | 🟡 `X-Forwarded-For` ishonchli proxy borligini nazarda tutadi | `RateLimitFilter` |

---

## 10. Kod bo'yicha qoidalar

- **Konstruktor injection.** Maydonda `@Autowired` **yo'q**.
- **DTO'lar — `record`.** So'rov tanalarida `@Valid` / `@Validated`.
- **`@Transactional` chegaralari aniq.** Tarmoq I/O (S3, Kafka) hech qachon
  ochiq tranzaksiya ichida bajarilmaydi.
- ⚠️ **Self-invocation taqiqlanadi.** `@Transactional` yoki `@Async` metodni
  **shu sinfning o'zidan** chaqirish Spring proxy'sini chetlab o'tadi va
  annotatsiya **jimgina ishlamay qoladi**. Shuning uchun quyidagilar alohida
  bean'larga ajratilgan:
  `PlayerRegistrar` · `SaveCommitService` · `JourneyExportWorker`.
- **JavaDoc:** domen izohi — **o'zbekcha**, texnik atamalar — inglizcha.
- **Izoh nima uchunni tushuntiradi, nimani emas.** Kod nima qilishini o'zi
  aytadi; izoh o'yin dizayni qaysi qarorni majbur qilganini aytadi.

---

## 11. Loyiha tuzilishi

```
backend/
├── pom.xml
├── Dockerfile
├── docker-compose.yml
├── README.md                       ← shu fayl
└── src/
    ├── main/
    │   ├── java/com/fayzinc/ertugrul/
    │   │   ├── ErtugrulBackendApplication.java
    │   │   ├── common/     ApiError · ErtugrulException · GlobalExceptionHandler · Auditable
    │   │   ├── config/     Security · Redis · Kafka · S3 · OpenApi · ErtugrulProperties
    │   │   ├── identity/   Player · Device · RefreshToken · Entitlement · Auth* · Jwt*
    │   │   ├── save/       ⭐ VectorClock · ConflictResolver · SaveBlobStore · Save*
    │   │   ├── codex/      CodexEntryProgress · CodexSyncService · CodexCategory
    │   │   ├── journey/    JourneyEntry · Share · Export · ExportService · Worker
    │   │   ├── telemetry/  TelemetryEvent · Producer · IngestService · funnel/
    │   │   ├── liveops/    RemoteConfig · EpisodeBalanceOverride · SeasonalEvent
    │   │   ├── stats/      ChoiceAggregate · ChoiceAggregateService
    │   │   └── integrity/  SaveSigner · RateLimitFilter · TelemetrySanityChecker
    │   └── resources/
    │       ├── application.yml
    │       └── db/migration/  V1 … V4
    └── test/
        ├── java/com/fayzinc/ertugrul/
        │   ├── SaveFlowIntegrationTest.java          (Testcontainers)
        │   ├── save/ConflictResolverTest.java        ⭐ eng muhim test
        │   └── integrity/TelemetrySanityCheckerTest.java
        └── resources/application-test.yml
```

---

## 12. Bog'liq hujjatlar

| Hujjat | Nima uchun kerak |
|---|---|
| `docs/00_AUDIT.md` | Save sxemasi va telemetriya funneli talablari (D6–D8) |
| `docs/02_HISTORY_LAYER.md` | Kodeks modeli, Safar Daftari, 7 ta Shubha sahnasi |
| `docs/04_CORE_SYSTEMS.md` | Parry oynasi, fraksiyalar, qiyinlik darajalari |
| `docs/05_MIH_SYSTEM.md` | ⭐ Jarohat tizimi va `WOUND_STATE` telemetriyasi |
| `docs/07_SETTINGS_HOTKEYS.md` | Akkaunt/bulut sozlamalari, GDPR, sertifikatsiya |

---

*Fayz Inc. — Backend jamoasi · `backend@dirilis-game.com`*
