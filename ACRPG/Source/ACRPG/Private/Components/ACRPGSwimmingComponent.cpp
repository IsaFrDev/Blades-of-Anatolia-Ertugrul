#include "Components/ACRPGSwimmingComponent.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGCharacterBase.h"
#include "Components/ACRPGStatsComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PhysicsVolume.h"

UACRPGSwimmingComponent::UACRPGSwimmingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UACRPGSwimmingComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AACRPGCharacterBase>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	MovementComponent = OwnerCharacter->GetCharacterMovement();
	CachedWalkSpeed = MovementComponent ? MovementComponent->MaxWalkSpeed : 500.f;

	// UE o'zi MOVE_Swimming ga o'tkazadi — biz faqat xabardor bo'lamiz.
	OwnerCharacter->MovementModeChangedDelegate.AddDynamic(
		this, &UACRPGSwimmingComponent::OnMovementModeChanged);

	SetComponentTickEnabled(false);
}

void UACRPGSwimmingComponent::OnMovementModeChanged(ACharacter* Character,
	EMovementMode PrevMode, uint8 PrevCustomMode)
{
	if (!MovementComponent)
	{
		return;
	}

	const bool bNowSwimming = MovementComponent->IsSwimming();

	if (bNowSwimming && !bIsSwimming)
	{
		EnterWater();
	}
	else if (!bNowSwimming && bIsSwimming)
	{
		ExitWater();
	}
}

void UACRPGSwimmingComponent::EnterWater()
{
	bIsSwimming = true;

	if (OwnerCharacter)
	{
		OwnerCharacter->SetCombatState(ECombatState::Swimming);
	}

	if (MovementComponent)
	{
		MovementComponent->MaxSwimSpeed = SwimSpeed;

		// Suv yuzasini Physics Volume dan taxminlaymiz.
		if (const APhysicsVolume* Volume = MovementComponent->GetPhysicsVolume())
		{
			if (Volume->GetRootComponent())
			{
				const FBoxSphereBounds Bounds = Volume->GetRootComponent()->Bounds;
				WaterSurfaceZ = Bounds.Origin.Z + Bounds.BoxExtent.Z;
				bHasWaterSurface = true;
			}
		}
	}

	SetComponentTickEnabled(true);
	UE_LOG(LogACRPG, Log, TEXT("Suvga kirdi."));
}

void UACRPGSwimmingComponent::ExitWater()
{
	bIsSwimming = false;
	bHasWaterSurface = false;
	SetComponentTickEnabled(false);

	if (MovementComponent)
	{
		MovementComponent->MaxWalkSpeed = CachedWalkSpeed;
	}

	if (OwnerCharacter && OwnerCharacter->GetCombatState() == ECombatState::Swimming)
	{
		OwnerCharacter->SetCombatState(ECombatState::Idle);
	}
}

void UACRPGSwimmingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsSwimming || !OwnerCharacter || !MovementComponent)
	{
		return;
	}

	// Chidamlilik.
	if (UACRPGStatsComponent* Stats = OwnerCharacter->GetStats())
	{
		Stats->ConsumeStamina(SwimStaminaDrain * DeltaTime);
	}

	if (!bHasWaterSurface)
	{
		return;
	}

	// --- Yuzada qolish ---
	// Personajni suv yuzasidan SurfaceOffset balandlikda ushlaymiz.
	const FVector Loc = OwnerCharacter->GetActorLocation();
	const float DesiredZ = WaterSurfaceZ + SurfaceOffset;
	const float DeltaZ = DesiredZ - Loc.Z;

	// Faqat pastda bo'lsa ko'taramiz — sho'ng'ishga ruxsat qoladi.
	if (DeltaZ > 1.f)
	{
		OwnerCharacter->AddActorWorldOffset(
			FVector(0.f, 0.f, DeltaZ * FMath::Min(1.f, DeltaTime * BuoyancyInterpSpeed)), true);
	}
}
