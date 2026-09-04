#include "Components/ACRPGQuestComponent.h"

#include "Core/ACRPG.h"
#include "Core/ACRPGGameInstance.h"
#include "Character/ACRPGCharacterBase.h"
#include "Components/ACRPGStatsComponent.h"
#include "Components/ACRPGInventoryComponent.h"
#include "Items/ACRPGItemBase.h"
#include "Quests/ACRPGInteractableInterface.h"

#include "Engine/OverlapResult.h"
#include "Engine/World.h"

UACRPGQuestComponent::UACRPGQuestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UACRPGQuestComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<AACRPGCharacterBase>(GetOwner());

	if (!StartingQuestID.IsNone())
	{
		StartQuest(StartingQuestID);
	}
}

// ---------------------------------------------------------------------------
// KVESTNI BOSHLASH (Ep.#34)
// ---------------------------------------------------------------------------

bool UACRPGQuestComponent::StartQuest(FName QuestID)
{
	if (QuestID.IsNone())
	{
		return false;
	}

	// Allaqachon jurnalda bormi?
	if (const FACRPGQuestProgress* Existing = FindProgress(QuestID))
	{
		if (Existing->State == EQuestState::Active || Existing->State == EQuestState::Completed)
		{
			return false;	// Ikki marta boshlab bo'lmaydi.
		}
	}

	const UACRPGGameInstance* GI = UACRPGGameInstance::Get(this);
	const FACRPGQuestData* Data = GI ? GI->FindQuest(QuestID) : nullptr;
	if (!Data)
	{
		UE_LOG(LogACRPG, Warning, TEXT("StartQuest: '%s' Data Table'da yo'q."), *QuestID.ToString());
		return false;
	}

	FACRPGQuestProgress NewProgress;
	NewProgress.QuestID = QuestID;
	NewProgress.State = EQuestState::Active;

	// Maqsadlarni ta'rifdan nusxalab olamiz va hisoblagichlarni nolga qo'yamiz.
	NewProgress.Objectives = Data->Objectives;
	for (FACRPGQuestObjective& Obj : NewProgress.Objectives)
	{
		Obj.CurrentCount = 0;
	}

	QuestLog.Add(NewProgress);

	// Ep.#36 — faol kvest bo'lmasa, shuni faol qilamiz.
	if (ActiveQuestID.IsNone())
	{
		SetActiveQuest(QuestID);
	}

	OnQuestStarted.Broadcast(QuestID);
	UE_LOG(LogACRPG, Log, TEXT("Kvest boshlandi: %s"), *QuestID.ToString());
	return true;
}

// ---------------------------------------------------------------------------
// MAQSADLARNI OSHIRISH (Ep.#39, #41, #42)
// ---------------------------------------------------------------------------

void UACRPGQuestComponent::AdvanceObjectives(EQuestObjectiveType Type, FName TargetTag, int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	// Faol kvestlarning HAMMASINI ko'rib chiqamiz — bitta dushman
	// bir vaqtda ikki kvestga hisoblanishi mumkin.
	for (FACRPGQuestProgress& Progress : QuestLog)
	{
		if (Progress.State != EQuestState::Active)
		{
			continue;
		}

		bool bChanged = false;

		for (int32 i = 0; i < Progress.Objectives.Num(); ++i)
		{
			FACRPGQuestObjective& Obj = Progress.Objectives[i];

			if (Obj.Type != Type || Obj.TargetTag != TargetTag || Obj.IsComplete())
			{
				continue;
			}

			// Chegaradan oshib ketmasin (3/3 bo'lsin, 5/3 emas).
			Obj.CurrentCount = FMath::Min(Obj.CurrentCount + Amount, Obj.RequiredCount);
			bChanged = true;

			OnObjectiveUpdated.Broadcast(Progress.QuestID, i, Obj.CurrentCount);

			UE_LOG(LogACRPG, Log, TEXT("Maqsad: %s [%d/%d]"),
				*Progress.QuestID.ToString(), Obj.CurrentCount, Obj.RequiredCount);
		}

		if (bChanged)
		{
			CheckQuestCompletion(Progress);
		}
	}
}

void UACRPGQuestComponent::NotifyEnemyKilled(FName EnemyTag)
{
	AdvanceObjectives(EQuestObjectiveType::Kill, EnemyTag, 1);
}

void UACRPGQuestComponent::NotifyItemCollected(FName ItemID, int32 Quantity)
{
	AdvanceObjectives(EQuestObjectiveType::Collect, ItemID, Quantity);
}

void UACRPGQuestComponent::NotifyLocationReached(FName LocationTag)
{
	AdvanceObjectives(EQuestObjectiveType::Reach, LocationTag, 1);
}

void UACRPGQuestComponent::NotifyTalkedTo(FName NPCTag)
{
	AdvanceObjectives(EQuestObjectiveType::TalkTo, NPCTag, 1);
}

// ---------------------------------------------------------------------------
// YAKUNLASH (Ep.#43)
// ---------------------------------------------------------------------------

void UACRPGQuestComponent::CheckQuestCompletion(FACRPGQuestProgress& Progress)
{
	for (const FACRPGQuestObjective& Obj : Progress.Objectives)
	{
		if (!Obj.IsComplete())
		{
			return;	// Hali tugamagan.
		}
	}

	CompleteQuest(Progress.QuestID);
}

bool UACRPGQuestComponent::CompleteQuest(FName QuestID)
{
	FACRPGQuestProgress* Progress = FindProgress(QuestID);
	if (!Progress || Progress->State != EQuestState::Active)
	{
		return false;
	}

	Progress->State = EQuestState::Completed;

	const UACRPGGameInstance* GI = UACRPGGameInstance::Get(this);
	const FACRPGQuestData* Data = GI ? GI->FindQuest(QuestID) : nullptr;

	if (Data)
	{
		GrantRewards(*Data);

		// Zanjir kvest — keyingisini ochamiz.
		if (!Data->NextQuestID.IsNone())
		{
			StartQuest(Data->NextQuestID);
		}
	}

	// Faol kvest tugadi — keyingi faolini topamiz.
	if (ActiveQuestID == QuestID)
	{
		FName NextActive = NAME_None;
		for (const FACRPGQuestProgress& P : QuestLog)
		{
			if (P.State == EQuestState::Active)
			{
				NextActive = P.QuestID;
				break;
			}
		}
		SetActiveQuest(NextActive);
	}

	OnQuestCompleted.Broadcast(QuestID);
	UE_LOG(LogACRPG, Log, TEXT("Kvest bajarildi: %s"), *QuestID.ToString());
	return true;
}

void UACRPGQuestComponent::GrantRewards(const FACRPGQuestData& QuestData)
{
	if (!OwnerCharacter)
	{
		return;
	}

	// Ep.#65 — tajriba.
	if (QuestData.XPReward > 0)
	{
		if (UACRPGStatsComponent* Stats = OwnerCharacter->GetStats())
		{
			Stats->AddXP(QuestData.XPReward);
		}
	}

	// Buyumlar.
	if (UACRPGInventoryComponent* Inv = OwnerCharacter->FindComponentByClass<UACRPGInventoryComponent>())
	{
		for (const FACRPGInventoryEntry& Reward : QuestData.ItemRewards)
		{
			Inv->AddItem(Reward.ItemID, Reward.Quantity);
		}
	}
}

// ---------------------------------------------------------------------------
// FAOL KVEST (Ep.#36-37)
// ---------------------------------------------------------------------------

void UACRPGQuestComponent::SetActiveQuest(FName QuestID)
{
	if (ActiveQuestID == QuestID)
	{
		return;
	}

	// Faqat faol kvestni tanlash mumkin (yoki bo'shatish).
	if (!QuestID.IsNone())
	{
		const FACRPGQuestProgress* Progress = FindProgress(QuestID);
		if (!Progress || Progress->State != EQuestState::Active)
		{
			return;
		}
	}

	ActiveQuestID = QuestID;
	OnActiveQuestChanged.Broadcast(ActiveQuestID);
}

bool UACRPGQuestComponent::GetCurrentObjective(FACRPGQuestObjective& OutObjective) const
{
	const FACRPGQuestProgress* Progress = FindProgress(ActiveQuestID);
	if (!Progress)
	{
		return false;
	}

	for (const FACRPGQuestObjective& Obj : Progress->Objectives)
	{
		if (!Obj.IsComplete())
		{
			OutObjective = Obj;
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// O'ZARO TA'SIR (Ep.#38, #42)
// ---------------------------------------------------------------------------

bool UACRPGQuestComponent::TryInteract()
{
	if (!OwnerCharacter)
	{
		return false;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Origin = OwnerCharacter->GetActorLocation();
	const FVector Forward = OwnerCharacter->GetActorForwardVector();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Interact), false, OwnerCharacter);

	// WorldDynamic + Pawn: sandiqlar ham, NPC lar ham topilsin.
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);

	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjParams,
		FCollisionShape::MakeSphere(InteractionRange), Params);

	AActor* Best = nullptr;
	float BestScore = -1.f;

	for (const FOverlapResult& R : Overlaps)
	{
		AActor* Candidate = R.GetActor();
		if (!Candidate || Candidate == OwnerCharacter)
		{
			continue;
		}

		// Interfeys yoki buyum bo'lishi kerak.
		const bool bInteractable =
			Candidate->Implements<UACRPGInteractableInterface>() ||
			Candidate->IsA<AACRPGItemBase>();

		if (!bInteractable)
		{
			continue;
		}

		// Oldimizda turganini tanlaymiz (orqadagini emas).
		const FVector ToCandidate = (Candidate->GetActorLocation() - Origin).GetSafeNormal2D();
		const float Dot = FVector::DotProduct(Forward, ToCandidate);

		if (Dot < 0.3f)	// ~72 gradusli konus
		{
			continue;
		}

		if (Dot > BestScore)
		{
			BestScore = Dot;
			Best = Candidate;
		}
	}

	if (!Best)
	{
		return false;
	}

	// Interfeysli bo'lsa — uni chaqiramiz.
	if (Best->Implements<UACRPGInteractableInterface>())
	{
		IACRPGInteractableInterface::Execute_OnInteract(Best, OwnerCharacter);
		return true;
	}

	// Oddiy buyum bo'lsa — olamiz.
	if (AACRPGItemBase* Item = Cast<AACRPGItemBase>(Best))
	{
		const FName PickedID = Item->ItemID;
		Item->Interact(OwnerCharacter);

		// Ep.#42 — kvest hisobiga qo'shamiz.
		NotifyItemCollected(PickedID, 1);
		return true;
	}

	return false;
}

// ---------------------------------------------------------------------------
// YORDAMCHILAR
// ---------------------------------------------------------------------------

FACRPGQuestProgress* UACRPGQuestComponent::FindProgress(FName QuestID)
{
	return QuestLog.FindByPredicate(
		[QuestID](const FACRPGQuestProgress& P) { return P.QuestID == QuestID; });
}

const FACRPGQuestProgress* UACRPGQuestComponent::FindProgress(FName QuestID) const
{
	return QuestLog.FindByPredicate(
		[QuestID](const FACRPGQuestProgress& P) { return P.QuestID == QuestID; });
}

EQuestState UACRPGQuestComponent::GetQuestState(FName QuestID) const
{
	const FACRPGQuestProgress* Progress = FindProgress(QuestID);
	return Progress ? Progress->State : EQuestState::Unavailable;
}

bool UACRPGQuestComponent::GetQuestProgress(FName QuestID, FACRPGQuestProgress& OutProgress) const
{
	if (const FACRPGQuestProgress* Found = FindProgress(QuestID))
	{
		OutProgress = *Found;
		return true;
	}
	return false;
}

void UACRPGQuestComponent::LoadFromSave(const TArray<FACRPGQuestProgress>& SavedLog, FName SavedActiveQuest)
{
	QuestLog = SavedLog;
	ActiveQuestID = SavedActiveQuest;

	OnActiveQuestChanged.Broadcast(ActiveQuestID);
	UE_LOG(LogACRPG, Log, TEXT("Kvest jurnali yuklandi: %d ta kvest."), QuestLog.Num());
}
