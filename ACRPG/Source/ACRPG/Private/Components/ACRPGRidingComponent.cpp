#include "Components/ACRPGRidingComponent.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGCharacterBase.h"
#include "AI/ACRPGAnimalController.h"

#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

UACRPGRidingComponent::UACRPGRidingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UACRPGRidingComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<AACRPGCharacterBase>(GetOwner());
}

AACRPGCharacterBase* UACRPGRidingComponent::FindMountableAnimal() const
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

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FindMount), false, OwnerCharacter);

	World->OverlapMultiByObjectType(
		Overlaps, OwnerCharacter->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(MountRange), Params);

	for (const FOverlapResult& R : Overlaps)
	{
		AACRPGCharacterBase* Candidate = Cast<AACRPGCharacterBase>(R.GetActor());
		if (!Candidate || Candidate == OwnerCharacter || !Candidate->IsAlive())
		{
			continue;
		}

		// Faqat "Mountable" turidagi hayvon.
		const AACRPGAnimalController* AnimalAI = Cast<AACRPGAnimalController>(Candidate->GetController());
		if (AnimalAI && AnimalAI->GetBehavior() == EAnimalBehavior::Mountable)
		{
			return Candidate;
		}
	}

	return nullptr;
}

bool UACRPGRidingComponent::TryMount()
{
	if (IsRiding() || !OwnerCharacter || !OwnerCharacter->CanStartAction())
	{
		return false;
	}

	AACRPGCharacterBase* Animal = FindMountableAnimal();
	if (!Animal)
	{
		return false;
	}

	MountedAnimal = Animal;

	// --- Hayvon AI sini o'chiramiz ---
	if (AACRPGAnimalController* AnimalAI = Cast<AACRPGAnimalController>(Animal->GetController()))
	{
		AnimalAI->SetMounted(true);
	}

	// --- O'yinchini egarga o'tqazamiz ---
	USkeletalMeshComponent* AnimalMesh = Animal->GetMesh();
	if (AnimalMesh && AnimalMesh->DoesSocketExist(SaddleSocket))
	{
		OwnerCharacter->AttachToComponent(
			AnimalMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			SaddleSocket);
	}
	else
	{
		OwnerCharacter->AttachToActor(Animal, FAttachmentTransformRules::KeepRelativeTransform);
	}

	// O'yinchi tanasi endi harakat qilmaydi — hayvon harakat qiladi.
	OwnerCharacter->GetCharacterMovement()->DisableMovement();
	OwnerCharacter->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OwnerCharacter->SetCombatState(ECombatState::Riding);

	// Hayvon tezligini oshiramiz.
	Animal->GetCharacterMovement()->MaxWalkSpeed = MountedSpeed;

	if (MountMontage)
	{
		OwnerCharacter->PlayAnimMontage(MountMontage);
	}

	UE_LOG(LogACRPG, Log, TEXT("Hayvonga mindi: %s"), *Animal->GetName());
	return true;
}

void UACRPGRidingComponent::AddRideInput(const FVector2D& Input)
{
	if (!MountedAnimal || !OwnerCharacter)
	{
		return;
	}

	// Kamera yo'nalishiga nisbatan harakat — o'yinchi kutgani shu.
	const AController* PC = OwnerCharacter->GetController();
	const FRotator ControlRot = PC ? PC->GetControlRotation() : MountedAnimal->GetActorRotation();
	const FRotator YawOnly(0.f, ControlRot.Yaw, 0.f);

	const FVector Forward = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

	MountedAnimal->AddMovementInput(Forward, Input.Y);
	MountedAnimal->AddMovementInput(Right, Input.X);
}

void UACRPGRidingComponent::Dismount()
{
	if (!MountedAnimal || !OwnerCharacter)
	{
		return;
	}

	// --- Ajratamiz ---
	OwnerCharacter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// Yon tomonga qo'yamiz — hayvon ichida qolib ketmasin.
	const FVector DismountLocation = MountedAnimal->GetActorLocation()
		- MountedAnimal->GetActorRightVector() * DismountOffset;

	OwnerCharacter->SetActorLocation(DismountLocation, true);

	OwnerCharacter->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	OwnerCharacter->SetCombatState(ECombatState::Idle);

	// --- Hayvon AI sini qaytaramiz ---
	if (AACRPGAnimalController* AnimalAI = Cast<AACRPGAnimalController>(MountedAnimal->GetController()))
	{
		AnimalAI->SetMounted(false);
	}

	MountedAnimal = nullptr;
	UE_LOG(LogACRPG, Log, TEXT("Hayvondan tushdi."));
}
