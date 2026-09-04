#include "World/ACRPGAreaTrigger.h"

#include "Core/ACRPG.h"
#include "Core/ACRPGPlayerController.h"
#include "Character/ACRPGPlayerCharacter.h"
#include "Components/ACRPGQuestComponent.h"
#include "UI/ACRPGHUDWidget.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AACRPGAreaTrigger::AACRPGAreaTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(1000.f, 1000.f, 500.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetHiddenInGame(true);
}

void AACRPGAreaTrigger::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AACRPGAreaTrigger::OnAreaBeginOverlap);
}

void AACRPGAreaTrigger::OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Faqat o'yinchi.
	AACRPGPlayerCharacter* Player = Cast<AACRPGPlayerCharacter>(OtherActor);
	if (!Player)
	{
		return;
	}

	// Bir marta yoki cooldown.
	if (bTriggerOnce && bHasTriggered)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (!bTriggerOnce && (Now - LastTriggerTime) < RetriggerCooldown)
	{
		return;
	}

	bHasTriggered = true;
	LastTriggerTime = Now;

	// --- Ep.#76: ekranda xabar ---
	if (AACRPGPlayerController* PC = Cast<AACRPGPlayerController>(Player->GetController()))
	{
		if (UACRPGHUDWidget* HUD = PC->GetHUDWidget())
		{
			HUD->ShowAreaMessage(AreaName, bIsHostileArea);
		}
	}

	if (EnterSound)
	{
		UGameplayStatics::PlaySound2D(this, EnterSound);
	}

	// --- Ep.#37: kvest maqsadi ---
	if (!AreaTag.IsNone())
	{
		if (UACRPGQuestComponent* Quests = Player->GetQuests())
		{
			Quests->NotifyLocationReached(AreaTag);
		}
	}

	UE_LOG(LogACRPG, Log, TEXT("Hududga kirdi: %s"), *AreaName.ToString());
}
