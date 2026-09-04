// ACRPGRidingComponent.h — Ep.#63 "Animal Riding System" (tuya/otga minish)
//
// IKKI YONDASHUV BOR:
//   A) O'yinchi hayvonni Possess qiladi (boshqaruv butunlay o'tadi)
//   B) O'yinchi hayvonga ULANADI va uni masofadan boshqaradi
//
// Biz B ni tanlaymiz, chunki:
//   - O'yinchi personaji ko'rinib turadi (egarda o'tiradi)
//   - Kamera, inventar, kvest — hammasi o'yinchida qoladi
//   - Tushish oson (shunchaki detach)
//
// Blueprint seriyasida ham shu yondashuv ishlatilgan.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACRPGRidingComponent.generated.h"

class AACRPGCharacterBase;
class UAnimMontage;

UCLASS(ClassGroup = (ACRPG), meta = (BlueprintSpawnableComponent))
class ACRPG_API UACRPGRidingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACRPGRidingComponent();

	/** Oldindagi minsa bo'ladigan hayvonni topib minadi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Minish")
	bool TryMount();

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Minish")
	void Dismount();

	UFUNCTION(BlueprintPure, Category = "ACRPG|Minish")
	bool IsRiding() const { return MountedAnimal != nullptr; }

	UFUNCTION(BlueprintPure, Category = "ACRPG|Minish")
	AACRPGCharacterBase* GetMountedAnimal() const { return MountedAnimal; }

	/** O'yinchi kirishini hayvonga uzatadi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Minish")
	void AddRideInput(const FVector2D& Input);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Minish", meta = (ClampMin = "50"))
	float MountRange = 250.f;

	/** Hayvon skeletidagi egar socketi. */
	UPROPERTY(EditDefaultsOnly, Category = "Minish")
	FName SaddleSocket = TEXT("SaddleSocket");

	UPROPERTY(EditDefaultsOnly, Category = "Minish")
	TObjectPtr<UAnimMontage> MountMontage;

	/** Minilgan hayvon shu tezlikda yuradi (odatda odamdan tezroq). */
	UPROPERTY(EditDefaultsOnly, Category = "Minish", meta = (ClampMin = "100"))
	float MountedSpeed = 900.f;

	/** Tushganda yon tomonga qancha siljiydi. */
	UPROPERTY(EditDefaultsOnly, Category = "Minish", meta = (ClampMin = "50"))
	float DismountOffset = 120.f;

private:
	AACRPGCharacterBase* FindMountableAnimal() const;

	UPROPERTY(Transient) TObjectPtr<AACRPGCharacterBase> OwnerCharacter;
	UPROPERTY(Transient) TObjectPtr<AACRPGCharacterBase> MountedAnimal;
};
