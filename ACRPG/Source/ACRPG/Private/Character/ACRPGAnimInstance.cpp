#include "Character/ACRPGAnimInstance.h"

#include "Character/ACRPGCharacterBase.h"
#include "Character/ACRPGPlayerCharacter.h"
#include "Components/ACRPGCombatComponent.h"
#include "Components/ACRPGClimbingComponent.h"
#include "Components/ACRPGEquipmentComponent.h"
#include "Components/ACRPGStatsComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetAnimationLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"

void UACRPGAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Bir marta keshlaymiz — har kadr cast qilishning o'rniga.
	OwnerCharacter = Cast<AACRPGCharacterBase>(TryGetPawnOwner());
	if (OwnerCharacter)
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement();
	}
}

void UACRPGAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter || !MovementComponent)
	{
		return;
	}

	// -----------------------------------------------------------------------
	// Ep.#2 — Blendspace o'zgaruvchilari
	// -----------------------------------------------------------------------
	const FVector Velocity = MovementComponent->Velocity;

	// Z ni tashlab yuboramiz: sakraganda "yugurish" animatsiyasi chiqmasin.
	GroundSpeed = Velocity.Size2D();

	// Direction: -180..180. Strafe blendspace shu qiymatga qarab chap/o'ng tanlaydi.
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, OwnerCharacter->GetActorRotation());

	bHasAcceleration	= MovementComponent->GetCurrentAcceleration().SizeSquared2D() > 0.f;
	bIsInAir			= MovementComponent->IsFalling();
	bIsCrouching		= OwnerCharacter->bIsCrouched;
	VerticalVelocity	= Velocity.Z;

	// -----------------------------------------------------------------------
	// Holat bayroqlari
	// -----------------------------------------------------------------------
	CombatState	= OwnerCharacter->GetCombatState();
	bIsDead		= (CombatState == ECombatState::Dead);
	bIsClimbing	= (CombatState == ECombatState::Climbing);
	bIsSwimming	= (CombatState == ECombatState::Swimming);
	bIsBlocking	= (CombatState == ECombatState::Blocking);

	if (const UACRPGStatsComponent* Stats = OwnerCharacter->GetStats())
	{
		bIsSprinting = Stats->IsSprinting();
	}

	// Ep.#19, #47 — qurol holati
	if (const AACRPGPlayerCharacter* Player = Cast<AACRPGPlayerCharacter>(OwnerCharacter))
	{
		if (const UACRPGEquipmentComponent* Equip = Player->GetEquipment())
		{
			bIsAiming		= Equip->IsAiming();
			bHasWeaponDrawn	= Equip->IsWeaponDrawn();
		}
	}

	// -----------------------------------------------------------------------
	// Ep.#47 — Aim Offset
	// Kamera qayerga qarayotgani bilan tananing yo'nalishi orasidagi farq.
	// -----------------------------------------------------------------------
	{
		const FRotator ControlRot = OwnerCharacter->GetBaseAimRotation();
		const FRotator ActorRot = OwnerCharacter->GetActorRotation();
		const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(ControlRot, ActorRot);

		// Silliq o'tish — aks holda kamera keskin burilganda bosh "sakraydi".
		AimPitch = FMath::FInterpTo(AimPitch, FMath::ClampAngle(Delta.Pitch, -90.f, 90.f), DeltaSeconds, 15.f);
		AimYaw   = FMath::FInterpTo(AimYaw,   FMath::ClampAngle(Delta.Yaw,   -90.f, 90.f), DeltaSeconds, 15.f);
	}

	// -----------------------------------------------------------------------
	// Ep.#25-26 — Foot IK
	//
	// Muhim: IK faqat yerda va harakatsiz-yaqin holatda ishlashi kerak.
	// Ep.#26 dagi "anims fix" aynan shu haqda edi — havoda IK oyoqni cho'zib yuborardi.
	// -----------------------------------------------------------------------
	if (!bEnableFootIK || bIsInAir || bIsClimbing || bIsSwimming || bIsDead)
	{
		LeftFootOffset  = FMath::FInterpTo(LeftFootOffset,  0.f, DeltaSeconds, IKInterpSpeed);
		RightFootOffset = FMath::FInterpTo(RightFootOffset, 0.f, DeltaSeconds, IKInterpSpeed);
		HipOffset       = FMath::FInterpTo(HipOffset,       0.f, DeltaSeconds, IKInterpSpeed);
		return;
	}

	FRotator LeftRot = FRotator::ZeroRotator;
	FRotator RightRot = FRotator::ZeroRotator;

	const float LeftTarget  = TraceFoot(LeftFootSocket,  LeftRot);
	const float RightTarget = TraceFoot(RightFootSocket, RightRot);

	// Tos suyagi pastroq oyoqqa qarab tushadi (manfiy qiymat).
	const float HipTarget = FMath::Min(0.f, FMath::Min(LeftTarget, RightTarget));

	// Har bir oyoq tos tushganidan keyingi farqni qoplaydi.
	LeftFootOffset  = FMath::FInterpTo(LeftFootOffset,  LeftTarget  - HipTarget, DeltaSeconds, IKInterpSpeed);
	RightFootOffset = FMath::FInterpTo(RightFootOffset, RightTarget - HipTarget, DeltaSeconds, IKInterpSpeed);
	HipOffset       = FMath::FInterpTo(HipOffset,       HipTarget,               DeltaSeconds, IKInterpSpeed);

	LeftFootRotation  = FMath::RInterpTo(LeftFootRotation,  LeftRot,  DeltaSeconds, IKInterpSpeed);
	RightFootRotation = FMath::RInterpTo(RightFootRotation, RightRot, DeltaSeconds, IKInterpSpeed);
}

/**
 * Ep.#25 — bitta oyoq uchun vertikal trace.
 *
 * Oyoq suyagining X/Y joylashuvini olamiz, lekin Z ni kapsula tagidan boshlaymiz.
 * Shunday qilganda animatsiya oyoqni qanchalik ko'targanidan qat'i nazar,
 * har doim bir xil balandlikdan trace qilamiz.
 */
float UACRPGAnimInstance::TraceFoot(const FName& Socket, FRotator& OutRotation) const
{
	OutRotation = FRotator::ZeroRotator;

	const USkeletalMeshComponent* Mesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
	const UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;
	if (!Mesh || !World || !Mesh->DoesSocketExist(Socket))
	{
		return 0.f;
	}

	const FVector FootLocation = Mesh->GetSocketLocation(Socket);
	const FVector ActorLocation = OwnerCharacter->GetActorLocation();
	const float CapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	// Kapsula markazidan boshlab, oyoq ostidan IKTraceDistance pastgacha.
	const FVector Start(FootLocation.X, FootLocation.Y, ActorLocation.Z);
	const FVector End(FootLocation.X, FootLocation.Y, ActorLocation.Z - CapsuleHalfHeight - IKTraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FootIK), false, OwnerCharacter);
	Params.bReturnPhysicalMaterial = false;

	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return 0.f;
	}

	// Yer bilan kapsula tagi orasidagi farq.
	const float FootFloorZ = Hit.ImpactPoint.Z;
	const float CapsuleBottomZ = ActorLocation.Z - CapsuleHalfHeight;
	const float Offset = FootFloorZ - CapsuleBottomZ;

	// Qiya sirtda oyoqni yotqizish uchun burchak (Ep.#26).
	// Normal vektorining X/Y komponentlaridan Roll va Pitch chiqaramiz.
	const FVector N = Hit.ImpactNormal;
	OutRotation = FRotator(
		-FMath::RadiansToDegrees(FMath::Atan2(N.X, N.Z)),	// Pitch
		0.f,												// Yaw
		FMath::RadiansToDegrees(FMath::Atan2(N.Y, N.Z))		// Roll
	);

	// Juda katta farqlarni cheklaymiz — zinapoyada oyoq cho'zilib ketmasin.
	return FMath::Clamp(Offset, -IKTraceDistance, IKTraceDistance);
}
