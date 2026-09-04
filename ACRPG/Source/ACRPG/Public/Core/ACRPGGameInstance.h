// ACRPGGameInstance.h
//
// Blueprint seriyasida buyum va kvest ma'lumotlari har bir Blueprint ichida
// alohida "Get Data Table Row" bilan o'qilardi. Bu xato manbasi: bitta Data Table'ni
// o'nta joyda qattiq bog'lab qo'yasiz.
//
// C++ da barcha Data Table'lar shu yerda — bitta markazda — turadi va
// har bir tizim GameInstance orqali so'raydi:
//     UACRPGGameInstance::Get(this)->FindItem("IronSword")
//
// Qamrab olingan epizodlar: #14 (item data), #34-35 (quest data), #80-81 (save slot nomi).

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Core/ACRPGTypes.h"
#include "ACRPGGameInstance.generated.h"

class UDataTable;

UCLASS()
class ACRPG_API UACRPGGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UACRPGGameInstance();

	/** Istalgan joydan qulay kirish. WorldContext — odatda `this`. */
	UFUNCTION(BlueprintPure, Category = "ACRPG", meta = (WorldContext = "WorldContextObject"))
	static UACRPGGameInstance* Get(const UObject* WorldContextObject);

	// --- Data Table'lar (editor'da tayinlanadi) ---

	/** Ep.#14, #18, #44 — DT_Items */
	UPROPERTY(EditDefaultsOnly, Category = "Ma'lumot")
	TObjectPtr<UDataTable> ItemDataTable;

	/** Ep.#34-35 — DT_Quests */
	UPROPERTY(EditDefaultsOnly, Category = "Ma'lumot")
	TObjectPtr<UDataTable> QuestDataTable;

	/**
	 * Buyum ma'lumotini ID bo'yicha topadi.
	 * @return topilmasa nullptr — chaqiruvchi tomon ALBATTA tekshirsin.
	 */
	const FACRPGItemData* FindItem(FName ItemID) const;

	/** Blueprint'dan chaqirish uchun (pointer emas, nusxa qaytaradi). */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Buyum")
	bool GetItemData(FName ItemID, FACRPGItemData& OutItem) const;

	const FACRPGQuestData* FindQuest(FName QuestID) const;

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Kvest")
	bool GetQuestData(FName QuestID, FACRPGQuestData& OutQuest) const;

	// --- Saqlash (Ep.#80-81) ---

	UPROPERTY(EditDefaultsOnly, Category = "Saqlash")
	FString SaveSlotName = TEXT("ACRPG_Slot0");

	UPROPERTY(EditDefaultsOnly, Category = "Saqlash")
	int32 SaveUserIndex = 0;
};
