// ACRPGVaultingComponent.h — Ep.#3 "Vaulting with Motion Warping"
//
// G'oya: personaj oldida past to'siq bo'lsa, sakrash o'rniga uning ustidan oshib o'tadi.
//
// Blueprint'da bu bir nechta "Sphere Trace" tugunlari va qo'lda "Set Motion Warp Target"
// chaqiruvlaridan iborat edi. Mantiq bir xil, lekin C++ da uni o'qish ancha oson —
// va eng muhimi, trace natijalarini debug qilish oson.
//
// Uch bosqichli tekshiruv:
//   1) OLDINGA trace  -> to'siq bormi va uning old yuzasi qayerda?
//   2) YUQORIDAN trace -> to'siq usti qanchalik baland? (juda baland bo'lsa — tirmashish)
//   3) NARIGI TOMONGA trace -> qo'nish joyi bo'shmi?
//
// Keyin topilgan ikki nuqtani Motion Warping'ga beramiz. Animatsiya o'zi
// personajni aynan o'sha nuqtalarga olib boradi — qo'lda joylashtirish shart emas.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACRPGVaultingComponent.generated.h"

class AACRPGCharacterBase;
class UAnimMontage;
class UMotionWarpingComponent;

UCLASS(ClassGroup = (ACRPG), meta = (BlueprintSpawnableComponent))
class ACRPG_API UACRPGVaultingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACRPGVaultingComponent();

	/**
	 * Sakrash tugmasi bosilganda chaqiriladi.
	 * @return true — vault boshlandi (sakrash BEKOR qilinsin);
	 *         false — to'siq yo'q, oddiy sakrash bo'lsin.
	 */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Vault")
	bool TryVault();

	UFUNCTION(BlueprintPure, Category = "ACRPG|Vault")
	bool IsVaulting() const { return bIsVaulting; }

protected:
	virtual void BeginPlay() override;

	// --- Sozlamalar ---

	/** To'siqni qidirish uchun oldinga qancha masofa. */
	UPROPERTY(EditDefaultsOnly, Category = "Vault", meta = (ClampMin = "20"))
	float ForwardTraceDistance = 150.f;

	/** Shu balandlikdan past to'siqlardan oshib o'tiladi. Balandroq bo'lsa — tirmashish (Ep.#32). */
	UPROPERTY(EditDefaultsOnly, Category = "Vault", meta = (ClampMin = "20"))
	float MaxVaultHeight = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Vault", meta = (ClampMin = "10"))
	float MinVaultHeight = 40.f;

	/** To'siq shundan qalin bo'lsa, oshib o'tib bo'lmaydi (devor). */
	UPROPERTY(EditDefaultsOnly, Category = "Vault", meta = (ClampMin = "20"))
	float MaxVaultDepth = 200.f;

	/** Trace radiusi — ingichka ustunlarni ham topsin. */
	UPROPERTY(EditDefaultsOnly, Category = "Vault", meta = (ClampMin = "1"))
	float TraceRadius = 20.f;

	/** Ep.#3 — vault animatsiyasi. Ichida ikkita Motion Warping Notify bo'lishi shart. */
	UPROPERTY(EditDefaultsOnly, Category = "Vault")
	TObjectPtr<UAnimMontage> VaultMontage;

	/**
	 * Montajdagi Motion Warping notify nomlari.
	 * Editor'da montajga "Motion Warping" turidagi Notify State qo'yib,
	 * Warp Target Name maydoniga aynan shu nomlarni yozasiz.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Vault")
	FName WarpTargetName_Start = TEXT("VaultStart");

	UPROPERTY(EditDefaultsOnly, Category = "Vault")
	FName WarpTargetName_Land = TEXT("VaultLand");

	/** Debug chiziqlarini ko'rsatish (editor'da sozlashda juda foydali). */
	UPROPERTY(EditAnywhere, Category = "Vault|Debug")
	bool bShowDebugTraces = false;

private:
	/** Vault uchun geometriya topadi. Topilsa true. */
	bool FindVaultTargets(FVector& OutStartPoint, FVector& OutLandPoint) const;

	UFUNCTION()
	void OnVaultMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(Transient)
	TObjectPtr<AACRPGCharacterBase> OwnerCharacter;

	bool bIsVaulting = false;
};
