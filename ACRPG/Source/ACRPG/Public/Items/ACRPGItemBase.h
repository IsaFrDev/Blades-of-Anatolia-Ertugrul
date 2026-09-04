// ACRPGItemBase.h — Ep.#19 (qurol/qalqon aktyorlari), #42 (xazina)
//
// Dunyoda ko'rinadigan har qanday buyum: qilich, qalqon, kamon, sandiq ichidagi narsa.
// Ikki holatda bo'ladi:
//   - Dunyoda yotgan (collision yoqilgan, olish mumkin)
//   - Kiyilgan (collision o'chgan, personajga ulangan)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/ACRPGTypes.h"
#include "ACRPGItemBase.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class AACRPGCharacterBase;

UCLASS()
class ACRPG_API AACRPGItemBase : public AActor
{
	GENERATED_BODY()

public:
	AACRPGItemBase();

	/** Data Table'dagi qator nomi — bu aktyor qaysi buyumni ifodalaydi. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buyum")
	FName ItemID = NAME_None;

	UFUNCTION(BlueprintPure, Category = "Buyum")
	UStaticMeshComponent* GetItemMesh() const { return ItemMesh; }

	/** Kiyilganda chaqiriladi — collision o'chadi. */
	UFUNCTION(BlueprintCallable, Category = "Buyum")
	virtual void OnEquipped(AACRPGCharacterBase* NewOwner);

	/** Yechilganda. */
	UFUNCTION(BlueprintCallable, Category = "Buyum")
	virtual void OnUnequipped();

	/** Ep.#42 — o'yinchi yaqinlashib "E" bosganda. */
	UFUNCTION(BlueprintCallable, Category = "Buyum")
	virtual void Interact(AACRPGCharacterBase* Interactor);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	/** Dunyoda yotganda "olish mumkin" zonasi. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<USphereComponent> PickupSphere;

	/** Ep.#42 — olingandan keyin aktyor o'chiriladimi. */
	UPROPERTY(EditDefaultsOnly, Category = "Buyum")
	bool bDestroyOnPickup = true;

	/** Nechta beriladi (o'q uchun 10 dona). */
	UPROPERTY(EditAnywhere, Category = "Buyum", meta = (ClampMin = "1"))
	int32 PickupQuantity = 1;

	bool bIsEquipped = false;
};
