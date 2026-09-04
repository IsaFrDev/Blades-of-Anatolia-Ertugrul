// ACRPGAnimalController.h — Ep.#57 (Animal AI), #58 (Animal Attack), #59 (Animal Sounds)
//
// Hayvonlar dushmanlardan uch jihati bilan farq qiladi:
//   1) Xatti-harakat turi (EAnimalBehavior): beozor / yirtqich / minsa bo'ladigan
//   2) Beozor hayvon o'yinchini ko'rsa QOCHADI, hujum qilmaydi
//   3) Vaqti-vaqti bilan tovush chiqaradi (Ep.#59)
//
// Ep.#63 — minsa bo'ladigan hayvon minilganda AI butunlay o'chiriladi.

#pragma once

#include "CoreMinimal.h"
#include "AI/ACRPGAIControllerBase.h"
#include "ACRPGAnimalController.generated.h"

class USoundBase;

UCLASS()
class ACRPG_API AACRPGAnimalController : public AACRPGAIControllerBase
{
	GENERATED_BODY()

public:
	AACRPGAnimalController();

	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION(BlueprintPure, Category = "ACRPG|Hayvon")
	EAnimalBehavior GetBehavior() const { return Behavior; }

	/** Ep.#63 — o'yinchi mindi: AI to'xtaydi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Hayvon")
	void SetMounted(bool bMounted);

protected:
	virtual void HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus) override;
	virtual bool IsHostileTo(const AActor* Other) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Hayvon")
	EAnimalBehavior Behavior = EAnimalBehavior::Passive;

	/** Ep.#58 — yirtqich shu masofadan yaqinlashsa hujum qiladi. */
	UPROPERTY(EditDefaultsOnly, Category = "Hayvon", meta = (ClampMin = "50"))
	float AttackRange = 150.f;

	// --- Ep.#59: tovushlar ---

	UPROPERTY(EditDefaultsOnly, Category = "Tovush")
	TArray<TObjectPtr<USoundBase>> IdleSounds;

	UPROPERTY(EditDefaultsOnly, Category = "Tovush")
	TObjectPtr<USoundBase> AlertSound;

	/** Tovushlar orasidagi tasodifiy tanaffus. */
	UPROPERTY(EditDefaultsOnly, Category = "Tovush", meta = (ClampMin = "1"))
	float MinIdleSoundInterval = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Tovush", meta = (ClampMin = "1"))
	float MaxIdleSoundInterval = 18.f;

private:
	void PlayIdleSound();
	void ScheduleNextIdleSound();

	FTimerHandle IdleSoundTimer;
	bool bIsMounted = false;
};
