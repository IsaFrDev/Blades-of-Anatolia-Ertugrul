// ACRPGGameMode.h — Ep.#68 (O'lim va qayta tug'ilish), Ep.#64 (menyular).
//
// Blueprint'da respawn odatda personaj ichida "Set Timer -> Open Level" bilan qilinardi.
// Bu yomon amaliyot: personaj o'zini o'zi qayta tug'dira olmaydi (u o'lgan).
// To'g'ri joy — GameMode: u o'yin qoidalarining egasi.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ACRPGGameMode.generated.h"

class AACRPGPlayerCharacter;

UCLASS()
class ACRPG_API AACRPGGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AACRPGGameMode();

	/**
	 * Ep.#68 — personaj o'lganda StatsComponent shu funksiyani chaqiradi.
	 * Ekranni qoraytiradi, kutadi, keyin oxirgi checkpoint'da tiriltiradi.
	 */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Respawn")
	void HandlePlayerDeath(AController* DeadController);

	/** Ep.#68 — checkpoint (masalan, qarorgohga kirganda yangilanadi). */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Respawn")
	void SetRespawnTransform(const FTransform& NewTransform) { RespawnTransform = NewTransform; }

protected:
	virtual void BeginPlay() override;

	/** O'lgandan keyin necha soniya kutiladi (o'lim animatsiyasi uzunligi). */
	UPROPERTY(EditDefaultsOnly, Category = "Respawn", meta = (ClampMin = "0.1"))
	float RespawnDelay = 3.f;

	UPROPERTY(BlueprintReadOnly, Category = "Respawn")
	FTransform RespawnTransform;

private:
	void RespawnPlayer(AController* Controller);

	FTimerHandle RespawnTimerHandle;
};
