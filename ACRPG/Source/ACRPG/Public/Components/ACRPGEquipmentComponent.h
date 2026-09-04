// ACRPGEquipmentComponent.h
//
// Ep.#14 (tizim), #15 (menyu), #16-18 (slotlar), #19 (qalqon va kamon),
// #44-45 (zirh), #46-51 (kamon va o'qlar), #73 (ikonkalar), #79 (personaj mesh'i)
//
// ASOSIY G'OYA: slot -> buyum ID -> dunyodagi aktyor.
//
// Har bir slot uchun uchta narsani sinxron ushlab turamiz:
//   1) EquippedItems[Slot]   — qaysi buyum kiyilgan (FName)
//   2) SpawnedActors[Slot]   — dunyoda ko'rinadigan aktyor (qilich, qalqon)
//   3) ArmorMeshes[Slot]     — zirh uchun qo'shimcha skeletal mesh
//
// Har o'zgarishda statistikani qayta hisoblaymiz (RecalculateStats) — shunda
// zirh qiymati hech qachon "adashib qolmaydi" (Ep.#82 dagi buglardan biri).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ACRPGTypes.h"
#include "ACRPGEquipmentComponent.generated.h"

class AACRPGCharacterBase;
class AACRPGItemBase;
class AACRPGWeaponBase;
class AACRPGArrowProjectile;
class USkeletalMeshComponent;
class UUserWidget;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChanged, EEquipmentSlot, Slot, FName, ItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoChanged, int32, NewAmmo);

UCLASS(ClassGroup = (ACRPG), meta = (BlueprintSpawnableComponent))
class ACRPG_API UACRPGEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACRPGEquipmentComponent();

	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event") FOnEquipmentChanged OnEquipmentChanged;
	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event") FOnAmmoChanged OnAmmoChanged;

	// -----------------------------------------------------------------------
	// KIYISH / YECHISH (Ep.#14, #16-19, #44-45)
	// -----------------------------------------------------------------------

	/**
	 * Buyumni o'z slotiga kiydiradi (slot Data Table'dan olinadi — Ep.#17).
	 * @return false — buyum topilmadi yoki slotga mos emas.
	 */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Ekipirovka")
	bool EquipItem(FName ItemID);

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Ekipirovka")
	bool UnequipSlot(EEquipmentSlot Slot);

	UFUNCTION(BlueprintPure, Category = "ACRPG|Ekipirovka")
	FName GetEquippedItem(EEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "ACRPG|Ekipirovka")
	bool IsSlotOccupied(EEquipmentSlot Slot) const;

	/** Ep.#10 — CombatComponent trace uchun ishlatadi. */
	UFUNCTION(BlueprintPure, Category = "ACRPG|Ekipirovka")
	AACRPGWeaponBase* GetEquippedWeapon() const;

	UFUNCTION(BlueprintPure, Category = "ACRPG|Ekipirovka")
	float GetWeaponDamage() const;

	// -----------------------------------------------------------------------
	// QUROLNI CHIQARISH/YASHIRISH (Ep.#19)
	// -----------------------------------------------------------------------

	/** Qurolni beldan qo'lga (yoki teskari). */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Ekipirovka")
	void ToggleWeaponDrawn();

	UFUNCTION(BlueprintPure, Category = "ACRPG|Ekipirovka")
	bool IsWeaponDrawn() const { return bWeaponDrawn; }

	// -----------------------------------------------------------------------
	// KAMON (Ep.#46-51)
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "ACRPG|Kamon")
	bool HasBowEquipped() const { return IsSlotOccupied(EEquipmentSlot::Ranged); }

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Kamon")
	void SetAiming(bool bNewAiming);

	UFUNCTION(BlueprintPure, Category = "ACRPG|Kamon")
	bool IsAiming() const { return bIsAiming; }

	/** Ep.#48, #50, #51 — o'q otadi (o'q-dori bo'lsa). */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Kamon")
	bool FireArrow();

	UFUNCTION(BlueprintPure, Category = "ACRPG|Kamon")
	int32 GetArrowCount() const;

	// -----------------------------------------------------------------------
	// MENYU (Ep.#15, #73)
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "ACRPG|UI")
	void ToggleEquipmentMenu();

	// --- Ep.#80: saqlash ---

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Save")
	void LoadFromSave(const TMap<EEquipmentSlot, FName>& SavedEquipment);

	UFUNCTION(BlueprintPure, Category = "ACRPG|Save")
	const TMap<EEquipmentSlot, FName>& GetEquippedMap() const { return EquippedItems; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// -----------------------------------------------------------------------
	// SOCKET NOMLARI (Ep.#19)
	//
	// Har bir slot uchun ikkita socket: qo'lda (drawn) va belda/orqada (sheathed).
	// Skeletda bu socketlarni oldindan yaratib qo'yasiz.
	// -----------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "Socket")
	TMap<EEquipmentSlot, FName> DrawnSockets;

	UPROPERTY(EditDefaultsOnly, Category = "Socket")
	TMap<EEquipmentSlot, FName> SheathedSockets;

	/** Ep.#48 — o'q shu socketdan uchadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Socket")
	FName ArrowSpawnSocket = TEXT("ArrowSocket");

	// --- Zirh (Ep.#44-45, #79) ---

	/**
	 * Zirh mesh'lari asosiy tanaga "Leader Pose" bilan bog'lanadi —
	 * ya'ni ular tananing skeletiga ergashadi. Bu modulli zirh tizimining kaliti.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Zirh")
	TMap<EEquipmentSlot, FName> ArmorMeshComponentNames;

	// --- Kamon (Ep.#48-51) ---

	UPROPERTY(EditDefaultsOnly, Category = "Kamon")
	TSubclassOf<AACRPGArrowProjectile> ArrowProjectileClass;

	/** Ep.#51 — o'q-dori sifatida ishlatiladigan buyum ID. */
	UPROPERTY(EditDefaultsOnly, Category = "Kamon")
	FName ArrowItemID = TEXT("Arrow");

	UPROPERTY(EditDefaultsOnly, Category = "Kamon", meta = (ClampMin = "100"))
	float ArrowSpeed = 4000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Kamon", meta = (ClampMin = "0"))
	float ArrowBaseDamage = 40.f;

	UPROPERTY(EditDefaultsOnly, Category = "Kamon")
	TObjectPtr<UAnimMontage> ShootMontage;

	/** Otish oralig'i (soniya). */
	UPROPERTY(EditDefaultsOnly, Category = "Kamon", meta = (ClampMin = "0"))
	float FireCooldown = 0.8f;

	// --- Menyu (Ep.#15) ---

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> EquipmentMenuClass;

private:
	/** Buyum aktyorini yaratib, kerakli socketga ulaydi. */
	AACRPGItemBase* SpawnAndAttachItem(const FACRPGItemData& Data, EEquipmentSlot Slot);

	/** Ep.#44 — barcha kiyilgan buyumlar bo'yicha zirh/urishni qayta hisoblaydi. */
	void RecalculateStats();

	/** Ep.#19 — barcha qurollarni to'g'ri socketga ko'chiradi. */
	void UpdateAttachments();

	void DestroySlotActor(EEquipmentSlot Slot);

	UPROPERTY(Transient) TObjectPtr<AACRPGCharacterBase> OwnerCharacter;

	/** Slot -> kiyilgan buyum ID. */
	UPROPERTY(SaveGame)
	TMap<EEquipmentSlot, FName> EquippedItems;

	/** Slot -> dunyodagi aktyor. */
	UPROPERTY(Transient)
	TMap<EEquipmentSlot, TObjectPtr<AACRPGItemBase>> SpawnedActors;

	/** Slot -> zirh mesh komponenti (Ep.#45). */
	UPROPERTY(Transient)
	TMap<EEquipmentSlot, TObjectPtr<USkeletalMeshComponent>> ArmorMeshComponents;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> EquipmentMenuWidget;

	bool bWeaponDrawn = false;
	bool bIsAiming = false;
	float LastFireTime = -100.f;
	float CachedWeaponDamage = 0.f;
};
