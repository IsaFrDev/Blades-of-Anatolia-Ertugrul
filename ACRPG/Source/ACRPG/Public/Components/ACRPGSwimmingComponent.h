// ACRPGSwimmingComponent.h — Ep.#56 "Swimming" (Oasis ko'lida suzish)
//
// UE da suzish uchun tayyor MOVE_Swimming rejimi bor va u Physics Volume orqali
// avtomatik yoqiladi. Lekin ikkita narsa qo'lda qilinadi:
//   1) Suv YUZASIDA qolish (aks holda personaj tubiga cho'kadi)
//   2) Suvga kirish/chiqish o'tishlari va animatsiya holati
//
// Ep.#53 da yasalgan Water Body Lake aktyori o'zining Physics Volume ini yaratadi,
// shuning uchun qo'shimcha volume qo'yish shart emas.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACRPGSwimmingComponent.generated.h"

class AACRPGCharacterBase;
class UCharacterMovementComponent;

UCLASS(ClassGroup = (ACRPG), meta = (BlueprintSpawnableComponent))
class ACRPG_API UACRPGSwimmingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACRPGSwimmingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "ACRPG|Suzish")
	bool IsSwimming() const { return bIsSwimming; }

	/** Suv yuzasining Z koordinatasi (suv aktyoridan olinadi). */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Suzish")
	void SetWaterSurfaceZ(float NewZ) { WaterSurfaceZ = NewZ; bHasWaterSurface = true; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Suzish", meta = (ClampMin = "10"))
	float SwimSpeed = 250.f;

	/** Personaj markazi suv yuzasidan qancha pastda turadi (ko'krakkacha suv). */
	UPROPERTY(EditDefaultsOnly, Category = "Suzish")
	float SurfaceOffset = -30.f;

	/** Yuzaga chiqish kuchi. */
	UPROPERTY(EditDefaultsOnly, Category = "Suzish", meta = (ClampMin = "1"))
	float BuoyancyInterpSpeed = 4.f;

	/** Ep.#6 — suzish ham chidamlilik yeydi. */
	UPROPERTY(EditDefaultsOnly, Category = "Suzish", meta = (ClampMin = "0"))
	float SwimStaminaDrain = 3.f;

private:
	UFUNCTION()
	void OnMovementModeChanged(class ACharacter* Character, EMovementMode PrevMode, uint8 PrevCustomMode);

	void EnterWater();
	void ExitWater();

	UPROPERTY(Transient) TObjectPtr<AACRPGCharacterBase> OwnerCharacter;
	UPROPERTY(Transient) TObjectPtr<UCharacterMovementComponent> MovementComponent;

	float WaterSurfaceZ = 0.f;
	bool bHasWaterSurface = false;
	bool bIsSwimming = false;
	float CachedWalkSpeed = 500.f;
};
