// ACRPGInventoryComponent.h — Ep.#14 (Equipment System), #42 (Treasure), #51 (Ammo)
//
// Inventar va ekipirovka — IKKI XIL narsa, shuning uchun ikki komponent:
//   Inventar   = nima BOR (ro'yxat, sonlar)
//   Ekipirovka = nima KIYILGAN (slotlar, mesh'lar, statistika)
//
// Blueprint seriyasida ikkalasi bitta BP_Equipment ichida aralashib ketgan edi va
// Ep.#18 "Slots Data and Category" epizodining yarmi shu chalkashlikni yechishga ketgan.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ACRPGTypes.h"
#include "ACRPGInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS(ClassGroup = (ACRPG), meta = (BlueprintSpawnableComponent))
class ACRPG_API UACRPGInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACRPGInventoryComponent();

	/** UI shu event'ga obuna bo'ladi — har o'zgarishda ro'yxatni qayta chizadi. */
	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event")
	FOnInventoryChanged OnInventoryChanged;

	/**
	 * Buyum qo'shadi. Stack limitini hisobga oladi (Ep.#51 — o'qlar).
	 * @return haqiqatda qo'shilgan miqdor (joy yetmasa kamroq bo'lishi mumkin)
	 */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Inventar")
	int32 AddItem(FName ItemID, int32 Quantity = 1);

	/** @return true — to'liq olib tashlandi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Inventar")
	bool RemoveItem(FName ItemID, int32 Quantity = 1);

	UFUNCTION(BlueprintPure, Category = "ACRPG|Inventar")
	int32 GetItemCount(FName ItemID) const;

	UFUNCTION(BlueprintPure, Category = "ACRPG|Inventar")
	bool HasItem(FName ItemID, int32 MinQuantity = 1) const { return GetItemCount(ItemID) >= MinQuantity; }

	/** Ep.#15 — menyuda ko'rsatish uchun. */
	UFUNCTION(BlueprintPure, Category = "ACRPG|Inventar")
	const TArray<FACRPGInventoryEntry>& GetAllItems() const { return Items; }

	/** Ep.#15 — kategoriya bo'yicha filtr (menyu tablari). */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Inventar")
	TArray<FACRPGInventoryEntry> GetItemsByCategory(EItemCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "ACRPG|Inventar")
	int32 GetMaxSlots() const { return MaxSlots; }

	// --- Ep.#80: saqlash ---

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Save")
	void LoadFromSave(const TArray<FACRPGInventoryEntry>& SavedItems);

protected:
	/** Nechta har xil buyum sig'adi (0 = cheksiz). */
	UPROPERTY(EditDefaultsOnly, Category = "Inventar", meta = (ClampMin = "0"))
	int32 MaxSlots = 40;

	/** O'yin boshida beriladigan buyumlar. */
	UPROPERTY(EditDefaultsOnly, Category = "Inventar")
	TArray<FACRPGInventoryEntry> StartingItems;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Inventar")
	TArray<FACRPGInventoryEntry> Items;

	virtual void BeginPlay() override;
};
