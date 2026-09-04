// ACRPGAreaTrigger.h — Ep.#76 "Entering Map Area Message"
//
// Xaritadagi ko'rinmas quti. O'yinchi kirganda ekranda hudud nomi chiqadi
// ("Siwa Vohasi") va kvest tizimiga "shu joyga yetib bordi" deb xabar beriladi.
//
// Ep.#66 (Market Towns) va #62 (Enemy Fortress) da ham shu aktyor ishlatiladi.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACRPGAreaTrigger.generated.h"

class UBoxComponent;
class USoundBase;

UCLASS()
class ACRPG_API AACRPGAreaTrigger : public AActor
{
	GENERATED_BODY()

public:
	AACRPGAreaTrigger();

	UFUNCTION(BlueprintPure, Category = "Hudud")
	FText GetAreaName() const { return AreaName; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UBoxComponent> TriggerBox;

	/** Ekranda chiqadigan nom. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hudud")
	FText AreaName;

	/** Ep.#37 — kvestdagi "Reach" maqsadi shu tegni qidiradi. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hudud")
	FName AreaTag = NAME_None;

	/** Faqat bir marta ko'rsatiladimi? */
	UPROPERTY(EditAnywhere, Category = "Hudud")
	bool bTriggerOnce = true;

	/** Qayta ko'rsatishdan oldin necha soniya kutiladi (bTriggerOnce=false bo'lsa). */
	UPROPERTY(EditAnywhere, Category = "Hudud", meta = (ClampMin = "0", EditCondition = "!bTriggerOnce"))
	float RetriggerCooldown = 60.f;

	UPROPERTY(EditAnywhere, Category = "Hudud")
	TObjectPtr<USoundBase> EnterSound;

	/** Ep.#62 — dushman hududi bo'lsa, HUD da ogohlantirish rangi. */
	UPROPERTY(EditAnywhere, Category = "Hudud")
	bool bIsHostileArea = false;

private:
	bool bHasTriggered = false;
	float LastTriggerTime = -1000.f;
};
