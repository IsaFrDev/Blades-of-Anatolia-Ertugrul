#include "Quests/ACRPGQuestGiverNPC.h"

#include "Core/ACRPG.h"
#include "Components/ACRPGQuestComponent.h"

#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

AACRPGQuestGiverNPC::AACRPGQuestGiverNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	// NPC joyida turadi.
	GetCharacterMovement()->MaxWalkSpeed = 150.f;

	QuestMarkerWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("QuestMarker"));
	QuestMarkerWidget->SetupAttachment(RootComponent);
	QuestMarkerWidget->SetWidgetSpace(EWidgetSpace::Screen);
	QuestMarkerWidget->SetDrawSize(FVector2D(32.f, 32.f));
	QuestMarkerWidget->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

	CharacterTag = TEXT("NPC");
}

void AACRPGQuestGiverNPC::BeginPlay()
{
	Super::BeginPlay();
	UpdateQuestMarker();
}

void AACRPGQuestGiverNPC::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateHeadLookAt(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// Ep.#40 — BOSHNI O'YINCHIGA BURISH
// ---------------------------------------------------------------------------

void AACRPGQuestGiverNPC::UpdateHeadLookAt(float DeltaSeconds)
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player || !IsAlive())
	{
		HeadLookAlpha = FMath::FInterpTo(HeadLookAlpha, 0.f, DeltaSeconds, HeadLookInterpSpeed);
		return;
	}

	const float Distance = FVector::Dist(Player->GetActorLocation(), GetActorLocation());

	// Uzoq bo'lsa — qaramaydi.
	if (Distance > HeadLookRange)
	{
		HeadLookAlpha = FMath::FInterpTo(HeadLookAlpha, 0.f, DeltaSeconds, HeadLookInterpSpeed);
		HeadLookRotation = FMath::RInterpTo(HeadLookRotation, FRotator::ZeroRotator,
			DeltaSeconds, HeadLookInterpSpeed);
		return;
	}

	// O'yinchiga qarash uchun kerakli burchak.
	const FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(
		GetActorLocation() + FVector(0.f, 0.f, 150.f),	// taxminan bosh balandligi
		Player->GetActorLocation() + FVector(0.f, 0.f, 90.f));

	// NPC tanasiga NISBATAN burchak (Control Rig shuni kutadi).
	FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(LookAt, GetActorRotation());

	// Orqada bo'lsa qaramaydi — bo'yin aylanib ketmasin.
	if (FMath::Abs(Delta.Yaw) > MaxHeadYaw)
	{
		HeadLookAlpha = FMath::FInterpTo(HeadLookAlpha, 0.f, DeltaSeconds, HeadLookInterpSpeed);
		return;
	}

	Delta.Pitch = FMath::Clamp(Delta.Pitch, -30.f, 30.f);
	Delta.Roll = 0.f;

	HeadLookRotation = FMath::RInterpTo(HeadLookRotation, Delta, DeltaSeconds, HeadLookInterpSpeed);
	HeadLookAlpha = FMath::FInterpTo(HeadLookAlpha, 1.f, DeltaSeconds, HeadLookInterpSpeed);
}

// ---------------------------------------------------------------------------
// Ep.#38 — DIALOG VA KVEST BERISH
// ---------------------------------------------------------------------------

void AACRPGQuestGiverNPC::OnInteract_Implementation(AACRPGCharacterBase* Interactor)
{
	if (!Interactor || QuestToGive.IsNone())
	{
		return;
	}

	UACRPGQuestComponent* Quests = Interactor->FindComponentByClass<UACRPGQuestComponent>();
	if (!Quests)
	{
		return;
	}

	const EQuestState State = Quests->GetQuestState(QuestToGive);

	switch (State)
	{
	case EQuestState::Unavailable:
	case EQuestState::Available:
		// Kvestni beramiz.
		Quests->StartQuest(QuestToGive);
		break;

	case EQuestState::Active:
		// Ep.#38 — "TalkTo" turidagi maqsad bo'lsa, hisobga olamiz.
		Quests->NotifyTalkedTo(CharacterTag);
		break;

	case EQuestState::Completed:
	default:
		break;
	}

	// Dialog oynasi.
	if (DialogueWidgetClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(Interactor->GetController()))
		{
			if (UUserWidget* Dialogue = CreateWidget<UUserWidget>(PC, DialogueWidgetClass))
			{
				Dialogue->AddToViewport();
			}
		}
	}

	UpdateQuestMarker();
}

FText AACRPGQuestGiverNPC::GetInteractionPrompt_Implementation() const
{
	return NSLOCTEXT("ACRPG", "TalkPrompt", "E — Gaplashish");
}

void AACRPGQuestGiverNPC::UpdateQuestMarker()
{
	if (!QuestMarkerWidget)
	{
		return;
	}

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const AACRPGCharacterBase* PlayerChar = Cast<AACRPGCharacterBase>(Player);

	if (!PlayerChar || QuestToGive.IsNone())
	{
		QuestMarkerWidget->SetVisibility(false);
		return;
	}

	const UACRPGQuestComponent* Quests = PlayerChar->FindComponentByClass<UACRPGQuestComponent>();
	if (!Quests)
	{
		QuestMarkerWidget->SetVisibility(false);
		return;
	}

	// Bajarilgan kvest uchun belgi ko'rsatilmaydi.
	const EQuestState State = Quests->GetQuestState(QuestToGive);
	QuestMarkerWidget->SetVisibility(State != EQuestState::Completed);
}
