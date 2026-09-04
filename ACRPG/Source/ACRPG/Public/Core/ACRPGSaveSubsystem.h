// ACRPGSaveSubsystem.h — Ep.#80-81
//
// Nega Subsystem?
// GameInstanceSubsystem — o'yin bilan birga yashaydigan, xarita almashsa ham
// yo'qolmaydigan obyekt. Saqlash/yuklash uchun aynan shu kerak.
//
// Blueprint'da bu mantiq Pause Menu ichida edi — ya'ni menyu yopilsa
// saqlash ham "yo'qolardi". Subsystem bu muammoni butunlay hal qiladi.
//
// Chaqirish: UACRPGSaveSubsystem::Get(this)->SaveGame();

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ACRPGSaveSubsystem.generated.h"

class UACRPGSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameSaved, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameLoaded, bool, bSuccess);

UCLASS()
class ACRPG_API UACRPGSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "ACRPG|Save", meta = (WorldContext = "WorldContextObject"))
	static UACRPGSaveSubsystem* Get(const UObject* WorldContextObject);

	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event") FOnGameSaved OnGameSaved;
	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event") FOnGameLoaded OnGameLoaded;

	/** Ep.#80 — hozirgi holatni diskka yozadi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Save")
	bool SaveGame(const FString& SlotName = TEXT(""));

	/** Ep.#80 — diskdan o'qib, o'yinga qo'llaydi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Save")
	bool LoadGame(const FString& SlotName = TEXT(""));

	UFUNCTION(BlueprintPure, Category = "ACRPG|Save")
	bool DoesSaveExist(const FString& SlotName = TEXT("")) const;

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Save")
	bool DeleteSave(const FString& SlotName = TEXT(""));

	/**
	 * Ep.#81 — o'ldirilgan noyob aktyorni ro'yxatga qo'shadi,
	 * shunda yuklashda u qayta paydo bo'lmaydi.
	 */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Save")
	void MarkActorDestroyed(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "ACRPG|Save")
	bool IsActorMarkedDestroyed(FName ActorName) const { return DestroyedActors.Contains(ActorName); }

private:
	FString ResolveSlotName(const FString& SlotName) const;

	/** Sessiya davomida yig'iladigan ro'yxat. */
	UPROPERTY()
	TSet<FName> DestroyedActors;
};
