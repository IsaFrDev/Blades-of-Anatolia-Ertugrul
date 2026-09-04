#include "Components/ACRPGVaultingComponent.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGCharacterBase.h"

#include "MotionWarpingComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UACRPGVaultingComponent::UACRPGVaultingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;	// Faqat so'ralganda ishlaydi — Tick shart emas.
}

void UACRPGVaultingComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<AACRPGCharacterBase>(GetOwner());
}

// ---------------------------------------------------------------------------
// GEOMETRIYA QIDIRUVI (Ep.#3 ning yuragi)
// ---------------------------------------------------------------------------

bool UACRPGVaultingComponent::FindVaultTargets(FVector& OutStartPoint, FVector& OutLandPoint) const
{
	if (!OwnerCharacter)
	{
		return false;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
	if (!World || !Capsule)
	{
		return false;
	}

	const FVector ActorLoc = OwnerCharacter->GetActorLocation();
	const FVector Forward  = OwnerCharacter->GetActorForwardVector();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(Vault), false, OwnerCharacter);
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(TraceRadius);

	// -----------------------------------------------------------------
	// 1-BOSQICH: oldinga trace — to'siq bormi?
	// Ko'krak balandligidan tekshiramiz (oyoq ostidagi maydatoshlar hisobga olinmasin).
	// -----------------------------------------------------------------
	const FVector ForwardStart = ActorLoc + FVector(0.f, 0.f, -HalfHeight + MinVaultHeight);
	const FVector ForwardEnd   = ForwardStart + Forward * ForwardTraceDistance;

	FHitResult ForwardHit;
	const bool bHitObstacle = World->SweepSingleByChannel(
		ForwardHit, ForwardStart, ForwardEnd, FQuat::Identity, ECC_Visibility, Sphere, Params);

	if (bShowDebugTraces)
	{
		DrawDebugLine(World, ForwardStart, ForwardEnd, bHitObstacle ? FColor::Green : FColor::Red, false, 2.f, 0, 2.f);
	}

	if (!bHitObstacle)
	{
		return false;	// Oldinda hech narsa yo'q — oddiy sakrash.
	}

	// Juda qiya sirt bo'lsa (nishablik), bu to'siq emas.
	if (FMath::Abs(ForwardHit.ImpactNormal.Z) > 0.3f)
	{
		return false;
	}

	// -----------------------------------------------------------------
	// 2-BOSQICH: yuqoridan pastga trace — to'siq usti qayerda?
	// To'siqning ichiga biroz kirib, yuqoridan pastga tashlaymiz.
	// -----------------------------------------------------------------
	const FVector TopStart = ForwardHit.ImpactPoint
		+ Forward * (TraceRadius + 10.f)
		+ FVector(0.f, 0.f, MaxVaultHeight);
	const FVector TopEnd = TopStart - FVector(0.f, 0.f, MaxVaultHeight * 2.f);

	FHitResult TopHit;
	if (!World->SweepSingleByChannel(TopHit, TopStart, TopEnd, FQuat::Identity, ECC_Visibility, Sphere, Params))
	{
		return false;	// To'siq usti topilmadi — demak bu tekis devor.
	}

	if (bShowDebugTraces)
	{
		DrawDebugSphere(World, TopHit.ImpactPoint, 15.f, 12, FColor::Yellow, false, 2.f);
	}

	// Balandlik tekshiruvi: past bo'lsa qadam tashlaydi, baland bo'lsa tirmashadi.
	const float ObstacleHeight = TopHit.ImpactPoint.Z - (ActorLoc.Z - HalfHeight);
	if (ObstacleHeight < MinVaultHeight || ObstacleHeight > MaxVaultHeight)
	{
		return false;
	}

	// Usti qiya bo'lsa, unga chiqib bo'lmaydi.
	if (TopHit.ImpactNormal.Z < 0.7f)
	{
		return false;
	}

	OutStartPoint = TopHit.ImpactPoint;

	// -----------------------------------------------------------------
	// 3-BOSQICH: narigi tomonda qo'nish joyi bormi?
	// To'siqdan MaxVaultDepth oldinga o'tib, yerni qidiramiz.
	// -----------------------------------------------------------------
	const FVector LandProbeStart = OutStartPoint + Forward * MaxVaultDepth + FVector(0.f, 0.f, 20.f);
	const FVector LandProbeEnd   = LandProbeStart - FVector(0.f, 0.f, MaxVaultHeight * 3.f);

	FHitResult LandHit;
	if (!World->SweepSingleByChannel(LandHit, LandProbeStart, LandProbeEnd, FQuat::Identity, ECC_Visibility, Sphere, Params))
	{
		return false;	// Narigi tomon jarlik — sakramaymiz.
	}

	// Qo'nish nuqtasida personaj sig'adimi? (Ep.#13, #33 dagi "devordan o'tib ketish" bugini oldini oladi)
	const FVector CapsuleCenter = LandHit.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 5.f);
	if (World->OverlapAnyTestByChannel(
			CapsuleCenter, FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), HalfHeight), Params))
	{
		return false;	// U yerda boshqa narsa turibdi.
	}

	if (bShowDebugTraces)
	{
		DrawDebugSphere(World, LandHit.ImpactPoint, 15.f, 12, FColor::Blue, false, 2.f);
	}

	OutLandPoint = LandHit.ImpactPoint;
	return true;
}

// ---------------------------------------------------------------------------
// VAULT BOSHLASH
// ---------------------------------------------------------------------------

bool UACRPGVaultingComponent::TryVault()
{
	if (bIsVaulting || !OwnerCharacter || !VaultMontage)
	{
		return false;
	}

	if (!OwnerCharacter->CanStartAction())
	{
		return false;
	}

	// Havoda vault qilib bo'lmaydi.
	if (OwnerCharacter->GetCharacterMovement()->IsFalling())
	{
		return false;
	}

	FVector StartPoint, LandPoint;
	if (!FindVaultTargets(StartPoint, LandPoint))
	{
		return false;
	}

	// -----------------------------------------------------------------
	// MOTION WARPING
	//
	// Bu qismning butun ma'nosi shu: animatsiya "o'rtacha" to'siq uchun yasalgan,
	// lekin har bir to'siq boshqacha. Warp Target berib qo'ysak, UE animatsiyani
	// cho'zib/qisqartirib personajni AYNAN kerakli nuqtaga olib boradi.
	// -----------------------------------------------------------------
	UMotionWarpingComponent* Warping = OwnerCharacter->GetMotionWarping();
	if (!Warping)
	{
		UE_LOG(LogACRPG, Warning, TEXT("MotionWarpingComponent yo'q — vault ishlamaydi."));
		return false;
	}

	// Personaj to'siqqa yuzma-yuz turishi kerak.
	const FRotator FaceRotation = (LandPoint - OwnerCharacter->GetActorLocation()).Rotation();
	const FRotator YawOnly(0.f, FaceRotation.Yaw, 0.f);

	Warping->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName_Start, StartPoint, YawOnly);
	Warping->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName_Land,  LandPoint,  YawOnly);

	// Holatni belgilaymiz — boshqa harakatlar bloklanadi.
	bIsVaulting = true;
	OwnerCharacter->SetCombatState(ECombatState::Vaulting);

	// Animatsiya davomida gravitatsiya va to'qnashuv o'chiriladi:
	// aks holda personaj to'siqqa urilib qoladi.
	OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	OwnerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);

	OwnerCharacter->PlayAnimMontage(VaultMontage);

	// Montaj tugaganda holatni tiklaymiz.
	if (UAnimInstance* AnimInst = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UACRPGVaultingComponent::OnVaultMontageEnded);
		AnimInst->Montage_SetEndDelegate(EndDelegate, VaultMontage);
	}

	return true;
}

void UACRPGVaultingComponent::OnVaultMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsVaulting = false;

	if (!OwnerCharacter)
	{
		return;
	}

	// Hammasini qaytaramiz. Bu qadam unutilsa — personaj havoda uchib yuradi
	// (Ep.#33 dagi tuzatilgan buglardan biri aynan shu edi).
	OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	OwnerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	OwnerCharacter->SetCombatState(ECombatState::Idle);
}
