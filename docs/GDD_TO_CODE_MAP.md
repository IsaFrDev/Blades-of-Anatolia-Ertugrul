# GDD → kod moslik jadvali

GDD Unreal Engine nomlarida yozilgan. Loyiha C++/CMake'da qurilgani uchun har bir
UE komponenti/subsystemi qayerga tushgani quyida ko'rsatilgan.

## II QISM 2.2 — Asosiy tizimlar

| GDD nomi | Kod | Fayl |
|---|---|---|
| `UHealthComponent` | `HealthComponent` | `game/include/ertugrul/components/Vitals.h` |
| `UStaminaComponent` | `StaminaComponent` | Vitals.h |
| `UFaithMeterComponent` ("Iymon") | `FaithComponent` | Vitals.h |
| `UBleedingComponent` | `BleedingComponent` | Vitals.h |
| `UInjuryStateComponent` | `InjuryStateComponent` | Vitals.h |
| `UMeleeCombatComponent` | `MeleeCombatComponent` | `components/Combat.h` |
| `UTargetLockComponent` | `TargetLockComponent` | Combat.h |
| `URangedComponent` | `RangedComponent` | Combat.h |
| `UBindingSwordComponent` | `BindingSwordComponent` | Combat.h |
| `UShieldBreakComponent` | `ShieldComponent` | Combat.h |
| `UBossPhaseComponent` | `BossPhaseComponent` | Combat.h |
| `UStealthComponent` | `StealthComponent` | `components/Stealth.h` |
| `UDisguiseComponent` + "shubha metri" | `DisguiseComponent` | Stealth.h |
| `UAssassinationComponent` | `AssassinationComponent` | Stealth.h |
| Detection (ko'rish konusi, ovoz) | `DetectionComponent` | Stealth.h |
| `UClimbingComponent` / `UParkourComponent` | `ParkourComponent` | `components/Movement.h` |
| `UHorseComponent` | `HorseComponent` | Movement.h |
| `AHorseCharacter` | `HorseCharacter` + `HorseBodyComponent` | `characters/Characters.h`, Movement.h |
| `UInvestigationSubsystem` | `InvestigationComponent` | `components/Narrative.h` |
| `UEavesdropComponent` | `EavesdropComponent` | Narrative.h |
| `UTrackingSenseComponent` ("Alp hissi") | `TrackingSenseComponent` | Narrative.h |
| `UCompanionCommandSubsystem` | `CompanionCommandComponent` | Narrative.h |
| `ACompanionAIController` | `CompanionCharacter` xulq metodlari | `src/characters/Characters.cpp` |
| `AEnemyAIController` | `EnemyCharacter::updateAi` | Characters.cpp |
| `ABossAIController` | `BossCharacter` | Characters.cpp |
| `UQuestManagerSubsystem` | `QuestManager` | `subsystems/Subsystems.h` |
| `UDialogueSubsystem` (+ dialog-duel) | `DialogueSystem` | Subsystems.h |
| `UReputationSubsystem` | `ReputationSystem` | Subsystems.h |
| `UObaManagerSubsystem` | `ObaManager` | Subsystems.h |
| `UQTESubsystem` | `QTESystem` | Subsystems.h |
| `UChoiceConsequenceSubsystem` | `ChoiceLedger` | Subsystems.h |
| `UWeatherTimeSubsystem` | `WorldClock` | Subsystems.h |
| `UCutsceneDirector` | `Application::applyNailScene` va sahna metodlari | `app/Application.h` |
| `UAudioLayerSubsystem` (zikr qatlamlari) | `BlacksmithMinigame` audio hodisalari | `minigames/Minigames.h` |
| `UBlacksmithMinigame` | `BlacksmithMinigame` | Minigames.h |
| `UCraftingSubsystem` (Artuq Bey) | `CraftingSystem` | Minigames.h |
| Ov (iz qidirish) | `HuntMinigame` | Minigames.h |
| `UCrowdCombatLOD` / olomon navbati | `CombatDirector` | Subsystems.h |
| Saqlash | `SaveSystem` (+ `BackendClient` bulut) | Subsystems.h, `net/BackendClient.h` |
| Lokalizatsiya (uz/tr/en) | `LocaleManager` | Subsystems.h |

## Hali qurilmagan (interfeys tayyor, implementatsiya keyingi bosqichda)

| GDD nomi | Reja |
|---|---|
| `UWaveSpawnerSubsystem` (1.3 tungi to'lqinlar) | `World::spawn` + taymer; daraja skriptida |
| `ULargeBattleSubsystem` (3.2, 6.2) | `CombatDirector` + zona LOD kengaytmasi |
| `UFireSpreadSubsystem` (6.2) | `World.lightSources` + tarqalish jadvali |
| `USnowTrackSubsystem` (4.3) | `StealthComponent.leavesTracks` bayrog'i bor; iz ombori qo'shiladi |
| `UEscortObjectiveComponent` (1.3) | `Actor` + himoya maqsadi komponenti |
| `USabotageObjectiveComponent` (6.2) | maqsad komponenti |
| `UFastTravelSubsystem` | daraja menejeri bilan birga |
| World Partition / Nanite / Lumen | renderer bosqichida (UE-ga xos; C++ yadroda ekvivalent yo'q) |

## IV–V QISM — Epizodlar va kvestlar

Har bir kvest `data/quests/*.json` da, `gdd_ref` maydoni GDD bo'limiga havola qiladi:

| Kvest fayli | GDD |
|---|---|
| `q1_1_deer_hunt.json` | Epizod 1 / Kvest 1.1 — Kiyik ovi |
| `q1_2_forest_ambush.json` | Epizod 1 / Kvest 1.2 — O'rmondagi pistirma |
| `q1_3_first_blood.json` | Epizod 1 / Kvest 1.3 — Birinchi qon va qochish |
| `q2_1_journey_to_aleppo.json` | Epizod 2 / Kvest 2.1 — Halabga sayohat |
| `q2_2_caravanserai_ambush.json` | Epizod 2 / Kvest 2.2 — Karvonsaroy tungi pistirmasi |
| `q2_3_expose_traitor.json` | Epizod 2 / Kvest 2.3 — Xoinni fosh qilish |
| `q3_1_infiltrate_amanos.json` | Epizod 3 / Kvest 3.1 — Amanos qal'asiga infiltratsiya |
| `q3_2_castle_courtyard.json` | Epizod 3 / Kvest 3.2 — Qal'a hovlisi jangi |
| `q3_3_titus_duel.json` | Epizod 3 / Kvest 3.3 — Final boss: Titus |
| `q4_1_border_ambush.json` | Epizod 4 / Kvest 4.1 — Chegaradagi pistirma |
| `q4_2_nail_scene.json` | Epizod 4 / Kvest 4.2 — Qiynoq va mix sahnasi |
| `q4_3_one_handed_escape.json` | Epizod 4 / Kvest 4.3 — Bir qo'lli qochish |
| `q5_1_return_to_forge.json` | Epizod 5 / Kvest 5.1 — Temirchilikka qaytish |
| `q5_2_test_of_faith.json` | Epizod 5 / Kvest 5.2 — Iymon sinovi |
| `q6_1_unite_the_beyliks.json` | Epizod 6 / Kvest 6.1 — Beyliklarni birlashtirish |
| `q6_2_night_raid.json` | Epizod 6 / Kvest 6.2 — Mo'g'ul lageriga tungi reyd |
| `q6_3_noyan_duel.json` | Epizod 6 / Kvest 6.3 — Final boss: Bayju Noyan |

## VII QISM — Bestiary

`data/enemies/units.json` (14 tur) va `data/enemies/bosses.json` (6 boss).
Har biri `gdd_ref` bilan belgilangan; bosslarda `phases` massivi GDD dagi faza
tavsiflarini (qalqon → uzun qilich → yaralangan g'azab kabi) aynan takrorlaydi.

## VIII QISM — UI/UX va audio

| GDD | Kod |
|---|---|
| HUD: sog'lik / chidam / Iymon / ko'rinuvchanlik | `HudState` + `Win32Renderer` (oyna) / `ConsoleRenderer` |
| Til: uz / tr / en subtitr | `LocaleManager`, `localization/ertugrul_loc.csv` |
| Zikr qatlamlari (a cappella jo'r) | `BlacksmithMinigame` combo → `ZikrBeatEvent` |
| Kodeks | `CodexUnlockedEvent` (hodisa bor, UI keyingi bosqichda) |
