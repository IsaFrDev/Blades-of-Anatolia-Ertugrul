// ACRPGArrowProjectile.h — Ep.#48 (Shoot Arrow), #49 (Arrow Model), #50 (Bow Damage)
//
// O'q — Projectile Movement bilan uchadigan oddiy aktyor.
// Ikkita nozik nuqta bor:
//   1) O'zini otgan personajga tegmasligi kerak (IgnoreActor).
//   2) Tekkanda "yopishib" qolishi kerak — bu vizual jihatdan juda muhim.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACRPGArrowProjectile.generated.h"

class UProjectileMovementComponent;
class UStaticMeshComponent;
class UCapsuleComponent;
class AACRPGCharacterBase;
class USoundBase;

UCLASS()
class ACRPG_API AACRPGArrowProjectile : public AActor
{
	GENERATED_BODY()

public:
	AACRPGArrowProjectile();

	/** EquipmentComponent otish paytida chaqiradi. */
	UFUNCTION(BlueprintCallable, Category = "O'q")
	void InitializeArrow(const FVector& InVelocity, float InDamage, AACRPGCharacterBase* InShooter);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnArrowHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UCapsuleComponent> CollisionCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UStaticMeshComponent> ArrowMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** Ep.#50 — masofa oshgani sayin urish kamayadimi. */
	UPROPERTY(EditDefaultsOnly, Category = "O'q")
	bool bUseDistanceFalloff = false;

	UPROPERTY(EditDefaultsOnly, Category = "O'q", meta = (ClampMin = "100", EditCondition = "bUseDistanceFalloff"))
	float FullDamageRange = 2000.f;

	/** Boshga tekkanda ko'paytiruvchi. */
	UPROPERTY(EditDefaultsOnly, Category = "O'q", meta = (ClampMin = "1"))
	float HeadshotMultiplier = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "O'q")
	FName HeadBoneName = TEXT("head");

	UPROPERTY(EditDefaultsOnly, Category = "O'q")
	TObjectPtr<USoundBase> ImpactSound;

	/** Tekkanidan keyin necha soniya turadi. */
	UPROPERTY(EditDefaultsOnly, Category = "O'q", meta = (ClampMin = "0"))
	float StuckLifeSpan = 15.f;

private:
	UPROPERTY(Transient) TObjectPtr<AACRPGCharacterBase> Shooter;

	float Damage = 0.f;
	FVector SpawnLocation = FVector::ZeroVector;
	bool bHasHit = false;
};
