// ACRPGEnemyController.h — Ep.#22 (AI Melee), #23 (Combo Attacks), #28 (Boss AI)
//
// Dushman AI si bazadan meros oladi va ustiga jang mantiqini qo'shadi.
// Boss ham shu klass — faqat sozlamalari boshqacha (ko'proq kombo, ko'proq masofa).

#pragma once

#include "CoreMinimal.h"
#include "AI/ACRPGAIControllerBase.h"
#include "ACRPGEnemyController.generated.h"

UCLASS()
class ACRPG_API AACRPGEnemyController : public AACRPGAIControllerBase
{
	GENERATED_BODY()

public:
	AACRPGEnemyController();

	/** BT Task shu masofani so'raydi. */
	UFUNCTION(BlueprintPure, Category = "ACRPG|AI")
	float GetAttackRange() const { return AttackRange; }

	/**
	 * Ep.#23 — bu safar nechta zarbali kombo qilsin?
	 * Har hujumda tasodifiy tanlanadi — jang bir xil bo'lib qolmasin.
	 */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|AI")
	int32 RollComboLength() const;

	/** Ep.#22 — hujumlar orasidagi tanaffus (AI ni "adolatli" qiladi). */
	UFUNCTION(BlueprintPure, Category = "ACRPG|AI")
	float GetAttackCooldown() const;

	virtual void OnPossess(APawn* InPawn) override;

protected:
	virtual bool IsHostileTo(const AActor* Other) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Jang", meta = (ClampMin = "50"))
	float AttackRange = 180.f;

	UPROPERTY(EditDefaultsOnly, Category = "Jang", meta = (ClampMin = "1"))
	int32 MinComboLength = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Jang", meta = (ClampMin = "1"))
	int32 MaxComboLength = 3;

	/** Tasodifiy tanaffus — barcha dushmanlar bir vaqtda hujum qilmasin. */
	UPROPERTY(EditDefaultsOnly, Category = "Jang", meta = (ClampMin = "0"))
	float MinAttackCooldown = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Jang", meta = (ClampMin = "0"))
	float MaxAttackCooldown = 3.f;

	/** Ep.#28 — boss bo'lsa, sog'lig'i kamayganda tezlashadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	bool bIsBoss = false;

	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bIsBoss"))
	float EnrageHealthPercent = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (ClampMin = "1", EditCondition = "bIsBoss"))
	float EnrageSpeedMultiplier = 1.4f;

private:
	UFUNCTION()
	void OnHealthChangedCheckEnrage(float NewHealth, float MaxHealth);

	bool bEnraged = false;
};
