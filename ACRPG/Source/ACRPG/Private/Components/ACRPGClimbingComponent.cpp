#include "Components/ACRPGClimbingComponent.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGCharacterBase.h"
#include "Components/ACRPGStatsComponent.h"

#include "MotionWarpingComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UACRPGClimbingComponent::UACRPGClimbingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UACRPGClimbingComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AACRPGCharacterBase>(GetOwner());
	if (OwnerCharacter)
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement();
	}

	SetComponentTickEnabled(false);	// Faqat tirmashayotganda kerak.
}

void UACRPGClimbingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsClimbing || bIsMantling)
	{
		return;
	}

	// Ep.#6 — chidamlilik tugasa tushib ketamiz.
	if (OwnerCharacter)
	{
		if (UACRPGStatsComponent* Stats = OwnerCharacter->GetStats())
		{
			if (!Stats->ConsumeStamina(ClimbStaminaDrain * DeltaTime))
			{
				UE_LOG(LogACRPG, Log, TEXT("Chidamlilik tugadi — tushib ketdi."));
				StopClimb();
				return;
			}
		}
	}

	UpdateClimbMovement(DeltaTime);
}

// ---------------------------------------------------------------------------
// DEVOR ANIQLASH (Ep.#30)
// ---------------------------------------------------------------------------

bool UACRPGClimbingComponent::DetectWall(FHitResult& OutHit) const
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

	const FVector Start = OwnerCharacter->GetActorLocation();
	const FVector End = Start + OwnerCharacter->GetActorForwardVector() * WallDetectionDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ClimbWall), false, OwnerCharacter);

	// Sphere sweep — ingichka trace devor burchagida "adashib" ketmasin.
	const bool bHit = World->SweepSingleByChannel(
		OutHit, Start, End, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(20.f), Params);

	if (bShowDebug)
	{
		DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 0.1f);
	}

	if (!bHit)
	{
		return false;
	}

	// Yetarlicha tik bo'lishi kerak — nishablikka tirmashib bo'lmaydi.
	return FMath::Abs(OutHit.ImpactNormal.Z) <= MaxWallNormalZ;
}

// ---------------------------------------------------------------------------
// BOSHLASH / TO'XTATISH
// ---------------------------------------------------------------------------

bool UACRPGClimbingComponent::TryStartClimb()
{
	if (bIsClimbing || !OwnerCharacter || !MovementComponent)
	{
		return false;
	}

	if (!OwnerCharacter->CanStartAction())
	{
		return false;
	}

	FHitResult WallHit;
	if (!DetectWall(WallHit))
	{
		return false;
	}

	CurrentWallNormal = WallHit.ImpactNormal;
	bIsClimbing = true;

	// --- Fizikani "qo'lga olamiz" ---
	// MOVE_Flying = gravitatsiya yo'q, biz istagan tomonga harakat.
	MovementComponent->SetMovementMode(MOVE_Flying);
	MovementComponent->StopMovementImmediately();
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->MaxFlySpeed = ClimbSpeed;
	MovementComponent->BrakingDecelerationFlying = 1000.f;

	OwnerCharacter->SetCombatState(ECombatState::Climbing);
	SetComponentTickEnabled(true);

	UE_LOG(LogACRPG, Log, TEXT("Tirmashish boshlandi."));
	return true;
}

void UACRPGClimbingComponent::StopClimb()
{
	if (!bIsClimbing)
	{
		return;
	}

	bIsClimbing = false;
	ClimbInput = FVector2D::ZeroVector;
	SetComponentTickEnabled(false);

	// --- Fizikani QAYTARAMIZ ---
	// Ep.#33: bu blokni unutish = personaj havoda muzlab qoladi.
	if (MovementComponent)
	{
		MovementComponent->SetMovementMode(MOVE_Falling);
		MovementComponent->bOrientRotationToMovement = true;
	}

	if (OwnerCharacter && OwnerCharacter->GetCombatState() == ECombatState::Climbing)
	{
		OwnerCharacter->SetCombatState(ECombatState::Idle);
	}
}

// ---------------------------------------------------------------------------
// HAR KADRDAGI HARAKAT (Ep.#30-31)
// ---------------------------------------------------------------------------

void UACRPGClimbingComponent::AddClimbInput(const FVector2D& Input)
{
	ClimbInput = Input;
}

void UACRPGClimbingComponent::UpdateClimbMovement(float DeltaTime)
{
	if (!OwnerCharacter || !MovementComponent)
	{
		return;
	}

	// --- 1) Devor hali bormi? ---
	FHitResult WallHit;
	if (!DetectWall(WallHit))
	{
		// Devor tugadi. Yuqoriga chiqayotgan bo'lsak — bu qirra, mantling qilamiz.
		if (ClimbInput.Y > 0.1f && TryMantle())
		{
			return;
		}

		// Aks holda tushib ketamiz.
		StopClimb();
		return;
	}

	CurrentWallNormal = WallHit.ImpactNormal;

	// --- 2) Devor yuzasidan aniq masofada ushlab turamiz ---
	// Aks holda personaj asta-sekin devor ichiga kirib ketadi yoki uzoqlashadi.
	const FVector DesiredLocation = WallHit.ImpactPoint + CurrentWallNormal * ClimbDistanceFromWall;
	const FVector CurrentLocation = OwnerCharacter->GetActorLocation();

	// Faqat gorizontal tuzatish — vertikal harakatga tegmaymiz.
	FVector Correction = DesiredLocation - CurrentLocation;
	Correction.Z = 0.f;

	if (!Correction.IsNearlyZero())
	{
		OwnerCharacter->AddActorWorldOffset(Correction * FMath::Min(1.f, DeltaTime * 8.f), true);
	}

	// --- 3) Yuzni devorga qaratamiz ---
	// Devor normali bizga QARAB turadi, shuning uchun teskarisini olamiz.
	const FRotator TargetRotation = (-CurrentWallNormal).Rotation();
	const FRotator SmoothRotation = FMath::RInterpTo(
		OwnerCharacter->GetActorRotation(),
		FRotator(0.f, TargetRotation.Yaw, 0.f),
		DeltaTime, WallAlignSpeed);

	OwnerCharacter->SetActorRotation(SmoothRotation);

	// --- 4) Kirish bo'yicha harakat ---
	// Devor tekisligidagi ikkita o'q:
	//   Yuqoriga = dunyo Z (devor tik bo'lgani uchun soddalashtiramiz)
	//   Yon      = devor normali x yuqoriga
	const FVector WallUp = FVector::UpVector;
	const FVector WallRight = FVector::CrossProduct(CurrentWallNormal, WallUp).GetSafeNormal();

	const FVector MoveDirection =
		WallUp * ClimbInput.Y + WallRight * ClimbInput.X;

	if (!MoveDirection.IsNearlyZero())
	{
		OwnerCharacter->AddMovementInput(MoveDirection.GetSafeNormal(), 1.f);
	}
}

// ---------------------------------------------------------------------------
// Ep.#32 — MANTLING (qirraga chiqish)
// ---------------------------------------------------------------------------

bool UACRPGClimbingComponent::TryMantle()
{
	if (bIsMantling || !OwnerCharacter || !MantleMontage)
	{
		return false;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
	if (!World || !Capsule)
	{
		return false;
	}

	const FVector ActorLoc = OwnerCharacter->GetActorLocation();
	const FVector Forward = OwnerCharacter->GetActorForwardVector();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(Mantle), false, OwnerCharacter);

	// Boshdan yuqoriroqdan, oldinga siljib, pastga trace — qirra usti qayerda?
	const FVector TopStart = ActorLoc
		+ FVector(0.f, 0.f, LedgeCheckHeight)
		+ Forward * (ClimbDistanceFromWall + 40.f);

	const FVector TopEnd = TopStart - FVector(0.f, 0.f, LedgeCheckHeight * 1.5f);

	FHitResult LedgeHit;
	if (!World->SweepSingleByChannel(LedgeHit, TopStart, TopEnd, FQuat::Identity,
		ECC_Visibility, FCollisionShape::MakeSphere(20.f), Params))
	{
		return false;	// Qirra topilmadi.
	}

	// Usti tekis bo'lishi kerak.
	if (LedgeHit.ImpactNormal.Z < 0.7f)
	{
		return false;
	}

	// Personaj u yerda sig'adimi? (Ep.#33 bug fix)
	const FVector StandLocation = LedgeHit.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 5.f);
	if (World->OverlapAnyTestByChannel(StandLocation, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), HalfHeight), Params))
	{
		return false;
	}

	if (bShowDebug)
	{
		DrawDebugSphere(World, StandLocation, 20.f, 12, FColor::Magenta, false, 3.f);
	}

	// --- Motion Warping bilan aniq nuqtaga chiqamiz ---
	bIsMantling = true;

	if (UMotionWarpingComponent* Warping = OwnerCharacter->GetMotionWarping())
	{
		Warping->AddOrUpdateWarpTargetFromLocationAndRotation(
			MantleWarpTargetName, StandLocation, OwnerCharacter->GetActorRotation());
	}

	OwnerCharacter->PlayAnimMontage(MantleMontage);

	if (UAnimInstance* AnimInst = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UACRPGClimbingComponent::OnMantleMontageEnded);
		AnimInst->Montage_SetEndDelegate(EndDelegate, MantleMontage);
	}

	return true;
}

void UACRPGClimbingComponent::OnMantleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsMantling = false;

	// Tirmashishni to'xtatamiz va normal yurishga qaytamiz.
	bIsClimbing = false;
	SetComponentTickEnabled(false);

	if (MovementComponent)
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->bOrientRotationToMovement = true;
	}

	if (OwnerCharacter)
	{
		OwnerCharacter->SetCombatState(ECombatState::Idle);
	}
}
