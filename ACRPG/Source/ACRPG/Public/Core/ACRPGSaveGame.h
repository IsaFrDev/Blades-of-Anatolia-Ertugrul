// ACRPGSaveGame.h — Ep.#80 (Save and Load), #81 (Save Quests and Time of Day)
//
// SAQLASH FALSAFASI: obyektlarni emas, MA'LUMOTNI saqlaymiz.
//
// Blueprint seriyasida bir necha marta "actor reference" ni saqlashga urinilgan
// va bu ishlamagan — chunki keyingi sessiyada u aktyor boshqa obyekt bo'ladi.
// To'g'ri yechim: ID (FName), pozitsiya (FVector), son (int32) saqlanadi;
// yuklashda ular bo'yicha obyektlar qayta tiklanadi.
//
// SaveGame olib borilishi kerak bo'lgan narsalar:
//   - O'yinchi: pozitsiya, sog'liq, chidamlilik, daraja, XP
//   - Inventar va ekipirovka
//   - Kvest jurnali (Ep.#81)
//   - Kun vaqti (Ep.#81)
//   - O'ldirilgan dushmanlar / ochilgan sandiqlar (qayta tug'ilmasin)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/ACRPGTypes.h"
#include "ACRPGSaveGame.generated.h"

UCLASS()
class ACRPG_API UACRPGSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// --- Meta ---

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString SaveSlotName = TEXT("ACRPG_Slot0");

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FDateTime SaveDate;

	/** Qaysi xaritada saqlangan. */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FName LevelName = NAME_None;

	// --- O'yinchi (Ep.#80) ---

	UPROPERTY(BlueprintReadWrite, Category = "Save|O'yinchi")
	FVector PlayerLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Save|O'yinchi")
	FRotator PlayerRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadWrite, Category = "Save|O'yinchi")
	float Health = 100.f;

	UPROPERTY(BlueprintReadWrite, Category = "Save|O'yinchi")
	float Stamina = 100.f;

	UPROPERTY(BlueprintReadWrite, Category = "Save|O'yinchi")
	int32 Level = 1;

	UPROPERTY(BlueprintReadWrite, Category = "Save|O'yinchi")
	int32 CurrentXP = 0;

	// --- Inventar va ekipirovka (Ep.#80) ---

	UPROPERTY(BlueprintReadWrite, Category = "Save|Inventar")
	TArray<FACRPGInventoryEntry> InventoryItems;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Inventar")
	TMap<EEquipmentSlot, FName> EquippedItems;

	// --- Kvestlar (Ep.#81) ---

	UPROPERTY(BlueprintReadWrite, Category = "Save|Kvest")
	TArray<FACRPGQuestProgress> QuestLog;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Kvest")
	FName ActiveQuestID = NAME_None;

	// --- Dunyo (Ep.#81) ---

	UPROPERTY(BlueprintReadWrite, Category = "Save|Dunyo")
	float TimeOfDay = 12.f;

	/**
	 * O'ldirilgan noyob dushmanlar / ochilgan sandiqlar.
	 * Aktyorning xaritadagi nomi (GetFName) ishlatiladi — u sessiyalar aro barqaror.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Save|Dunyo")
	TSet<FName> DestroyedActorNames;

	/** Ep.#68 — oxirgi checkpoint. */
	UPROPERTY(BlueprintReadWrite, Category = "Save|Dunyo")
	FTransform RespawnTransform;
};
