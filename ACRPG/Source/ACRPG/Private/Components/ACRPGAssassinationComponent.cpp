#include "Components/ACRPGAssassinationComponent.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGCharacterBase.h"
#include "AI/ACRPGEnemyController.h"

#include "MotionWarpingComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UACRPGAssassinationComponent::UACRPGAssassinationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UACRPGAssassinationComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<AACRPGCharacterBase>(GetOwner());
}

// ---------------------------------------------------------------------------
// NISHONNI TEKSHIRISH — uchta shart
// ---------------------------------------------------------------------------

bool UACRPGAssassinationComponent::IsValidTarget(const AACRPGCharacterBase* Candidate) const
{
	if (!Candidate || Candidate == OwnerCharacter || !OwnerCharacter)
	{
		return false;
	}

	// 0-shart: tirikmi?
	if (!Candidate->IsAlive())
	{
		return false;
	}

	// 2-shart: ORQADAmizmi?
	//
	// Dushmanning oldinga qaragan vektori bilan "dushmandan bizga" vektorini solishtiramiz.
	// Agar biz uning orqasida bo'lsak, bu ikki vektor QARAMA-QARSHI yo'nalgan bo'ladi,
	// ya'ni skalyar ko'paytma (dot product) MANFIY bo'ladi.
	const FVector VictimForward = Candidate->GetActorForwardVector();
	const FVector VictimToUs = (OwnerCharacter->GetActorLocation() - Candidate->GetActorLocation()).GetSafeNormal2D();

	const float Dot = FVector::DotProduct(VictimForward, VictimToUs);

	// Burchakni dot chegarasiga aylantiramiz.
	// BehindAngleDegrees = 90 bo'lsa -> cos(180-90) = 0 -> Dot < 0 bo'lishi kerak.
	const float DotThreshold = FMath::Cos(FMath::DegreesToRadians(180.f - BehindAngleDegrees));
	if (Dot > -DotThreshold)
	{
		return false;	// Biz uning oldida yoki yonida turibmiz.
	}

	// 3-shart: dushman bizni SEZMAGANmi? (Ep.#21 AI holati bilan bog'lanish)
	if (const AACRPGEnemyController* EnemyAI = Cast<AACRPGEnemyController>(Candidate->GetController()))
	{
		const EAIState State = EnemyAI->GetAIState();
		if (State == EAIState::Chasing || State == EAIState::Attacking)
		{
			return false;	// U bizni ko'rgan — endi bu adolatli jang.
		}
	}

	// To'siq yo'qmi? (devor orqali assassination bo'lmasin)
	FHitResult Blocking;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AssassinLOS), false, OwnerCharacter);
	Params.AddIgnoredActor(Candidate);

	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		Blocking,
		OwnerCharacter->GetActorLocation(),
		Candidate->GetActorLocation(),
		ECC_Visibility, Params);

	return !bBlocked;
}

AACRPGCharacterBase* UACRPGAssassinationComponent::FindAssassinationTarget() const
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

	// Cho'kkalab yurganda radius kengayadi — yashirin o'ynash rag'batlantiriladi.
	float Range = MaxRange;
	if (OwnerCharacter->bIsCrouched)
	{
		Range *= CrouchedRangeMultiplier;
	}

	// Atrofdagi barcha Pawn'larni bitta overlap bilan topamiz.
	// Bu har bir dushmanni alohida tekshirishdan (Get All Actors Of Class) ancha tez.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AssassinScan), false, OwnerCharacter);

	World->OverlapMultiByObjectType(
		Overlaps,
		OwnerCharacter->GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Range),
		Params);

	if (bShowDebug)
	{
		DrawDebugSphere(World, OwnerCharacter->GetActorLocation(), Range, 16, FColor::Cyan, false, 1.f);
	}

	// Eng yaqin haqiqiy nishonni tanlaymiz.
	AACRPGCharacterBase* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (const FOverlapResult& Result : Overlaps)
	{
		AACRPGCharacterBase* Candidate = Cast<AACRPGCharacterBase>(Result.GetActor());
		if (!IsValidTarget(Candidate))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(
			Candidate->GetActorLocation(), OwnerCharacter->GetActorLocation());

		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}

	return Best;
}

// ---------------------------------------------------------------------------
// BAJARISH
// ---------------------------------------------------------------------------

bool UACRPGAssassinationComponent::TryAssassinate()
{
	if (bIsAssassinating || !OwnerCharacter || !AssassinMontage)
	{
		return false;
	}

	if (!OwnerCharacter->CanStartAction())
	{
		return false;
	}

	AACRPGCharacterBase* Victim = FindAssassinationTarget();
	if (!Victim)
	{
		return false;
	}

	CurrentVictim = Victim;
	bIsAssassinating = true;

	// -----------------------------------------------------------------
	// MOTION WARPING: o'yinchini qurbonning orqasiga aniq joylashtiramiz.
	//
	// Nishon nuqtasi = qurbon joylashuvi - uning oldinga vektori * masofa.
	// Yo'nalish = qurbon qayerga qarasa, biz ham o'sha tomonga.
	// -----------------------------------------------------------------
	const FVector VictimLoc = Victim->GetActorLocation();
	const FVector VictimForward = Victim->GetActorForwardVector();

	const FVector WarpLocation = VictimLoc - VictimForward * WarpDistanceBehind;
	const FRotator WarpRotation = VictimForward.Rotation();

	if (UMotionWarpingComponent* Warping = OwnerCharacter->GetMotionWarping())
	{
		Warping->AddOrUpdateWarpTargetFromLocationAndRotation(
			WarpTargetName, WarpLocation, FRotator(0.f, WarpRotation.Yaw, 0.f));
	}

	// Ikkala personaj ham harakatsiz bo'ladi.
	OwnerCharacter->SetCombatState(ECombatState::Assassinating);
	OwnerCharacter->GetCharacterMovement()->DisableMovement();

	// Bir-biriga urilib ketmasliklari uchun to'qnashuvni vaqtincha o'chiramiz.
	OwnerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// Hujumchi animatsiyasi.
	OwnerCharacter->PlayAnimMontage(AssassinMontage);

	// Qurbon o'z animatsiyasini o'ynaydi va o'ladi.
	// (Bu chaqiruv ichida sog'liq 0 ga tushadi — ACRPGCharacterBase ga qarang.)
	Victim->ReceiveAssassination(OwnerCharacter);

	// Ep.#41 — kvest tizimi "dushman o'ldi" ni StatsComponent->OnDeath orqali eshitadi,
	// shuning uchun bu yerda alohida xabar berish shart emas.

	if (UAnimInstance* AnimInst = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UACRPGAssassinationComponent::OnAssassinationEnded);
		AnimInst->Montage_SetEndDelegate(EndDelegate, AssassinMontage);
	}

	UE_LOG(LogACRPG, Log, TEXT("Assassination: %s"), *Victim->GetName());
	return true;
}

void UACRPGAssassinationComponent::OnAssassinationEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsAssassinating = false;
	CurrentVictim = nullptr;

	if (!OwnerCharacter)
	{
		return;
	}

	OwnerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	OwnerCharacter->SetCombatState(ECombatState::Idle);
}
