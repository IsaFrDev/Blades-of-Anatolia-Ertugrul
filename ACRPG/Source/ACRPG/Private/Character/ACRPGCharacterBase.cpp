#include "Character/ACRPGCharacterBase.h"

#include "Core/ACRPG.h"
#include "Core/ACRPGGameMode.h"
#include "Components/ACRPGStatsComponent.h"
#include "Components/ACRPGCombatComponent.h"
#include "Components/ACRPGTargetingComponent.h"
#include "Components/ACRPGFootstepComponent.h"

#include "MotionWarpingComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "Engine/DamageEvents.h"

AACRPGCharacterBase::AACRPGCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// --- Ep.#2: harakat sozlamalari ---
	// Blueprint'da bu qiymatlar Character Movement komponentida qo'lda kiritilgan edi.
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;					// personaj yurish yo'nalishiga buriladi
	Move->RotationRate = FRotator(0.f, 500.f, 0.f);
	Move->MaxWalkSpeed = 500.f;
	Move->MaxWalkSpeedCrouched = 250.f;						// Ep.#2 cho'kkalash
	Move->NavAgentProps.bCanCrouch = true;
	Move->JumpZVelocity = 500.f;
	Move->AirControl = 0.25f;
	Move->BrakingDecelerationWalking = 2000.f;

	// Kamera personajni aylantirmaydi — faqat harakat aylantiradi.
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// --- Komponentlar ---
	StatsComponent = CreateDefaultSubobject<UACRPGStatsComponent>(TEXT("StatsComponent"));
	CombatComponent = CreateDefaultSubobject<UACRPGCombatComponent>(TEXT("CombatComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	FootstepComponent = CreateDefaultSubobject<UACRPGFootstepComponent>(TEXT("Footsteps"));

	// Ep.#10: qilich trace'i mesh'ni urmasligi uchun mesh'ga alohida kanal beriladi
	// (loyihada "SwordTrace" nomli Object Channel yaratiladi).
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

void AACRPGCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// Stats komponenti "men o'ldim" deb aytganda HandleDeath chaqirilsin.
	if (StatsComponent)
	{
		StatsComponent->OnDeath.AddDynamic(this, &AACRPGCharacterBase::HandleDeath);
	}
}

// ---------------------------------------------------------------------------
// HOLAT
// ---------------------------------------------------------------------------

void AACRPGCharacterBase::SetCombatState(ECombatState NewState)
{
	if (CombatState == NewState)
	{
		return;
	}

	// O'lgandan keyin holat o'zgarmaydi — bu ko'p bug'ning oldini oladi (Ep.#82).
	if (CombatState == ECombatState::Dead)
	{
		return;
	}

	const ECombatState OldState = CombatState;
	CombatState = NewState;
	OnCombatStateChanged(OldState, NewState);
}

void AACRPGCharacterBase::OnCombatStateChanged(ECombatState OldState, ECombatState NewState)
{
	// Bazada bo'sh — hosila klasslar (o'yinchi/dushman) o'zi to'ldiradi.
}

bool AACRPGCharacterBase::IsAlive() const
{
	return CombatState != ECombatState::Dead
		&& StatsComponent != nullptr
		&& StatsComponent->GetHealth() > 0.f;
}

bool AACRPGCharacterBase::CanMove() const
{
	switch (CombatState)
	{
	case ECombatState::Dead:
	case ECombatState::HitReacting:
	case ECombatState::Assassinating:
	case ECombatState::Vaulting:
		return false;
	default:
		return true;
	}
}

bool AACRPGCharacterBase::CanStartAction() const
{
	return IsAlive()
		&& CombatState != ECombatState::HitReacting
		&& CombatState != ECombatState::Assassinating
		&& CombatState != ECombatState::Vaulting
		&& CombatState != ECombatState::Dodging;
}

// ---------------------------------------------------------------------------
// URISH (Ep.#6, #10, #11, #60)
// ---------------------------------------------------------------------------

float AACRPGCharacterBase::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!IsAlive() || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	// Ep.#12 — dodge paytidagi "i-frames": urish umuman o'tmaydi.
	// Bu tekshiruv blokdan OLDIN bo'lishi kerak.
	if (const UACRPGTargetingComponent* Targeting = FindComponentByClass<UACRPGTargetingComponent>())
	{
		if (Targeting->IsInvulnerable())
		{
			return 0.f;
		}
	}

	// Ep.#69-70 — blok qilayotgan bo'lsa, urish kamayadi yoki umuman o'tmaydi.
	float FinalDamage = DamageAmount;
	if (CombatComponent)
	{
		const FVector AttackerLoc = DamageCauser ? DamageCauser->GetActorLocation() : GetActorLocation();
		FinalDamage = CombatComponent->ModifyIncomingDamage(FinalDamage, AttackerLoc);
	}

	if (FinalDamage <= 0.f)
	{
		return 0.f;	// To'liq bloklandi
	}

	// Ep.#44 — zirh himoyasi StatsComponent ichida hisoblanadi.
	Super::TakeDamage(FinalDamage, DamageEvent, EventInstigator, DamageCauser);

	if (StatsComponent)
	{
		StatsComponent->ApplyDamage(FinalDamage, DamageCauser);
	}

	// --- Ep.#11: qon effekti ---
	FVector HitLocation = GetActorLocation();
	FVector HitNormal = -GetActorForwardVector();

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
		HitLocation = PointEvent->HitInfo.ImpactPoint;
		HitNormal = PointEvent->ShotDirection;
	}
	PlayHitFeedback(HitLocation, HitNormal);

	// --- Ep.#10: hit reaction (agar hali tirik bo'lsa) ---
	if (IsAlive() && DamageCauser)
	{
		PlayHitReaction(DamageCauser->GetActorLocation());
	}

	return FinalDamage;
}

EHitDirection AACRPGCharacterBase::GetHitDirectionFrom(const FVector& SourceLocation) const
{
	// Zarba manbaiga qarab burchak hisoblaymiz.
	// Blueprint'da bu "Delta Rotator -> Break -> branch" zanjiri edi.
	const FVector ToSource = (SourceLocation - GetActorLocation()).GetSafeNormal2D();
	const FVector Forward = GetActorForwardVector();
	const FVector Right = GetActorRightVector();

	const float ForwardDot = FVector::DotProduct(Forward, ToSource);
	const float RightDot = FVector::DotProduct(Right, ToSource);

	// |ForwardDot| katta bo'lsa — old yoki orqa; aks holda yon.
	if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
	{
		return ForwardDot > 0.f ? EHitDirection::Front : EHitDirection::Back;
	}
	return RightDot > 0.f ? EHitDirection::Right : EHitDirection::Left;
}

void AACRPGCharacterBase::PlayHitFeedback(const FVector& HitLocation, const FVector& HitNormal)
{
	if (BloodEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, BloodEffect, HitLocation, HitNormal.Rotation());
	}
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, HitLocation);
	}
}

void AACRPGCharacterBase::PlayHitReaction(const FVector& SourceLocation)
{
	// Hujum qilayotgan paytda zarba yesa — hujum bekor bo'ladi (Ep.#10).
	const EHitDirection Dir = GetHitDirectionFrom(SourceLocation);

	if (TObjectPtr<UAnimMontage> const* Found = HitReactionMontages.Find(Dir))
	{
		if (*Found)
		{
			SetCombatState(ECombatState::HitReacting);
			PlayAnimMontage(*Found);

			// Montaj tugagach avtomatik Idle'ga qaytamiz.
			GetWorldTimerManager().SetTimer(
				HitStunTimer, this, &AACRPGCharacterBase::ClearHitStun, HitStunDuration, false);
		}
	}
}

void AACRPGCharacterBase::ClearHitStun()
{
	if (CombatState == ECombatState::HitReacting)
	{
		SetCombatState(ECombatState::Idle);
	}
}

// ---------------------------------------------------------------------------
// ASSASSINATION (Ep.#4)
// ---------------------------------------------------------------------------

void AACRPGCharacterBase::ReceiveAssassination(AACRPGCharacterBase* Assassin)
{
	if (!IsAlive())
	{
		return;
	}

	SetCombatState(ECombatState::Assassinating);

	if (AssassinatedMontage)
	{
		PlayAnimMontage(AssassinatedMontage);
	}

	// Assassination = bir zarbada o'lim. Sog'liqni 0 ga tushiramiz.
	if (StatsComponent)
	{
		StatsComponent->ApplyDamage(StatsComponent->GetHealth() + 1.f, Assassin);
	}
}

// ---------------------------------------------------------------------------
// O'LIM (Ep.#68)
// ---------------------------------------------------------------------------

void AACRPGCharacterBase::HandleDeath(AActor* Killer)
{
	if (CombatState == ECombatState::Dead)
	{
		return;	// Ikki marta o'lmasin (Ep.#82 dagi bug).
	}

	SetCombatState(ECombatState::Dead);

	// Harakat va to'qnashuvni o'chiramiz.
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Assassination montaji allaqachon o'ynayotgan bo'lsa, ustiga o'lim qo'ymaymiz.
	const bool bAlreadyPlaying = GetCurrentMontage() == AssassinatedMontage && AssassinatedMontage;

	if (!bAlreadyPlaying && DeathMontages.Num() > 0)
	{
		const int32 Index = FMath::RandRange(0, DeathMontages.Num() - 1);
		if (DeathMontages[Index])
		{
			PlayAnimMontage(DeathMontages[Index]);
		}
	}
	else if (DeathMontages.Num() == 0)
	{
		// Montaj yo'q bo'lsa — ragdoll (Ep.#11 dagi muqobil yechim).
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetMesh()->SetSimulatePhysics(true);
	}

	// O'yinchi bo'lsa — GameMode respawn qiladi (Ep.#68).
	if (AController* MyController = GetController())
	{
		if (MyController->IsPlayerController())
		{
			if (AACRPGGameMode* GM = GetWorld()->GetAuthGameMode<AACRPGGameMode>())
			{
				GM->HandlePlayerDeath(MyController);
			}
			return;	// O'yinchi tanasi o'chirilmaydi
		}

		// AI bo'lsa — miyasini o'chiramiz.
		MyController->UnPossess();
		MyController->Destroy();
	}

	if (CorpseLifeSpan > 0.f)
	{
		SetLifeSpan(CorpseLifeSpan);
	}
}
