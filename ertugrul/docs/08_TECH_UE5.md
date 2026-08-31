# 08 — TEXNIK ARXITEKTURA: UNREAL ENGINE 5 (C++)

> **Maqsad:** 48 epizod, 4 mavsum, ~60 soat kontent, PC + PS5 + XSX, 60 FPS.
> **Falsafa:** dizayner **kod yozmasdan** epizod qura olsin; kod **ma'lumot bilan boshqarilsin** (data-driven).

---

# 1. MODUL TUZILISHI

```
ErtugrulGame/                          ← .uproject
│
├── Source/
│   ├── ErtugrulCore/          [Runtime]  Poydevor: tag'lar, interfeyslar, save
│   ├── ErtugrulGameplay/      [Runtime]  GAS, jang, harakat, jarohat
│   ├── ErtugrulNarrative/     [Runtime]  ⭐ Epizod, quest, dialog, intro director
│   ├── ErtugrulHistory/       [Runtime]  ⭐ Kodeks, BilgeGöz, Safar Daftari
│   ├── ErtugrulWorld/         [Runtime]  Level streaming, ob-havo, olomon, oba
│   ├── ErtugrulAI/            [Runtime]  Dushman AI, nerge, jamoa buyruqlari
│   ├── ErtugrulUI/            [Runtime]  CommonUI, HUD, menyu
│   ├── ErtugrulOnline/        [Runtime]  ⭐ Java backend bilan aloqa
│   ├── ErtugrulEditor/        [Editor]   ⭐ Epizod muharriri, validator
│   └── ErtugrulTests/         [Editor]   Avtomatik testlar
│
├── Content/
│   ├── Data/Episodes/         48 × UErtEpisodeDataAsset
│   ├── Data/Codex/            ~180 × UErtCodexEntry
│   ├── Data/Quests/           ~200 × UErtQuestGraph
│   ├── Data/Characters/       ~60  × UErtCharacterDef
│   └── Data/Items/            14 qurol + zirh + resurs
│
└── Config/
    ├── DefaultGameplayTags.ini
    └── DefaultErtugrulSettings.ini
```

## 1.1 Modul bog'liqliklari (bir tomonlama — aylanma bog'liqlik yo'q)

```
                    ┌─────────────────┐
                    │  ErtugrulCore   │  ← hech kimga bog'liq emas
                    └────────┬────────┘
          ┌──────────────────┼──────────────────┐
          ▼                  ▼                  ▼
   ┌─────────────┐   ┌──────────────┐   ┌─────────────┐
   │  Gameplay   │   │   History    │   │   Online    │
   └──────┬──────┘   └──────┬───────┘   └──────┬──────┘
          │                 │                  │
          └────────┬────────┴──────────────────┘
                   ▼
            ┌──────────────┐        ┌──────────┐      ┌──────┐
            │  Narrative   │───────▶│  World   │─────▶│  AI  │
            └──────┬───────┘        └──────────┘      └──────┘
                   ▼
            ┌──────────────┐
            │      UI      │  ← hamma narsani "o'qiydi", hech narsani o'zgartirmaydi
            └──────────────┘
```

```csharp
// ErtugrulNarrative.Build.cs
public class ErtugrulNarrative : ModuleRules
{
    public ErtugrulNarrative(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine",
            "GameplayTags", "GameplayAbilities", "GameplayTasks",
            "ErtugrulCore", "ErtugrulGameplay", "ErtugrulHistory"
        });
        PrivateDependencyModuleNames.AddRange(new[] {
            "LevelSequence", "MovieScene", "EnhancedInput",
            "AIModule", "UMG", "ErtugrulWorld"
        });
    }
}
```

---

# 2. ⭐ EPIZOD MA'LUMOT MODELI (sizning JSON'ingizning to'g'ri versiyasi)

## 2.1 Asosiy Data Asset

```cpp
// ErtugrulNarrative/Public/Episode/ErtEpisodeDataAsset.h
#pragma once
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ErtEpisodeDataAsset.generated.h"

UENUM(BlueprintType)
enum class EErtEpisodeArchetype : uint8
{
    Infiltration, Siege, Survival, Investigation,
    Court, Escort, Defense, Chase, Ritual
};

UENUM(BlueprintType)
enum class EErtDifficultyTier : uint8 { T1, T2, T3, T4, T5 };

/** ⭐ Tarixiy ishonchlilik — o'yinning asosiy differensiatori */
UENUM(BlueprintType)
enum class EErtConfidence : uint8
{
    Documented  UMETA(DisplayName="HUJJATLI"),
    Disputed    UMETA(DisplayName="BAHSLI"),
    Legend      UMETA(DisplayName="RIVOYAT")
};

USTRUCT(BlueprintType)
struct FErtHistoricalAnchor
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FString      HijriDate;       // "641 Muharram"
    UPROPERTY(EditAnywhere) FString      GregorianDate;   // "26 Iyun 1243"
    UPROPERTY(EditAnywhere) int32        YearGregorian = 0; // sortlash uchun
    UPROPERTY(EditAnywhere) EErtConfidence Confidence = EErtConfidence::Documented;
    UPROPERTY(EditAnywhere, meta=(MultiLine)) FText ScholarNote;
    UPROPERTY(EditAnywhere) TArray<FString> SourceUrls;
};

USTRUCT(BlueprintType)
struct FErtObjective
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FGameplayTag ObjectiveTag;
    UPROPERTY(EditAnywhere) FText        LocDescription;   // ⚠️ FText — loc-key bilan
    UPROPERTY(EditAnywhere) bool         bOptional = false;
    /** ⭐ "Boshqariladigan mag'lubiyat" — EP10 Turgut, EP35 Köse Dağ */
    UPROPERTY(EditAnywhere) bool         bScriptedFailure = false;
    UPROPERTY(EditAnywhere) FGameplayTagContainer CompletionTags;
};

USTRUCT(BlueprintType)
struct FErtEnemyComposition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) TMap<FGameplayTag,int32> ArchetypeCounts; // Enemy.Templar.Sergeant → 4
    UPROPERTY(EditAnywhere) int32 MaxSimultaneous = 6;   // ⚠️ balans: 5+ = o'lim
    UPROPERTY(EditAnywhere) int32 BackgroundAgents = 0;  // Mass Framework (Köse Dağ: 3000)
};

USTRUCT(BlueprintType)
struct FErtBranchChoice
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName        ChoiceId;         // "SS_1", "EP12_TITUS"
    UPROPERTY(EditAnywhere) FText        Prompt;
    UPROPERTY(EditAnywhere) TArray<FText> Options;
    /** ⭐ Shubha sahnasi — tarixiy noaniqlik tanlovi */
    UPROPERTY(EditAnywhere) bool         bIsUncertaintyScene = false;
    /** Har variant uchun kodeks — ikkalasi ham ochiladi */
    UPROPERTY(EditAnywhere) TArray<FName> CodexPerOption;
    /** world_state ga yoziladigan flag'lar */
    UPROPERTY(EditAnywhere) TArray<FGameplayTag> ResultFlags;
};

// ═══════════════════════════════════════════════════════════════
UCLASS(BlueprintType)
class ERTUGRULNARRATIVE_API UErtEpisodeDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    // ── Identifikatsiya ──────────────────────────────────────────
    UPROPERTY(EditAnywhere, Category="ID") int32 SchemaVersion = 2;
    UPROPERTY(EditAnywhere, Category="ID") FName EpisodeId;        // "EP024"
    UPROPERTY(EditAnywhere, Category="ID") FName SeasonId;         // "S2"
    UPROPERTY(EditAnywhere, Category="ID") int32 GlobalIndex = 0;  // 0..47
    UPROPERTY(EditAnywhere, Category="ID") int32 SeasonIndex = 0;  // 0..11

    // ── Matn (⚠️ FText — hardcode string EMAS) ───────────────────
    UPROPERTY(EditAnywhere, Category="Text") FText Title;
    UPROPERTY(EditAnywhere, Category="Text", meta=(MultiLine)) FText Synopsis;
    UPROPERTY(EditAnywhere, Category="Text", meta=(MultiLine)) FText CliffhangerText;

    // ── Tarix ⭐ ─────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, Category="History") FErtHistoricalAnchor Anchor;
    UPROPERTY(EditAnywhere, Category="History") TArray<FName> CodexUnlocks;

    // ── O'ynaladigan intro ⭐ (mp4 EMAS) ─────────────────────────
    UPROPERTY(EditAnywhere, Category="Intro")
    TSoftObjectPtr<class UErtIntroSequenceAsset> IntroSequence;

    // ── Dizayn ───────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, Category="Design") EErtEpisodeArchetype Archetype;
    UPROPERTY(EditAnywhere, Category="Design") EErtDifficultyTier   Tier;
    UPROPERTY(EditAnywhere, Category="Design") int32 EstimatedMinutes = 55;
    UPROPERTY(EditAnywhere, Category="Design") TArray<FErtObjective> Objectives;
    UPROPERTY(EditAnywhere, Category="Design") FErtEnemyComposition  Enemies;
    UPROPERTY(EditAnywhere, Category="Design") TArray<FGameplayTag>  MechanicsIntroduced;
    UPROPERTY(EditAnywhere, Category="Design") TArray<FGameplayTag>  TraversalTypes;

    // ── Mix tizimi ⭐ ────────────────────────────────────────────
    UPROPERTY(EditAnywhere, Category="Mih") FGameplayTag MihBeatTag;   // har epizodda ≥1
    UPROPERTY(EditAnywhere, Category="Mih") float ExpectedHandIntegrityAtStart = 100.f;
    UPROPERTY(EditAnywhere, Category="Mih") float WoundDrainMultiplier = 1.f;

    // ── Oqim ─────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, Category="Flow") TArray<FName> Prerequisites;
    UPROPERTY(EditAnywhere, Category="Flow") TArray<FName> Unlocks;
    UPROPERTY(EditAnywhere, Category="Flow") TArray<FErtBranchChoice> Choices;
    UPROPERTY(EditAnywhere, Category="Flow") TArray<FGameplayTag> WorldStateDeltas;
    UPROPERTY(EditAnywhere, Category="Flow") TArray<FGameplayTag> Checkpoints;
    UPROPERTY(EditAnywhere, Category="Flow") TArray<FGameplayTag> FailConditions;

    // ── Kontent ──────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, Category="Content") TArray<FGameplayTag> ContentWarnings;
    UPROPERTY(EditAnywhere, Category="Content") bool bHasTortureScene = false;

    // ── Muhit ────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, Category="World") FGameplayTag Season;      // Kuz/Qish/Bahor/Yoz
    UPROPERTY(EditAnywhere, Category="World") FGameplayTag TimeOfDay;
    UPROPERTY(EditAnywhere, Category="World") FGameplayTag Weather;
    UPROPERTY(EditAnywhere, Category="World") TArray<TSoftObjectPtr<UWorld>> StreamingLevels;

    // ── Audio ────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> MusicCue;
    UPROPERTY(EditAnywhere, Category="Audio") FGameplayTag AmbienceTag;

    // ── Telemetriya ──────────────────────────────────────────────
    UPROPERTY(EditAnywhere, Category="Telemetry") FName FunnelId;

    // ── Editor validatsiyasi ⭐ ──────────────────────────────────
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Ctx) const override;
#endif
};
```

## 2.2 ⭐ Editor validatori — dizayner xato qila olmaydi

Bu — sizning JSON'ingizdagi `"season": 0` kabi buglarni **build vaqtida** ushlaydi:

```cpp
#if WITH_EDITOR
EDataValidationResult UErtEpisodeDataAsset::IsDataValid(FDataValidationContext& Ctx) const
{
    EDataValidationResult R = Super::IsDataValid(Ctx);
    auto Fail = [&](const FString& Msg){ Ctx.AddError(FText::FromString(Msg));
                                        R = EDataValidationResult::Invalid; };

    // 1) ID formati
    if (!EpisodeId.ToString().MatchesWildcard(TEXT("EP0??")))
        Fail(TEXT("EpisodeId 'EP001'..'EP048' formatida bo'lishi kerak"));

    // 2) Indeks izchilligi (sizning JSON'ingizdagi off-by-one bug)
    if (GlobalIndex < 0 || GlobalIndex > 47) Fail(TEXT("GlobalIndex 0..47"));
    const int32 ExpectedSeason = GlobalIndex / 12;
    if (SeasonId != *FString::Printf(TEXT("S%d"), ExpectedSeason + 1))
        Fail(TEXT("SeasonId GlobalIndex ga mos emas"));

    // 3) ⭐ Mix beat — HAR epizodda majburiy (sizning talabingiz)
    if (!MihBeatTag.IsValid())
        Fail(TEXT("MihBeatTag majburiy — har epizodda mix ko'rinishi kerak"));

    // 4) ⭐ Tarixiy langar
    if (Anchor.YearGregorian < 1227 || Anchor.YearGregorian > 1261)
        Fail(TEXT("Yil 1227-1261 oralig'ida bo'lishi kerak"));
    if (Anchor.Confidence != EErtConfidence::Documented && Anchor.ScholarNote.IsEmpty())
        Fail(TEXT("BAHSLI/RIVOYAT uchun ScholarNote majburiy"));

    // 5) ⭐ O'ynaladigan intro (mp4 emas)
    if (IntroSequence.IsNull())
        Fail(TEXT("IntroSequence majburiy — o'ynaladigan intro"));

    // 6) Dangling reference'lar (sizning JSON'ingizdagi asosiy muammo)
    for (const FName& C : CodexUnlocks)
        if (!UErtCodexRegistry::Get().Contains(C))
            Fail(FString::Printf(TEXT("Kodeks topilmadi: %s"), *C.ToString()));
    for (const FName& P : Prerequisites)
        if (!UErtEpisodeRegistry::Get().Contains(P))
            Fail(FString::Printf(TEXT("Prerequisite topilmadi: %s"), *P.ToString()));

    // 7) Balans
    if (Enemies.MaxSimultaneous > 8)
        Ctx.AddWarning(FText::FromString(TEXT("8+ bir vaqtda dushman — o'lim tuzog'i")));
    if (EstimatedMinutes < 30 || EstimatedMinutes > 90)
        Ctx.AddWarning(FText::FromString(TEXT("Epizod 45-70 daq bo'lishi kerak")));

    // 8) ⭐ Arxetip takrorlanishi (global tekshiruv)
    if (UErtEpisodeRegistry::Get().WouldRepeatArchetypeThrice(EpisodeId, Archetype))
        Ctx.AddWarning(FText::FromString(TEXT("Arxetip 3 marta ketma-ket — xilma-xillik buziladi")));

    // 9) Kontent ogohlantirishi
    if (bHasTortureScene && ContentWarnings.Num() == 0)
        Fail(TEXT("Qiynoq sahnasi bor, lekin ContentWarnings bo'sh — sertifikatsiya rad etadi"));

    return R;
}
#endif
```

> ⚠️ Bu validator **CI/CD da ham ishlaydi** (`UnrealEditor-Cmd -run=DataValidation`). Xato bo'lsa — **build to'xtaydi**. Bu — 48 epizodli loyihada omon qolishning yagona yo'li.

---

# 3. QUEST TIZIMI — GRAF, RO'YXAT EMAS

Sizning JSON'ingizda `"quests": ["q1_1_deer_hunt", ...]` — **chiziqli ro'yxat**. Real o'yinda quest — **yo'naltirilgan graf** (DAG).

```cpp
// ErtugrulNarrative/Public/Quest/ErtQuestGraph.h
USTRUCT(BlueprintType)
struct FErtQuestNode
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName        NodeId;
    UPROPERTY(EditAnywhere) FGameplayTag CompletionTag;
    UPROPERTY(EditAnywhere) TArray<FName> NextNodes;        // bir nechta yo'l
    /** Shart: bu tugun faqat shu tag'lar bo'lsa ochiladi */
    UPROPERTY(EditAnywhere) FGameplayTagQuery Requirement;
    UPROPERTY(EditAnywhere) bool bOptional = false;
    UPROPERTY(EditAnywhere) bool bTerminal = false;
};

UCLASS(BlueprintType)
class UErtQuestGraph : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere) FName QuestId;
    UPROPERTY(EditAnywhere) FName EntryNode;
    UPROPERTY(EditAnywhere) TArray<FErtQuestNode> Nodes;
    UPROPERTY(EditAnywhere) TMap<FName, FErtQuestReward> RewardsByTerminal;
};
```

```cpp
// Runtime — quest holati
void UErtQuestSubsystem::OnGameplayTagAdded(const FGameplayTag& Tag)
{
    for (FErtActiveQuest& Q : ActiveQuests)
    {
        const FErtQuestNode& Cur = Q.Graph->FindNode(Q.CurrentNode);
        if (Cur.CompletionTag != Tag) continue;

        // Keyingi ochiladigan tugunlarni topamiz (shartlarga qarab)
        TArray<FName> Available;
        for (const FName& Next : Cur.NextNodes)
        {
            const FErtQuestNode& N = Q.Graph->FindNode(Next);
            if (N.Requirement.Matches(WorldState->GetTags()))
                Available.Add(Next);
        }

        if (Available.Num() == 0 || Cur.bTerminal)
        {
            CompleteQuest(Q, Cur.NodeId);
            // ⭐ Telemetriya — qaysi yo'l bilan tugatildi
            Online->Telemetry()->Push(FErtTelemetryEvent::QuestComplete(Q.Graph->QuestId, Cur.NodeId));
        }
        else Q.CurrentNode = Available[0];  // yoki o'yinchi tanlaydi
    }
}
```

---

# 4. ⭐ WORLD STATE — o'yinchining tarixi

```cpp
// ErtugrulCore/Public/State/ErtWorldStateSubsystem.h
UCLASS()
class ERTUGRULCORE_API UErtWorldStateSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    // ── Flag'lar (bool holatlar) ─────────────────────────────────
    UFUNCTION(BlueprintCallable) void SetFlag(FGameplayTag Tag, bool bValue);
    UFUNCTION(BlueprintPure)     bool HasFlag(FGameplayTag Tag) const;

    // ── Sonli holatlar ───────────────────────────────────────────
    UFUNCTION(BlueprintCallable) void SetScalar(FName Key, float Value);
    UFUNCTION(BlueprintPure)     float GetScalar(FName Key, float Default = 0.f) const;

    // ── Fraksiya obro'si (6 fraksiya, -100..100) ────────────────
    UFUNCTION(BlueprintCallable) void AddReputation(EErtFaction F, float Delta);

    // ── O'lgan hamrohlar (permadeath) ────────────────────────────
    UPROPERTY(SaveGame) TSet<FName> FallenCompanions;

    // ── Shubha sahnasi tanlovlari ⭐ ─────────────────────────────
    UPROPERTY(SaveGame) TMap<FName, int32> UncertaintyChoices;  // "SS_1" → 1

    FErtWorldSnapshot Snapshot() const;   // rekap va backend uchun

private:
    UPROPERTY(SaveGame) FGameplayTagCountContainer Flags;
    UPROPERTY(SaveGame) TMap<FName, float> Scalars;
    UPROPERTY(SaveGame) TMap<EErtFaction, float> Reputation;
};
```

```cpp
// ⭐ Fraksiya obro'si — muvozanat matritsasi bilan
void UErtWorldStateSubsystem::AddReputation(EErtFaction F, float Delta)
{
    Reputation.FindOrAdd(F) = FMath::Clamp(Reputation.FindOrAdd(F) + Delta, -100.f, 100.f);

    // ⚠️ Neytral qola olmaysiz — dushman fraksiyalarda teskari ta'sir
    for (int32 i = 0; i < 6; ++i)
    {
        const EErtFaction Other = (EErtFaction)i;
        if (Other == F) continue;
        const float Enmity = FactionEnmity[(int32)F][i];   // -1..+1
        if (!FMath::IsNearlyZero(Enmity))
            Reputation.FindOrAdd(Other) =
                FMath::Clamp(Reputation.FindOrAdd(Other) + Delta * Enmity * 0.5f, -100.f, 100.f);
    }
    OnReputationChanged.Broadcast(F, Reputation[F]);
}
```

---

# 5. GAS (Gameplay Ability System) sxemasi

## 5.1 Atributlar

```cpp
// ErtugrulGameplay/Public/Abilities/ErtAttributeSet.h
UCLASS()
class UErtAttributeSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    ATTRIBUTE_ACCESSORS(UErtAttributeSet, Health)
    ATTRIBUTE_ACCESSORS(UErtAttributeSet, MaxHealth)
    ATTRIBUTE_ACCESSORS(UErtAttributeSet, Stamina)       // Nafas
    ATTRIBUTE_ACCESSORS(UErtAttributeSet, Posture)       // Poza
    ATTRIBUTE_ACCESSORS(UErtAttributeSet, MaxPosture)
    ATTRIBUTE_ACCESSORS(UErtAttributeSet, HandIntegrity) // ⭐ Mix
    ATTRIBUTE_ACCESSORS(UErtAttributeSet, MaxHandIntegrity)
    ATTRIBUTE_ACCESSORS(UErtAttributeSet, Sabr)          // ⭐ psixologik chidam
    ATTRIBUTE_ACCESSORS(UErtAttributeSet, Iman)          // ⭐ iymon
    ATTRIBUTE_ACCESSORS(UErtAttributeSet, BodyHeat)      // harorat (qish arki)

    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& D) override;
};

void UErtAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& D)
{
    // ⭐ HandIntegrity hech qachon MaxHandIntegrity dan oshmaydi.
    //    Bu — o'yinning markaziy qoidasi: qo'l hech qachon to'liq tuzalmaydi.
    if (D.EvaluatedData.Attribute == GetHandIntegrityAttribute())
        SetHandIntegrity(FMath::Clamp(GetHandIntegrity(), 0.f, GetMaxHandIntegrity()));

    if (D.EvaluatedData.Attribute == GetPostureAttribute() && GetPosture() >= GetMaxPosture())
        GetOwningAbilitySystemComponent()->AddLooseGameplayTag(TAG_State_PostureBroken);
}
```

## 5.2 Gameplay Tag ierarxiyasi

```ini
; Config/DefaultGameplayTags.ini (qisqartirilgan)
Ability.Combat.LightAttack
Ability.Combat.Parry
Ability.Combat.Dodge
Ability.Combat.Kick              ; ⭐ EP24 dan keyin ochiladi
Ability.Traversal.Climb
Ability.Traversal.BiteGrip       ; ⭐ arqonni tishlab ushlash (EP41)

State.Wound.Intact
State.Wound.Fresh
State.Wound.Chronic
State.Wound.Adapted
State.Wound.BoundBlade           ; qilich bilakka bog'langan
State.Hand.Hidden                ; ijtimoiy stealth uchun

Wound.Source.Cold
Wound.Source.Parry
Wound.Source.BowDraw
Wound.Source.Climb
Wound.Source.NoyanStrike

Mih.Beat.Object                  ; ⭐ har epizodda majburiy — 48 xil
Mih.Beat.OnOther
Mih.Beat.Corpse
Mih.Beat.Mechanical
Mih.Beat.Dialogue
Mih.Beat.Advantage

Codex.Observe.KayiBanner
Codex.Use.BowInRain
Codex.Dialogue.CaravanTrade

Episode.EP024.NailDriven         ; qaytarib bo'lmaydigan nuqta
Choice.SS1.SuleymanShah
Choice.SS1.GunduzAlp
Choice.EP012.TitusSpared
```

---

# 6. LEVEL STREAMING VA XOTIRA

## 6.1 World Partition + Data Layers

```
   Mintaqa (World Partition)
   ├── Data Layer: "Base"        — doim yuklangan (terrain, ob-havo)
   ├── Data Layer: "Season_S1"   — 1-mavsum obyektlari (kuz, Templar)
   ├── Data Layer: "Season_S3"   — 3-mavsum (qor, mo'g'ul, vayronalar)
   ├── Data Layer: "Oba_L1/L2/L3"— ⭐ oba qurilish darajasi
   └── Data Layer: "Post_EP024"  — ⭐ mix dan keyingi dunyo o'zgarishlari
```

```cpp
// Epizod boshlanganda kerakli qatlamlarni yoqamiz
void UErtEpisodeSubsystem::PrepareWorldFor(const UErtEpisodeDataAsset* Ep)
{
    auto* DLM = UDataLayerManager::GetDataLayerManager(GetWorld());

    DLM->SetDataLayerRuntimeState(FindLayer(Ep->SeasonId), EDataLayerRuntimeState::Activated);

    // ⭐ Mix dan keyingi dunyo — EP024 dan keyin doim yoqiq
    if (WorldState->HasFlag(TAG_Episode_EP024_NailDriven))
        DLM->SetDataLayerRuntimeState(FindLayer("Post_EP024"), EDataLayerRuntimeState::Activated);

    // Oba darajasi
    const int32 ObaLevel = FMath::RoundToInt(WorldState->GetScalar("oba.level", 1.f));
    DLM->SetDataLayerRuntimeState(FindLayer(FName(*FString::Printf(TEXT("Oba_L%d"), ObaLevel))),
                                  EDataLayerRuntimeState::Activated);
}
```

## 6.2 Xotira byudjeti (PS5 / XSX: 12.5 GB o'yin uchun)

| Kategoriya | Byudjet | Izoh |
|---|---|---|
| Nanite geometriya | 3.2 GB | ⚠️ Köse Dağ da eng yuqori |
| Virtual Texture pool | 2.4 GB | |
| Animatsiya (2 ta to'liq animset!) | 1.1 GB | ⭐ o'ng + chap qo'l alohida |
| Audio (VO 8 til strim) | 1.4 GB | |
| Mass Framework (3000 agent) | 0.9 GB | Faqat jang epizodlarida |
| Olomon (shahar) | 0.7 GB | Halab, Konya |
| Gameplay + AI | 1.2 GB | |
| UI + Kodeks | 0.4 GB | |
| Rezerv | 1.2 GB | |

⚠️ **Eng katta risk: ikkita to'liq jang animseti.** O'ng qo'l (EP01-23) va chap qo'l (EP24-48) bir vaqtda yuklanmasligi kerak — `Post_EP024` data layer bilan almashtiriladi.

---

# 7. ⭐ MASS FRAMEWORK — Köse Dağ (3000 agent)

```cpp
// ErtugrulAI/Public/Mass/ErtMassBattleProcessor.h
UCLASS()
class UErtMassBattleProcessor : public UMassProcessor
{
    GENERATED_BODY()
    UErtMassBattleProcessor();
protected:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager& EM, FMassExecutionContext& Ctx) override;
private:
    FMassEntityQuery SoldierQuery;
};

void UErtMassBattleProcessor::Execute(FMassEntityManager& EM, FMassExecutionContext& Ctx)
{
    SoldierQuery.ForEachEntityChunk(EM, Ctx, [](FMassExecutionContext& C)
    {
        const auto Transforms = C.GetMutableFragmentView<FTransformFragment>();
        const auto Morale     = C.GetMutableFragmentView<FErtMoraleFragment>();
        const auto Formation  = C.GetFragmentView<FErtFormationFragment>();

        for (int32 i = 0; i < C.GetNumEntities(); ++i)
        {
            // ⭐ Soxta chekinish: mo'g'ullar chekinsa saljuqiy ruhiyati oshadi
            //    va ular safni buzib quvadi — Köse Dağ'ning aynan sababi
            if (Formation[i].bEnemyFeigningRetreat)
            {
                Morale[i].Value += 0.35f * C.GetDeltaTimeSeconds();
                if (Morale[i].Value > 0.8f)
                    Formation[i].bBreakingRanks = true;   // → o'rab olinadi
            }
            IntegrateMovement(Transforms[i], Formation[i], C.GetDeltaTimeSeconds());
        }
    });
}
```

**LOD strategiyasi:**
| Masofa | Ko'rinish | Xarajat |
|---|---|---|
| 0–25 m | To'liq Skeletal Mesh + GAS | ~40 ta |
| 25–80 m | Vertex Animation Texture (VAT) | ~300 ta |
| 80–400 m | Instanced Static Mesh + imposters | ~2700 ta |

---

# 8. SAVE TIZIMI

```cpp
// ErtugrulCore/Public/Save/ErtSaveGame.h
UCLASS()
class UErtSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 SchemaVersion = 2;           // ⭐ migratsiya uchun
    UPROPERTY() FDateTime SavedAt;
    UPROPERTY() FName CurrentEpisodeId;
    UPROPERTY() int32 PlaytimeSeconds = 0;

    // ⭐ Mix — o'yinning eng muhim saqlanadigan holati
    UPROPERTY() float HandIntegrity = 100.f;
    UPROPERTY() float MaxIntegrity  = 100.f;
    UPROPERTY() EErtHandPhase HandPhase = EErtHandPhase::Intact;
    UPROPERTY() float Sabr = 50.f;
    UPROPERTY() float Iman = 50.f;
    UPROPERTY() int32 OpiumUses = 0;

    UPROPERTY() FErtWorldSnapshot WorldState;
    UPROPERTY() TSet<FName> UnlockedCodex;          // ⭐ backend bilan sinxron
    UPROPERTY() TArray<FErtJourneyEntry> JourneyLog; // ⭐ Safar Daftari
    UPROPERTY() TMap<FName,int32> UncertaintyChoices;
    UPROPERTY() TSet<FName> FallenCompanions;
    UPROPERTY() FErtObaState Oba;

    /** ⭐ HMAC imzo — anti-cheat (backend tekshiradi) */
    UPROPERTY() FString Signature;
};
```

```cpp
// Migratsiya — eski save'larni buzmaslik
bool UErtSaveSubsystem::MigrateIfNeeded(UErtSaveGame* S)
{
    if (S->SchemaVersion == CURRENT_SCHEMA) return true;

    if (S->SchemaVersion == 1)
    {
        // v1 → v2: MaxIntegrity maydoni yo'q edi
        S->MaxIntegrity = (S->HandPhase == EErtHandPhase::Intact) ? 100.f : 55.f;
        S->SchemaVersion = 2;
        UE_LOG(LogErtSave, Warning, TEXT("Save v1 → v2 migratsiya qilindi"));
    }
    return S->SchemaVersion == CURRENT_SCHEMA;
}
```

---

# 9. ONLINE MODULI (Java backend bilan aloqa)

```cpp
// ErtugrulOnline/Public/ErtOnlineSubsystem.h
UCLASS()
class UErtOnlineSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    /** ⚠️ Hamma narsa OFFLINE ishlashi kerak. Backend — bonus, shart emas. */
    UFUNCTION(BlueprintCallable) void SyncCodexProgress();
    UFUNCTION(BlueprintCallable) void UploadSave(int32 Slot);
    UFUNCTION(BlueprintCallable) void FetchRemoteConfig();   // live-ops balans
    UFUNCTION(BlueprintCallable) void PushJourneyEntry(const FErtJourneyEntry& E);
    UFUNCTION(BlueprintCallable) void FetchChoiceStats(FName ChoiceId);  // "62% Titusni qutqardi"

    class UErtTelemetryQueue* Telemetry() const { return TelemetryQueue; }

private:
    UPROPERTY() UErtTelemetryQueue* TelemetryQueue;  // offline navbat, keyin yuboriladi
    FString BaseUrl;        // https://api.ertugrul.game/v1
    FString AccessToken;
};
```

```cpp
// ⭐ Live-ops: balansni patchsiz o'zgartirish
void UErtOnlineSubsystem::ApplyRemoteConfig(const FErtRemoteConfig& Cfg)
{
    for (const FErtEpisodeBalanceOverride& O : Cfg.EpisodeOverrides)
    {
        auto* Ep = UErtEpisodeRegistry::Get().Find(O.EpisodeId);
        if (!Ep) continue;
        // Telemetriya EP029 da o'yinchilar juda ko'p o'layotganini ko'rsatdi →
        // parry oynasini kengaytiramiz, patch chiqarmasdan
        RuntimeBalance.Add(O.EpisodeId, O);
    }
}
```

---

# 10. TESTLAR VA CI/CD

## 10.1 Avtomatik testlar

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FErtAllEpisodesValidTest,
    "Ertugrul.Data.AllEpisodesValid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FErtAllEpisodesValidTest::RunTest(const FString&)
{
    TArray<UErtEpisodeDataAsset*> All = UErtEpisodeRegistry::Get().GetAll();
    TestEqual(TEXT("48 epizod bo'lishi kerak"), All.Num(), 48);

    // ⭐ Har epizodda mix beat bor
    for (auto* Ep : All)
        TestTrue(FString::Printf(TEXT("%s da MihBeatTag yo'q"), *Ep->EpisodeId.ToString()),
                 Ep->MihBeatTag.IsValid());

    // ⭐ Sanalar o'sib borishi kerak
    for (int32 i = 1; i < All.Num(); ++i)
        TestTrue(TEXT("Sanalar xronologik"),
                 All[i]->Anchor.YearGregorian >= All[i-1]->Anchor.YearGregorian);

    // ⭐ Arxetip 2 martadan ko'p ketma-ket takrorlanmasin
    for (int32 i = 2; i < All.Num(); ++i)
        TestFalse(TEXT("Arxetip 3 marta ketma-ket"),
            All[i]->Archetype == All[i-1]->Archetype && All[i]->Archetype == All[i-2]->Archetype);

    // ⭐ Ot 48 epizoddan 12 tasidan ko'pida asosiy bo'lmasin
    int32 HorseCount = 0;
    for (auto* Ep : All)
        if (Ep->TraversalTypes.Num() && Ep->TraversalTypes[0] == TAG_Traversal_Horse) ++HorseCount;
    TestTrue(TEXT("Ot monopoliyasi"), HorseCount <= 12);

    return true;
}
```

## 10.2 CI/CD pipeline

```yaml
# .github/workflows/build.yml
name: Ertugrul Build
on: [push, pull_request]
jobs:
  validate-data:
    runs-on: [self-hosted, windows, ue5]
    steps:
      - uses: actions/checkout@v4
        with: { lfs: true }
      - name: ⭐ Data validation (epizodlar, kodeks, quest)
        run: >
          UnrealEditor-Cmd.exe ErtugrulGame.uproject
          -run=DataValidation -unattended -nopause
          -abslog=%GITHUB_WORKSPACE%\Logs\validation.log
      - name: Automation tests
        run: >
          UnrealEditor-Cmd.exe ErtugrulGame.uproject
          -ExecCmds="Automation RunTests Ertugrul; Quit" -unattended
      - name: Localization completeness  # ⚠️ hardcoded string qidiradi
        run: python Tools/check_loc_keys.py --fail-on-hardcoded
  build:
    needs: validate-data
    strategy: { matrix: { platform: [Win64, PS5, XSX] } }
    steps:
      - name: BuildCookRun
        run: RunUAT.bat BuildCookRun -project=... -platform=${{ matrix.platform }} -cook -stage -pak
```

---

# 11. PRODUCTION XARITASI (kim nima yozadi)

| Modul | Kim | Odam-oy |
|---|---|---|
| `ErtugrulCore` + save | Senior gameplay eng. | 4 |
| `ErtugrulGameplay` (GAS, jang, ⭐mix) | 2× Senior | 14 |
| `ErtugrulNarrative` (⭐epizod, quest, intro) | Senior + Mid | 10 |
| `ErtugrulHistory` (⭐kodeks, BilgeGöz) | Mid | 5 |
| `ErtugrulWorld` (streaming, ob-havo, oba) | Senior tech artist + eng. | 8 |
| `ErtugrulAI` (⭐nerge, Mass, boss) | Senior AI eng. | 9 |
| `ErtugrulUI` | Mid + UX | 6 |
| `ErtugrulOnline` | Mid | 3 |
| `ErtugrulEditor` (⭐validator, tooling) | Tools eng. | 4 |
| **Jami muhandislik** | | **~63 odam-oy** |

⚠️ **Eng katta texnik risk:** ikkita to'liq jang animseti (o'ng/chap qo'l). Bu **6–8 kunlik alohida mocap** + ~1.1 GB xotira + ikki barobar animatsiya QA. Buni **prototip bosqichida** hal qilish kerak — keyinroq juda qimmat bo'ladi.
