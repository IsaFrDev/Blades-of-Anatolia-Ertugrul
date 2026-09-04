// ACRPGAnimInstance.h — Anim Blueprint uchun "ma'lumot uzatuvchi".
//
// Blueprint seriyasida Anim BP ning Event Graph'ida har kadr "Cast to BP_Player"
// qilinardi. Bu sekin va xavfli (cast muvaffaqiyatsiz bo'lsa — bo'sh o'zgaruvchilar).
//
// To'g'ri yechim: C++ da AnimInstance yaratamiz, u NativeUpdateAnimation ichida
// bir marta personajni keshlaydi va o'zgaruvchilarni to'ldiradi. Anim Blueprint esa
// shu klassdan meros oladi va faqat State Machine bilan shug'ullanadi.
//
// Qamrab olingan epizodlar: #2 (blendspace, cho'kkalash), #25-26 (Foot IK),
// #30-31 (tirmashish), #47 (aim offset), #56 (suzish), #69 (blok).

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Core/ACRPGTypes.h"
#include "ACRPGAnimInstance.generated.h"

class AACRPGCharacterBase;
class UCharacterMovementComponent;

UCLASS()
class ACRPG_API UACRPGAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;

	/**
	 * ThreadSafe versiya — o'yin oqimini bloklamaydi.
	 * UE5 da og'ir hisob shu yerda bo'lishi kerak (Anim BP'da "Property Access" bilan).
	 */
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	// -----------------------------------------------------------------------
	// Ep.#2 — Blendspace uchun asosiy o'zgaruvchilar
	// -----------------------------------------------------------------------

	/** Gorizontal tezlik (sm/s). BS_Locomotion ning X o'qi. */
	UPROPERTY(BlueprintReadOnly, Category = "Harakat")
	float GroundSpeed = 0.f;

	/** Harakat yo'nalishi personaj oldiga nisbatan (-180..180). Strafe blendspace uchun. */
	UPROPERTY(BlueprintReadOnly, Category = "Harakat")
	float Direction = 0.f;

	/** Kirish bormi — to'xtash animatsiyasiga o'tish uchun. */
	UPROPERTY(BlueprintReadOnly, Category = "Harakat")
	bool bHasAcceleration = false;

	UPROPERTY(BlueprintReadOnly, Category = "Harakat")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Harakat")
	bool bIsCrouching = false;

	UPROPERTY(BlueprintReadOnly, Category = "Harakat")
	bool bIsSprinting = false;

	/** Vertikal tezlik — tushish animatsiyasi uchun. */
	UPROPERTY(BlueprintReadOnly, Category = "Harakat")
	float VerticalVelocity = 0.f;

	// -----------------------------------------------------------------------
	// Holat bayroqlari — State Machine shartlari
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "Holat")
	ECombatState CombatState = ECombatState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Holat")
	bool bIsClimbing = false;		// Ep.#30-31

	UPROPERTY(BlueprintReadOnly, Category = "Holat")
	bool bIsSwimming = false;		// Ep.#56

	UPROPERTY(BlueprintReadOnly, Category = "Holat")
	bool bIsBlocking = false;		// Ep.#69

	UPROPERTY(BlueprintReadOnly, Category = "Holat")
	bool bIsAiming = false;			// Ep.#47

	UPROPERTY(BlueprintReadOnly, Category = "Holat")
	bool bIsDead = false;

	/** Ep.#19 — qo'lda qilich bormi (idle pozasi shunga qarab o'zgaradi). */
	UPROPERTY(BlueprintReadOnly, Category = "Holat")
	bool bHasWeaponDrawn = false;

	// -----------------------------------------------------------------------
	// Ep.#47 — Aim Offset (kamon bilan yuqoriga/pastga qarash)
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "Aim")
	float AimPitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Aim")
	float AimYaw = 0.f;

	// -----------------------------------------------------------------------
	// Ep.#25-26 — Foot IK
	//
	// Har bir oyoq ostiga trace tashlaymiz va yerning balandligiga qarab
	// oyoqni ko'taramiz/tushiramiz. Anim BP da bu qiymatlar "Two Bone IK"
	// tugunlariga uzatiladi.
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "Foot IK")
	float LeftFootOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Foot IK")
	float RightFootOffset = 0.f;

	/** Ikkala oyoq ham yerga tegishi uchun tos suyagini pastga tushirish. */
	UPROPERTY(BlueprintReadOnly, Category = "Foot IK")
	float HipOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Foot IK")
	FRotator LeftFootRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Foot IK")
	FRotator RightFootRotation = FRotator::ZeroRotator;

	// --- Foot IK sozlamalari ---

	UPROPERTY(EditDefaultsOnly, Category = "Foot IK")
	FName LeftFootSocket = TEXT("foot_l");

	UPROPERTY(EditDefaultsOnly, Category = "Foot IK")
	FName RightFootSocket = TEXT("foot_r");

	/** Oyoq ostiga qancha pastga trace qilinadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Foot IK", meta = (ClampMin = "10"))
	float IKTraceDistance = 55.f;

	/** Silliqlik — 0 bo'lsa oyoq sakraydi, katta bo'lsa kechikadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Foot IK", meta = (ClampMin = "1"))
	float IKInterpSpeed = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Foot IK")
	bool bEnableFootIK = true;

private:
	/** Ep.#25 — bitta oyoq uchun trace. Qaytaradi: yer bilan oyoq orasidagi farq. */
	float TraceFoot(const FName& Socket, FRotator& OutRotation) const;

	/** Bu ikkisi keshlanadi — har kadr GetOwningActor() chaqirmaslik uchun. */
	UPROPERTY(Transient)
	TObjectPtr<AACRPGCharacterBase> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> MovementComponent;
};
