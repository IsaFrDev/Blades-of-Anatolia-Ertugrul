#include "Components/ACRPGFootstepComponent.h"

#include "Core/ACRPG.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Perception/AISense_Hearing.h"

UACRPGFootstepComponent::UACRPGFootstepComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UACRPGFootstepComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ACharacter>(GetOwner());
}

const FACRPGFootstepEntry* UACRPGFootstepComponent::FindEntryForSurface(EPhysicalSurface Surface) const
{
	return FootstepEntries.FindByPredicate(
		[Surface](const FACRPGFootstepEntry& E) { return E.Surface == Surface; });
}

void UACRPGFootstepComponent::OnFootstep(FName FootSocket)
{
	if (!OwnerCharacter)
	{
		return;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!World || !Mesh)
	{
		return;
	}

	// Havoda qadam tovushi chiqmaydi.
	if (OwnerCharacter->GetCharacterMovement()->IsFalling())
	{
		return;
	}

	// --- Oyoq ostiga trace ---
	FVector Start = OwnerCharacter->GetActorLocation();
	if (Mesh->DoesSocketExist(FootSocket))
	{
		Start = Mesh->GetSocketLocation(FootSocket);
		Start.Z += 20.f;	// biroz yuqoridan boshlaymiz
	}

	const FVector End = Start - FVector(0.f, 0.f, TraceDistance);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(Footstep), false, OwnerCharacter);

	// SHU QATOR ENG MUHIMI — usiz PhysMaterial har doim nullptr bo'ladi.
	Params.bReturnPhysicalMaterial = true;

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return;
	}

	// --- Sirt turini aniqlaymiz ---
	EPhysicalSurface Surface = SurfaceType_Default;
	if (Hit.PhysMaterial.IsValid())
	{
		Surface = Hit.PhysMaterial->SurfaceType;
	}

	// --- Ovoz balandligi ---
	const bool bCrouched = OwnerCharacter->bIsCrouched;
	float Volume = VolumeMultiplier * (bCrouched ? CrouchVolumeMultiplier : 1.f);

	// --- Tovush va effekt ---
	const FACRPGFootstepEntry* Entry = FindEntryForSurface(Surface);

	USoundBase* Sound = (Entry && Entry->Sound) ? Entry->Sound.Get() : DefaultFootstepSound.Get();
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Hit.ImpactPoint, Volume);
	}

	// Ep.#55 — qumda chang ko'tariladi.
	if (Entry && Entry->Effect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, Entry->Effect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}

	// --- Ep.#24: AI eshitadigan shovqin ---
	if (!bMakeNoise || (bCrouched && bSilentWhenCrouched))
	{
		return;
	}

	// Tez yurayotgan bo'lsa balandroq shovqin.
	const float Speed = OwnerCharacter->GetVelocity().Size2D();
	const float MaxWalk = OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed;
	const float SpeedRatio = MaxWalk > 0.f ? FMath::Clamp(Speed / MaxWalk, 0.f, 1.f) : 0.f;

	const float Loudness = FMath::Lerp(WalkNoiseLoudness, SprintNoiseLoudness, SpeedRatio);

	// Bu yerda Instigator = o'yinchi. Ep.#27 dagi toshdan farqli o'laroq,
	// bu yerda AI shovqinni AYNAN o'yinchiga bog'lashi kerak.
	UAISense_Hearing::ReportNoiseEvent(
		World, Hit.ImpactPoint, Loudness, OwnerCharacter, 0.f, TEXT("Footstep"));
}
