# Unity'ga ko'chirish — holat va yo'l xaritasi

Unity loyihasi: `D:\My project` (Unity 6000.3, URP, Input System, Cinemachine).
C++ dvijok o'z holicha qoladi va **kontent manbai** bo'lib xizmat qiladi: darajalar,
cutscene'lar, lokalizatsiya, ovozlar shu repodan Unity'ga oqadi.

## Hozir tayyor (2026-09-03)

| Nima | Qayerda (Unity) | Izoh |
|---|---|---|
| 182 ta model (OBJ + MTL + basecolor) | `Assets/Ertugrul/Models/` | Unity OBJ ni o'zi import qiladi, materiallar URP Lit |
| 942 ta ovoz (uz/tr/en) | `Assets/Ertugrul/Audio/vo/<til>/` | WAV, 24 kHz mono |
| Cutscene, epizod, daraja JSON'lari | `Assets/Ertugrul/Data/` | C++ format — C# o'quvchi hali yozilmagan |
| Lokalizatsiya CSV (937 kalit, 3 til) | `Assets/Ertugrul/Localization/` | |
| 5 daraja — Unity uchun eksport | `Assets/Ertugrul/Levels/*.level.json` | relyef 257², rekvizitlar (scatter joylashtirilgan), spawn, osmon |
| Importer | `Assets/Ertugrul/Editor/ErtugrulLevelImporter.cs` | menyu **Ertugrul → Import ALL levels → scenes** |

Eksport buyrug'i (C++ tomonda, darajani o'zgartirganda qayta chiqarish):

```bash
build/ertugrul.exe --level oba_valley --export-unity "D:/My project/Assets/Ertugrul/Levels"
```

## Import bajarildi (2026-09-03)

5 sahna `Assets/Ertugrul/Scenes/` da: aleppo_road, forest_pass, oba_camp, oba_valley,
sogut_village — barcha rekvizitlar topildi (0 yo'qolgan model), relyefga protsedural o't
qatlami (`Assets/Ertugrul/Textures/grass.terrainlayer`), quyosh/tuman JSON dan, spawn
markerlari (`Spawn_player` va h.k., yo'nalishi tekshirildi — oba oldinda ko'rinadi).
Qayta import: `Assets/Ertugrul/Levels/AUTOIMPORT` bo'sh faylini yarating va Unity
oynasiga bosing (yoki menyu **Ertugrul → Import ALL levels → scenes**).
Unity CLI (`unity command eval/screenshot`) `com.unity.pipeline` paketi orqali ulanadi.

## Birinchi qadam (siz)

1. Unity Hub'dan `D:\My project` ni oching — modellar importi ~5–10 daqiqa (195 MB, 4096² teksturalar).
2. Menyu: **Ertugrul → Import ALL levels → scenes**. `Assets/Ertugrul/Scenes/` da 5 sahna paydo bo'ladi.
3. `oba_valley.unity` ni oching. Agar sahna ko'zgu bo'lib ko'rinsa (yurt eshiklari teskari),
   `ErtugrulLevelImporter.cs` dagi `FlipZ` ni `false` qiling va qayta import qiling.

## Keyingi bosqichlar (tartib bilan)

1. **Personaj** — OBJ modellarda skelet yo'q (C++ da protsedural rig edi). Unity'da:
   Mixamo'ga `ottoman.obj` → FBX qilib yuklab avtorig, yoki Unity'ning o'zida
   Animation Rigging. Locomotion: Character Controller + Cinemachine FreeLook.
2. **Lokalizatsiya** — CSV ni Unity Localization String Table'ga import qilish
   (`unity:localization` skill yordam beradi). Kalitlar bir xil qoladi.
3. **Cutscene** — `Data/cutscenes/*.json` ni o'qib Timeline yaratuvchi editor skript:
   aktyor pozitsiyalari, kamera kesmalari, replikalar (VO WAV + subtitr).
4. **Jang / kamon / iymon** — C++ mantiq (`Character.cpp`, `Combat.cpp`, `Encounter.cpp`)
   C# ga qayta yoziladi. Bu eng katta qism (~8 ming qator).
5. **Menyu / HUD** — UI Toolkit (`unity:ui-uitk`).

Halol baho: 1–3 bir hafta, 4–5 yana 2–3 hafta. C++ versiya shu paytgacha demo sifatida ishlayveradi.
