// ACRPGEnemyCharacter.h — Ep.#21 (sezilishi), #41 (kvest), #65 (XP mukofoti)
//
// MUHIM NOZIKLIK (Ep.#21 da eng ko'p vaqt olgan xato):
// AI Perception faqat "Stimuli Source" komponentiga ega aktyorlarni ko'radi.
// O'yinchi personajida bu komponent bo'lmasa, dushmanlar uni HECH QACHON ko'rmaydi
// va hech qanday xato ham chiqmaydi — shunchaki ishlamaydi.
//
// Shuning uchun uni C++ da qo'shamiz: unutib bo'lmaydi.

#pragma once

#include "CoreMinimal.h"
#include "Character/ACRPGCharacterBase.h"
#include "ACRPGEnemyCharacter.generated.h"

class UAIPerceptionStimuliSourceComponent;
class UWidgetComponent;

UCLASS()
class ACRPG_API AACRPGEnemyCharacter : public AACRPGCharacterBase
{
	GENERATED_BODY()

public:
	AACRPGEnemyCharacter();

	virtual void HandleDeath(AActor* Killer) override;

	UFUNCTION(BlueprintPure, Category = "ACRPG|Dushman")
	int32 GetXPReward() const { return XPReward; }

protected:
	virtual void BeginPlay() override;
	virtual void OnCombatStateChanged(ECombatState OldState, ECombatState NewState) override;

	/** Ep.#21 — AI shu komponent tufayli bir-birini "ko'radi". */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSource;

	/** Boshi ustidagi sog'liq chizig'i (ixtiyoriy). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UWidgetComponent> HealthBarWidget;

	/** Ep.#65 — o'ldirilganda o'yinchiga beriladigan tajriba. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mukofot", meta = (ClampMin = "0"))
	int32 XPReward = 50;

	/** Ep.#42 — o'lganda tushadigan buyumlar (ItemID -> ehtimollik 0..1). */
	UPROPERTY(EditDefaultsOnly, Category = "Mukofot")
	TMap<FName, float> LootTable;

	/** Sog'liq chizig'i faqat zarba yegandan keyin ko'rinadi. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	bool bHideHealthBarUntilDamaged = true;

private:
	UFUNCTION()
	void OnHealthChanged_ShowBar(float NewHealth, float MaxHealth);

	void DropLoot();
};
