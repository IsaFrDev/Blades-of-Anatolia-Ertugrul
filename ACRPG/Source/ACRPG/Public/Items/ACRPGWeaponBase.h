// ACRPGWeaponBase.h — Ep.#10, #19
//
// Qurol = buyum + trace socketlari + effektlar.
// CombatComponent shu aktyordan TraceStart/TraceEnd socketlarini o'qiydi.

#pragma once

#include "CoreMinimal.h"
#include "Items/ACRPGItemBase.h"
#include "ACRPGWeaponBase.generated.h"

class USoundBase;
class UNiagaraSystem;

UCLASS()
class ACRPG_API AACRPGWeaponBase : public AACRPGItemBase
{
	GENERATED_BODY()

public:
	AACRPGWeaponBase();

	/** CombatComponent trace uchun ishlatadi. */
	UFUNCTION(BlueprintPure, Category = "Qurol")
	USceneComponent* GetWeaponMesh() const;

	UFUNCTION(BlueprintPure, Category = "Qurol")
	float GetWeaponDamage() const { return WeaponDamage; }

	/** Ep.#10 — zarba tekkanda tovush va uchqun. */
	UFUNCTION(BlueprintCallable, Category = "Qurol")
	void PlayImpactEffects(const FVector& Location, const FVector& Normal);

protected:
	/** Data Table'dagi qiymatni bekor qiladi (0 bo'lsa DT dan olinadi). */
	UPROPERTY(EditDefaultsOnly, Category = "Qurol", meta = (ClampMin = "0"))
	float WeaponDamage = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Qurol")
	TObjectPtr<USoundBase> SwingSound;

	UPROPERTY(EditDefaultsOnly, Category = "Qurol")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditDefaultsOnly, Category = "Qurol")
	TObjectPtr<UNiagaraSystem> ImpactEffect;
};
