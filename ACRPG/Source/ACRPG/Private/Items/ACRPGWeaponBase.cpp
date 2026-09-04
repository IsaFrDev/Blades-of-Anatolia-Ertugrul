#include "Items/ACRPGWeaponBase.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

AACRPGWeaponBase::AACRPGWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

USceneComponent* AACRPGWeaponBase::GetWeaponMesh() const
{
	return ItemMesh;
}

void AACRPGWeaponBase::PlayImpactEffects(const FVector& Location, const FVector& Normal)
{
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Location);
	}
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, Location, Normal.Rotation());
	}
}
