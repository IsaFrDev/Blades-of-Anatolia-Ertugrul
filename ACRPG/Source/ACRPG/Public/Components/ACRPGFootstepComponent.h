// ACRPGFootstepComponent.h — Ep.#55 "Sand Footsteps"
//
// Qumda, toshda, suvda — har xil tovush va har xil chang effekti.
//
// QANDAY ISHLAYDI:
//   1) Animatsiya montajida "Footstep" nomli Anim Notify bo'ladi
//   2) Notify shu komponentning OnFootstep() ini chaqiradi
//   3) Biz oyoq ostiga trace tashlaymiz va Physical Material ni o'qiymiz
//   4) Sirt turiga mos tovush/effektni ijro etamiz
//
// MUHIM: trace da bReturnPhysicalMaterial = true bo'lishi SHART,
// aks holda Hit.PhysMaterial har doim bo'sh keladi (Ep.#55 dagi klassik xato).
//
// Sozlash: Project Settings > Physics > Physical Surfaces da
// SurfaceType1 = Sand, SurfaceType2 = Stone... deb nomlanadi.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ACRPGTypes.h"
#include "ACRPGFootstepComponent.generated.h"

class ACharacter;

UCLASS(ClassGroup = (ACRPG), meta = (BlueprintSpawnableComponent))
class ACRPG_API UACRPGFootstepComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACRPGFootstepComponent();

	/**
	 * Anim Notify shuni chaqiradi.
	 * @param FootSocket — qaysi oyoq ("foot_l" yoki "foot_r")
	 */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Qadam")
	void OnFootstep(FName FootSocket);

	/** Ep.#24 — yugurganda AI eshitadigan shovqin chiqarsinmi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Qadam")
	void SetMakeNoise(bool bNewMakeNoise) { bMakeNoise = bNewMakeNoise; }

protected:
	virtual void BeginPlay() override;

	/** Sirt turi -> tovush + effekt jadvali. */
	UPROPERTY(EditDefaultsOnly, Category = "Qadam")
	TArray<FACRPGFootstepEntry> FootstepEntries;

	/** Jadvalda topilmasa ishlatiladi. */
	UPROPERTY(EditDefaultsOnly, Category = "Qadam")
	TObjectPtr<USoundBase> DefaultFootstepSound;

	UPROPERTY(EditDefaultsOnly, Category = "Qadam", meta = (ClampMin = "10"))
	float TraceDistance = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Qadam", meta = (ClampMin = "0", ClampMax = "2"))
	float VolumeMultiplier = 1.f;

	/** Cho'kkalab yurganda tovush pasayadi (yashirincha o'ynash). */
	UPROPERTY(EditDefaultsOnly, Category = "Qadam", meta = (ClampMin = "0", ClampMax = "1"))
	float CrouchVolumeMultiplier = 0.3f;

	// --- Ep.#24: AI eshitishi ---

	UPROPERTY(EditDefaultsOnly, Category = "Shovqin")
	bool bMakeNoise = true;

	/** Yugurganda shovqin kuchi. */
	UPROPERTY(EditDefaultsOnly, Category = "Shovqin", meta = (ClampMin = "0", ClampMax = "2"))
	float SprintNoiseLoudness = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shovqin", meta = (ClampMin = "0", ClampMax = "2"))
	float WalkNoiseLoudness = 0.3f;

	/** Cho'kkalab yurganda shovqin umuman chiqmaydi. */
	UPROPERTY(EditDefaultsOnly, Category = "Shovqin")
	bool bSilentWhenCrouched = true;

private:
	const FACRPGFootstepEntry* FindEntryForSurface(EPhysicalSurface Surface) const;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;
};
