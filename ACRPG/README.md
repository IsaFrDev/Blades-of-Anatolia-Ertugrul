# ACRPG — Gorka Games UE5 RPG seriyasining C++ porti

Gorka Games'ning **"Unreal Engine 5 RPG Tutorial Series"** (84 epizod, Blueprint) o'yin
tizimlarining C++ dagi tashkil etilgan varianti. Assassin's Creed uslubidagi ochiq
dunyo RPG uchun asos.

**Maqsad:** Blueprint'dagi mantiqni C++ ga ko'chirish — bir xil natija, lekin
tezroq, o'qish oson va git'da diff qilinadigan kod.

---

## Talablar

| Narsa | Versiya |
|---|---|
| Unreal Engine | 5.3 / 5.4 / 5.5 |
| IDE | Visual Studio 2022 (Windows) yoki Rider |
| Plaginlar | Motion Warping, Enhanced Input, Niagara, Control Rig, AI Module |

---

## O'rnatish

### 1. Modulni loyihaga qo'shish

`Source/ACRPG/` papkasini o'z loyihangizning `Source/` papkasiga ko'chiring,
so'ng `<LoyihaNomi>.uproject` faylida modulni ro'yxatga oling:

```json
{
  "Modules": [
    {
      "Name": "ACRPG",
      "Type": "Runtime",
      "LoadingPhase": "Default"
    }
  ],
  "Plugins": [
    { "Name": "MotionWarping", "Enabled": true },
    { "Name": "EnhancedInput", "Enabled": true },
    { "Name": "Niagara",       "Enabled": true },
    { "Name": "ControlRig",    "Enabled": true }
  ]
}
```

> **Eslatma:** `IMPLEMENT_PRIMARY_GAME_MODULE` loyihada faqat BITTA marta bo'lishi
> kerak. Agar sizda allaqachon asosiy modul bo'lsa, `Private/Core/ACRPG.cpp` dagi
> o'sha qatorni `IMPLEMENT_MODULE(FACRPGModule, ACRPG)` ga o'zgartiring.

### 2. Qayta generatsiya va build

```bash
# .uproject ga o'ng tugma -> "Generate Visual Studio project files"
# keyin Visual Studio'da Build
```

### 3. Blueprint'larni yaratish

C++ klasslari **asos**, Blueprint'lar esa **sozlama**. Har biri uchun BP yarating:

| C++ klass | Blueprint | Nima tayinlanadi |
|---|---|---|
| `AACRPGPlayerCharacter` | `BP_Player` | Mesh, AnimBP, barcha `IA_*` Input Action'lar, montajlar |
| `AACRPGEnemyCharacter` | `BP_Enemy` | Mesh, kombo montajlari, `XPReward`, `LootTable` |
| `AACRPGQuestGiverNPC` | `BP_QuestGiver` | `QuestToGive`, dialog widget |
| `UACRPGAnimInstance` | `ABP_Player` | State Machine, Blendspace, Foot IK tugunlari |
| `AACRPGGameMode` | `BP_GameMode` | `DefaultPawnClass = BP_Player` |
| `UACRPGGameInstance` | `BP_GameInstance` | `ItemDataTable`, `QuestDataTable` |
| `AACRPGEnemyController` | `BP_EnemyAI` | `BehaviorTreeAsset = BT_Enemy` |
| `UACRPGHUDWidget` | `WBP_HUD` | HealthBar, StaminaBar, XPBar (nomlar aynan) |

### 4. Data Table'lar

Ikkita Data Table yarating:

- **DT_Items** — Row Structure: `ACRPGItemData`
- **DT_Quests** — Row Structure: `ACRPGQuestData`

va ularni `BP_GameInstance` ga tayinlang.

### 5. Kerakli socketlar

Skeletda quyidagi socketlar bo'lishi shart:

```
hand_r    -> Sword_Drawn      (qilich qo'lda)
spine_03  -> Sword_Sheathed   (qilich belda)
hand_l    -> Shield_Drawn
spine_01  -> Bow_Sheathed
hand_l    -> Bow_Drawn
hand_r    -> ArrowSocket      (o'q uchadigan joy)
foot_l / foot_r               (Foot IK va qadam tovushi)
```

Qurol mesh'ida:
```
TraceStart  -> dasta
TraceEnd    -> uchi
```

### 6. Anim Notify'larni qo'yish

Bu qadam **majburiy** — usiz jang ishlamaydi:

| Montaj | Notify | Qayerga |
|---|---|---|
| Hujum montajlari | `ACRPG Qilich trace` (State) | pichoq harakat qilayotgan oraliq |
| Hujum montajlari | `ACRPG Kombo oynasi` (State) | zarbaning oxirgi 30% |
| Yurish/yugurish | `ACRPG Qadam tovushi` | oyoq yerga tekkan kadr |
| Dodge montajlari | `ACRPG Zarbaga chidamlilik` (State) | dumalashning o'rtasi |

### 7. Physical Surface'lar (Ep.#55)

`Project Settings > Physics > Physical Surfaces`:

```
SurfaceType1 = Sand
SurfaceType2 = Stone
SurfaceType3 = Water
SurfaceType4 = Wood
```

---

## Papka tuzilishi

```
Source/ACRPG/
├── ACRPG.Build.cs
├── Public/
│   ├── Core/          Modul, GameMode, GameInstance, PlayerController, Save
│   ├── Character/     Personajlar, AnimInstance, AnimNotify'lar
│   ├── Components/    Barcha o'yin tizimlari (stats, jang, kvest...)
│   ├── AI/            AI Controller'lar va Behavior Tree tugunlari
│   ├── Items/         Qurollar, o'qlar, otiladigan buyumlar
│   ├── Quests/        Interfeys va kvest beruvchi NPC
│   ├── UI/            HUD va menyular
│   └── World/         Kun/tun sikli, hudud triggerlari
└── Private/           (Public bilan bir xil tuzilish)
```

---

## Arxitektura qoidalari

1. **Personaj — dirijyor, komponent — ijrochi.**
   `AACRPGPlayerCharacter` deyarli mantiqsiz: u kirishni komponentlarga uzatadi.

2. **Komponentlar UI ni bilmaydi.**
   Ular faqat delegate chiqaradi. HUD o'zi obuna bo'ladi.

3. **Ma'lumot Data Table'da, holat komponentda.**
   `FACRPGItemData` o'zgarmaydi; `FACRPGInventoryEntry` o'zgaradi va saqlanadi.

4. **Vaqt animatsiyada, mantiq kodda.**
   "Qachon urish o'tadi" — Anim Notify hal qiladi, C++ emas.

5. **AI: Controller = miya, Character = tana.**
   Personaj o'lganda miya to'xtaydi, tana qoladi.

---

## Debug

Loyihada `LogACRPG` kategoriyasi bor:

```
// Konsolda
Log LogACRPG Verbose

// Trace'larni ko'rish uchun BP defaults'da:
bShowDebugTraces = true    (VaultingComponent)
bShowTraceDebug  = true    (CombatComponent)
bShowDebug       = true    (Climbing / Assassination)
```

---

## Epizodlar bilan bog'liqlik

Har bir fayl boshida qaysi epizodni qamrab olishi yozilgan. To'liq jadval
uchun HTML qo'llanmaga qarang.
