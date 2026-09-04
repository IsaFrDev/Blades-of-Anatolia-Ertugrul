#include "Character/ACRPGEnemyCharacter.h"

#include "Core/ACRPG.h"
#include "Core/ACRPGGameInstance.h"
#include "Components/ACRPGStatsComponent.h"
#include "Components/ACRPGQuestComponent.h"
#include "Items/ACRPGItemBase.h"

#include "Components/WidgetComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Engine/World.h"

AACRPGEnemyCharacter::AACRPGEnemyCharacter()
{
	// --- Ep.#21: AI bu personajni ko'ra olsin ---
	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	StimuliSource->bAutoRegister = true;
	StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
	StimuliSource->RegisterForSense(UAISense_Hearing::StaticClass());

	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HealthBarWidget->SetupAttachment(RootComponent);
	HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidget->SetDrawSize(FVector2D(120.f, 12.f));
	HealthBarWidget->SetRelativeLocation(FVector(0.f, 0.f, 110.f));

	CharacterTag = TEXT("Enemy");
}

void AACRPGEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (bHideHealthBarUntilDamaged && HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(false);

		if (StatsComponent)
		{
			StatsComponent->OnHealthChanged.AddDynamic(
				this, &AACRPGEnemyCharacter::OnHealthChanged_ShowBar);
		}
	}
}

void AACRPGEnemyCharacter::OnHealthChanged_ShowBar(float NewHealth, float MaxHealth)
{
	if (HealthBarWidget && NewHealth < MaxHealth)
	{
		HealthBarWidget->SetVisibility(true);
	}
}

void AACRPGEnemyCharacter::OnCombatStateChanged(ECombatState OldState, ECombatState NewState)
{
	Super::OnCombatStateChanged(OldState, NewState);

	if (NewState == ECombatState::Dead && HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(false);
	}
}

void AACRPGEnemyCharacter::HandleDeath(AActor* Killer)
{
	// Ikki marta o'lmasin.
	if (GetCombatState() == ECombatState::Dead)
	{
		return;
	}

	// --- Ep.#65: o'ldirgan o'yinchiga XP ---
	if (AACRPGCharacterBase* KillerChar = Cast<AACRPGCharacterBase>(Killer))
	{
		if (UACRPGStatsComponent* KillerStats = KillerChar->GetStats())
		{
			KillerStats->AddXP(XPReward);
		}

		// --- Ep.#41: kvest hisobiga qo'shamiz ---
		// Kvest komponenti "Bandit" tegli 3 ta dushmanni sanaydi.
		if (UACRPGQuestComponent* Quests = KillerChar->FindComponentByClass<UACRPGQuestComponent>())
		{
			Quests->NotifyEnemyKilled(CharacterTag);
		}
	}

	DropLoot();

	Super::HandleDeath(Killer);
}

void AACRPGEnemyCharacter::DropLoot()
{
	UWorld* World = GetWorld();
	const UACRPGGameInstance* GI = UACRPGGameInstance::Get(this);
	if (!World || !GI || LootTable.Num() == 0)
	{
		return;
	}

	int32 DropIndex = 0;

	for (const auto& Pair : LootTable)
	{
		// Ehtimollik tekshiruvi.
		if (FMath::FRand() > Pair.Value)
		{
			continue;
		}

		const FACRPGItemData* Data = GI->FindItem(Pair.Key);
		if (!Data || Data->ItemActorClass.IsNull())
		{
			continue;
		}

		UClass* ItemClass = Data->ItemActorClass.LoadSynchronous();
		if (!ItemClass)
		{
			continue;
		}

		// Bir nechta buyum bir joyga tushmasin — atrofga sochamiz.
		const float Angle = FMath::DegreesToRadians(DropIndex * 60.f);
		const FVector Offset(FMath::Cos(Angle) * 60.f, FMath::Sin(Angle) * 60.f, 20.f);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		World->SpawnActor<AACRPGItemBase>(
			ItemClass, GetActorLocation() + Offset, FRotator::ZeroRotator, Params);

		++DropIndex;
	}
}
