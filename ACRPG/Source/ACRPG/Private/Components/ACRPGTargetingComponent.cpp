#include "Components/ACRPGTargetingComponent.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGCharacterBase.h"
#include "Components/ACRPGStatsComponent.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

UACRPGTargetingComponent::UACRPGTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;	// Lock tekshiruvi har kadr kerak emas.
}

void UACRPGTargetingComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<AACRPGCharacterBase>(GetOwner());
	SetComponentTickEnabled(false);	// Faqat lock bo'lganda kerak.
}

void UACRPGTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateLockValidity();
}

// ---------------------------------------------------------------------------
// NISHON TANLASH
// ---------------------------------------------------------------------------

AActor* UACRPGTargetingComponent::FindBestTarget() const
{
	if (!OwnerCharacter)
	{
		return nullptr;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Kamera qayerga qarayotgani — nishon tanlashda asosiy mezon.
	const FVector ViewLocation = OwnerCharacter->GetActorLocation();
	const FVector ViewDirection = OwnerCharacter->GetBaseAimRotation().Vector();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOn), false, OwnerCharacter);

	World->OverlapMultiByObjectType(
		Overlaps, ViewLocation, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(MaxLockDistance), Params);

	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();

	const float MaxAngleCos = FMath::Cos(FMath::DegreesToRadians(MaxLockAngle));

	for (const FOverlapResult& R : Overlaps)
	{
		AACRPGCharacterBase* Candidate = Cast<AACRPGCharacterBase>(R.GetActor());
		if (!Candidate || Candidate == OwnerCharacter || !Candidate->IsAlive())
		{
			continue;
		}

		const FVector ToTarget = Candidate->GetActorLocation() - ViewLocation;
		const float Distance = ToTarget.Size();
		if (Distance > MaxLockDistance || Distance < KINDA_SMALL_NUMBER)
		{
			continue;
		}

		// Burchak tekshiruvi — orqadagi dushmanlarni tanlamaymiz.
		const float Cos = FVector::DotProduct(ViewDirection, ToTarget / Distance);
		if (Cos < MaxAngleCos)
		{
			continue;
		}

		// Ko'rinadimi? Devor orqasidagi dushmanni lock qilib bo'lmaydi.
		FHitResult Blocking;
		FCollisionQueryParams LOSParams(SCENE_QUERY_STAT(LockLOS), false, OwnerCharacter);
		LOSParams.AddIgnoredActor(Candidate);

		if (World->LineTraceSingleByChannel(Blocking, ViewLocation,
			Candidate->GetActorLocation(), ECC_Visibility, LOSParams))
		{
			continue;
		}

		// --- Ball: masofa + burchak jarimasi ---
		// AngleDeg = 0 (to'g'ri qarshimizda) -> jarima 0
		// AngleDeg = 60 -> jarima 60 * AngleWeight
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Cos, -1.f, 1.f)));
		const float Score = Distance + AngleDeg * AngleWeight;

		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Candidate;
		}
	}

	return Best;
}

void UACRPGTargetingComponent::ToggleLockOn()
{
	if (CurrentTarget)
	{
		ClearTarget();
		return;
	}

	if (AActor* Found = FindBestTarget())
	{
		SetTarget(Found);
	}
}

void UACRPGTargetingComponent::SetTarget(AActor* NewTarget)
{
	if (CurrentTarget == NewTarget)
	{
		return;
	}

	CurrentTarget = NewTarget;

	// Nishon ustidagi belgi.
	if (MarkerWidget)
	{
		MarkerWidget->DestroyComponent();
		MarkerWidget = nullptr;
	}

	if (CurrentTarget && LockOnMarkerClass)
	{
		MarkerWidget = NewObject<UWidgetComponent>(CurrentTarget);
		MarkerWidget->SetWidgetClass(LockOnMarkerClass);
		MarkerWidget->SetWidgetSpace(EWidgetSpace::Screen);
		MarkerWidget->SetDrawSize(FVector2D(48.f, 48.f));
		MarkerWidget->SetupAttachment(CurrentTarget->GetRootComponent());
		MarkerWidget->RegisterComponent();
		MarkerWidget->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	}

	SetComponentTickEnabled(CurrentTarget != nullptr);
	OnTargetChanged.Broadcast(CurrentTarget);
}

void UACRPGTargetingComponent::ClearTarget()
{
	SetTarget(nullptr);
}

void UACRPGTargetingComponent::SwitchTarget(bool bRight)
{
	if (!CurrentTarget || !OwnerCharacter)
	{
		return;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Origin = OwnerCharacter->GetActorLocation();
	const FVector Right = OwnerCharacter->GetBaseAimRotation().RotateVector(FVector::RightVector);
	const FVector ToCurrentDir = (CurrentTarget->GetActorLocation() - Origin).GetSafeNormal2D();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SwitchTarget), false, OwnerCharacter);
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(MaxLockDistance), Params);

	AActor* Best = nullptr;
	float BestDelta = TNumericLimits<float>::Max();

	for (const FOverlapResult& R : Overlaps)
	{
		AACRPGCharacterBase* C = Cast<AACRPGCharacterBase>(R.GetActor());
		if (!C || C == OwnerCharacter || C == CurrentTarget || !C->IsAlive())
		{
			continue;
		}

		const FVector ToC = (C->GetActorLocation() - Origin).GetSafeNormal2D();

		// Hozirgi nishondan kerakli tomondami?
		const float Side = FVector::DotProduct(Right, ToC - ToCurrentDir);
		if ((bRight && Side <= 0.f) || (!bRight && Side >= 0.f))
		{
			continue;
		}

		// Eng yaqin burchakdagisini olamiz — "sakrab" o'tmasin.
		const float Delta = FMath::Abs(Side);
		if (Delta < BestDelta)
		{
			BestDelta = Delta;
			Best = C;
		}
	}

	if (Best)
	{
		SetTarget(Best);
	}
}

void UACRPGTargetingComponent::UpdateLockValidity()
{
	if (!CurrentTarget || !OwnerCharacter)
	{
		return;
	}

	// Nishon o'ldimi?
	const AACRPGCharacterBase* TargetChar = Cast<AACRPGCharacterBase>(CurrentTarget);
	if (!TargetChar || !TargetChar->IsAlive())
	{
		// O'lgan bo'lsa — keyingi dushmanga avtomatik o'tamiz (yaxshi UX).
		AActor* Next = FindBestTarget();
		SetTarget(Next);
		return;
	}

	// Juda uzoqlashdimi?
	const float Dist = FVector::Dist(CurrentTarget->GetActorLocation(), OwnerCharacter->GetActorLocation());
	if (Dist > BreakLockDistance)
	{
		ClearTarget();
	}
}

// ---------------------------------------------------------------------------
// DODGE ROLL (Ep.#12)
// ---------------------------------------------------------------------------

void UACRPGTargetingComponent::PerformDodge()
{
	if (!OwnerCharacter || bIsDodging)
	{
		return;
	}

	if (!OwnerCharacter->CanStartAction())
	{
		return;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
	{
		return;
	}

	// Cooldown — ketma-ket dodge bilan "sirg'anib" ketib bo'lmasin.
	if (World->GetTimeSeconds() - LastDodgeTime < DodgeCooldown)
	{
		return;
	}

	// Chidamlilik.
	UACRPGStatsComponent* Stats = OwnerCharacter->GetStats();
	if (!Stats || !Stats->ConsumeStamina(DodgeStaminaCost))
	{
		return;
	}

	// --- Yo'nalishni aniqlaymiz ---
	// Kirish vektorini personaj o'qlariga proyeksiya qilamiz.
	const FVector InputDir = OwnerCharacter->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();

	UAnimMontage* Chosen = DodgeBackwardMontage;	// Kirish bo'lmasa — orqaga.

	if (!InputDir.IsNearlyZero())
	{
		const float ForwardDot = FVector::DotProduct(OwnerCharacter->GetActorForwardVector(), InputDir);
		const float RightDot   = FVector::DotProduct(OwnerCharacter->GetActorRightVector(), InputDir);

		if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
		{
			Chosen = ForwardDot > 0.f ? DodgeForwardMontage : DodgeBackwardMontage;
		}
		else
		{
			Chosen = RightDot > 0.f ? DodgeRightMontage : DodgeLeftMontage;
		}

		// Lock-on YO'Q bo'lsa, personajni kirish yo'nalishiga buramiz —
		// shunda oldinga dumalash haqiqatan ham oldinga bo'ladi.
		if (!CurrentTarget)
		{
			OwnerCharacter->SetActorRotation(FRotator(0.f, InputDir.Rotation().Yaw, 0.f));
			Chosen = DodgeForwardMontage;
		}
	}

	if (!Chosen)
	{
		return;
	}

	bIsDodging = true;
	LastDodgeTime = World->GetTimeSeconds();
	OwnerCharacter->SetCombatState(ECombatState::Dodging);
	OwnerCharacter->PlayAnimMontage(Chosen);

	// i-frames montajdagi Anim Notify orqali yoqiladi (SetInvulnerable).
	if (UAnimInstance* AnimInst = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UACRPGTargetingComponent::OnDodgeMontageEnded);
		AnimInst->Montage_SetEndDelegate(EndDelegate, Chosen);
	}
}

void UACRPGTargetingComponent::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsDodging = false;
	bInvulnerable = false;	// Xavfsizlik uchun — Notify ishlamay qolsa ham o'chadi.

	if (OwnerCharacter && OwnerCharacter->GetCombatState() == ECombatState::Dodging)
	{
		OwnerCharacter->SetCombatState(ECombatState::Idle);
	}
}
