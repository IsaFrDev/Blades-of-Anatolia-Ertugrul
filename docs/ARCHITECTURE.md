# Arxitektura

GDD (II QISM) dagi modul tuzilishi qanday qilib C++ va Java kodiga tushirilgani.
Loyiha **faqat PC** uchun: Windows (asosiy), Linux/macOS (konsol rejimi).

```
┌───────────────────────────── PC (Windows) ───────────────────────────┐
│  Win32 oyna + OpenGL 3D renderer   yoki   ANSI konsol renderer       │
│         (Renderer / InputSource interfeyslari ortida)                │
└───────────────────────────────┬──────────────────────────────────────┘
                                │
┌───────────────────────────────▼──────────────────────────────────────┐
│                        O'YIN YADROSI (C++20)                          │
│                                                                       │
│  app/        Application (sikl) · Renderer/InputSource interfeyslari  │
│  characters/ PlayerCharacter · Companion · Enemy · Boss · Horse       │
│  components/ vitals · combat · stealth · movement · narrative         │
│  subsystems/ Quest · Dialogue · Oba · Reputation · QTE · Save · Locale│
│  world/      Actor + Component · World (tick, so'rovlar, LOS)         │
│  core/       Types · Vec3 · Json · EventBus · Events · Log · Rng      │
│  net/        BackendClient (HTTP/1.1, Winsock/POSIX)                  │
└───────────────────────────────┬──────────────────────────────────────┘
                                │  REST + JSON
┌───────────────────────────────▼──────────────────────────────────────┐
│                        BACKEND (Java 21)                              │
│  api/     ApiRoutes  (HTTP qatlami)                                   │
│  service/ GameService · ContentService  (biznes mantiq)               │
│  repo/    JsonStore  (atomik fayl ombori — DB bilan almashtiriladi)   │
│  model/   Player · SaveSlot · TelemetryEvent  (record)                │
│  web/     admin panel (statik, kutubxonasiz)                          │
└───────────────────────────────────────────────────────────────────────┘
```

## 1. Actor + Component modeli

GDD Unreal Engine komponentlariga asoslangan, shuning uchun yadro ham xuddi shu shaklda:

```cpp
class Component {                      // UE ActorComponent ekvivalenti
    virtual void onAttach();
    virtual void tick(float dt);
    Actor* owner();
};

class Actor {                          // UE AActor ekvivalenti
    Vec3 position, velocity; float yaw;
    template<typename T> T* addComponent(...);
    template<typename T> T* get() const;   // std::type_index bo'yicha
    std::vector<std::string> tags;         // "player", "enemy", "companion"...
};
```

`World` aktyorlarni egallaydi, `tick` qiladi va so'rovlarni bajaradi
(`allWithTag`, `nearest`, `lineOfSight`, `hidingSpotAt`, `localLight`).

**Nima uchun ECS emas?** GDD tizimlari (masalan `UInjuryStateComponent` boshqa komponentlarning
ruxsatlarini o'zgartiradi) o'zaro ko'p bog'langan. Komponent-obyekt modeli GDD nomlariga
bir-birga mos tushadi va kodni hujjat bilan solishtirish oson bo'ladi.

## 2. Hodisalar (EventBus)

Tizimlar bir-birini bilmaydi — `EventBus` orqali gaplashadi:

```cpp
EventBus::get().subscribe<PlayerSpottedEvent>([](const PlayerSpottedEvent& e) { ... });
EventBus::get().emit(ParrySuccessEvent{defender, attacker});
```

`std::type_index` bo'yicha tiplangan; hodisalar `core/Events.h` da e'lon qilingan
(jang, stealth, hikoya, dunyo, QTE guruhlari).

## 3. Vaqt bo'yicha boshqariladigan jang

Animatsiya hali yo'q, shuning uchun hujum fazalari taymer bilan boshqariladi —
animatsiya kelganda faqat fazalar animatsiya hodisalariga ulanadi:

| Hujum | windup | active | recovery |
|---|---|---|---|
| Yengil | 0.16 s | 0.12 s | 0.24 s |
| Og'ir | 0.42 s | 0.16 s | 0.55 s |
| G'azab | 0.60 s | 0.22 s | 0.70 s |
| Finisher | 0.30 s | 0.35 s | 0.60 s |

`active` fazasida `resolveHit()` oldindagi konusdagi dushmanlarni tekshiradi:
mudofaa (parry/dodge i-frame) → qalqon → sog'lik.

## 4. Stealth formulasi

```
ko'rinuvchanlik = f(yorug'lik) × cho'kkalash × harakat_tezligi × berkinish × olomon
yorug'lik = max(WorldClock.ambientLight(), World.localLight(pozitsiya))
```

Dushman tomonida `DetectionComponent` ko'rish konusi + LOS + eshitish + hid (it) ni
birlashtirib `suspicion` ni to'ldiradi va hushyorlikni o'zgartiradi:
`Tinch → Shubha → Qidiruv → Jang`.

## 5. Kontent — kodda emas, JSON'da

`data/` papkasi **ham o'yin, ham backend** tomonidan o'qiladi:

| Fayl | Kim o'qiydi | Nima |
|---|---|---|
| `data/quests/*.json` | `QuestManager` (C++), `ContentService` (Java) | 17 kvest, maqsadlar, mukofotlar, zanjir |
| `data/enemies/*.json` | `ContentDatabase` → `EnemyCharacter::applyArchetype` | 20 dushman turi + 6 boss (fazalari bilan) |
| `data/dialogue/*.json` | `DialogueSystem` | dialog daraxti, dalil talab qiluvchi variantlar |
| `data/characters/cast.json` | `ContentDatabase` | 44 personaj va ularning gameplay roli |
| `data/episodes/episodes.json` | `ContentDatabase` | 6 epizod, cliffhangerlar, yon kvestlar |
| `localization/ertugrul_loc.csv` | `LocaleManager` | uz / tr / en — 150 kalit |

Shuning uchun kontent o'zgarishi rekompilyatsiya talab qilmaydi va backend uni
validatsiya qila oladi (`/api/content/validate`).

## 6. Renderer abstraksiyasi

```cpp
class Renderer {
    virtual void draw(const World& world, const HudState& hud) = 0;
};
class InputSource {
    virtual void poll(InputState& state) = 0;
};
```

Ikkita PC backendi bor:

| Backend | Fayl | Nima |
|---|---|---|
| **OpenGL 3D** (standart) | `game/src/app/Win32GlRenderer.cpp` | perspektiv 3D, uchinchi shaxs kamerasi, kun/tun nuri, tuman, ko'rish konuslari |
| Oyna va kirish | `game/src/app/Win32Window.cpp` | WGL uchun piksel formati, klaviatura, sichqoncha (kamera uchun ushlanadi) |
| **ANSI konsol** (`--console`) | `game/src/app/ConsoleFrontend.cpp` | terminalda ASCII xarita — SSH yoki tez sinov uchun |

`Application` ikkalasini ham bilmaydi — faqat `HudState` va `World` beradi.
OpenGL/Vulkan backend qo'shilganda `Application` va o'yin mantig'i umuman o'zgarmaydi.

3D renderer to'g'ridan-to'g'ri o'yin holatidan chizadi (alohida sahna grafi yo'q):
`World::blockers` → chodir/bino qutilari, `World::hidingSpots` → butalar,
`World::lightSources` → mash'alalar, aktyorlar → 3D gavdalar, `DetectionComponent` →
yerdagi ko'rish konusi, `WorldClock` → quyosh burchagi va osmon rangi.
Bu GDD IX dagi 3-bosqich ("greybox"): geometriya sodda, lekin fazo, masshtab va
o'qiluvchanlik haqiqiy — modellar keyin shu joyga qo'yiladi.

## 7. Backend qatlamlari

```
HTTP  →  ApiRoutes  →  GameService / ContentService  →  JsonStore  →  fayl tizimi
```

- `JsonStore` atomik yozadi (`.tmp` → `move`), o'qish/yozish qulfi bilan.
- Telemetriya JSONL jurnaliga qo'shiladi (tez yozish, oson tahlil).
- Virtual threadlar (`Executors.newVirtualThreadPerTaskExecutor()`) — Java 21 imkoniyati.
- PostgreSQL'ga o'tish uchun faqat `JsonStore` o'rniga yangi repozitoriya kerak.

## 8. Sinov strategiyasi

| Daraja | Nima tekshiriladi | Qayerda |
|---|---|---|
| Birlik | Json, sog'lik/zirh, chidam, parry, yarador fazalar, sharaf darajalari | `game/tests/test_main.cpp` |
| Integratsiya | kvest zanjiri + mukofot, dialog-duel dalillari, saqlash aylanishi, lokalizatsiya | shu yerda |
| Tizim | to'liq o'yin oqimi (ov → stealth → jang → mix → zikr → saqlash) | `--scene demo` |
| Kontent | kvest zanjiri butunligi | `/api/content/validate` |
