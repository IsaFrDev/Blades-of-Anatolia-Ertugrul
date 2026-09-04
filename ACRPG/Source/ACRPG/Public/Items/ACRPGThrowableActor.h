// ACRPGThrowableActor.h — Ep.#27 "Throw Object AI Distraction"
//
// Otilgan tosh yerga tushganda SHOVQIN chiqaradi. AI ning Hearing sezgisi (Ep.#24)
// shu shovqinni eshitib, o'sha joyni tekshirgani boradi.
//
// Kalit funksiya: UAISense_Hearing::ReportNoiseEvent().
// Blueprint'da bu "Report Noise Event" tuguni edi.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACRPGThrowableActor.generated.h"

class UStaticMeshComponent;
class UProjectileMovementComponent;
class USoundBase;

UCLASS()
class ACRPG_API AACRPGThrowableActor : public AActor
{
	GENERATED_BODY()

public:
	AACRPGThrowableActor();

	UFUNCTION(BlueprintCallable, Category = "Chalg'itish")
	void Launch(const FVector& InVelocity);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnImpact(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/**
	 * Shovqin kuchi. AI Perception'dagi Hearing Range shu qiymatga ko'paytiriladi:
	 * 1.0 = to'liq radius, 0.5 = yarmi.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Chalg'itish", meta = (ClampMin = "0.1", ClampMax = "5"))
	float NoiseLoudness = 1.f;

	/** Shovqin "tegi" — AI qaysi turdagi shovqinni eshitgani muhim bo'lsa. */
	UPROPERTY(EditDefaultsOnly, Category = "Chalg'itish")
	FName NoiseTag = TEXT("Distraction");

	UPROPERTY(EditDefaultsOnly, Category = "Chalg'itish")
	TObjectPtr<USoundBase> ImpactSound;

	/** Tushgandan keyin necha soniyada yo'qoladi. */
	UPROPERTY(EditDefaultsOnly, Category = "Chalg'itish", meta = (ClampMin = "1"))
	float LifeAfterImpact = 8.f;

private:
	bool bHasImpacted = false;
};
