// ACRPGCombatComponent.h — Ep.#9 (Combat), #10 (Sword Trace Damage),
//                           #23 (AI combos), #27 (Throw distraction),
//                           #69-70 (Blocking), #75 (Attack orientation)
//
// UCH TA MUHIM G'OYA:
//
// 1) KOMBO + INPUT BUFFER (Ep.#9)
//    O'yinchi tugmani animatsiya tugashidan oldin bosadi. Agar biz uni shu zahoti
//    rad qilsak, jang "og'ir" his qilinadi. Yechim: bosishni ESLAB QOLAMIZ va
//    animatsiya "combo window" ga yetganda keyingi zarbani boshlaymiz.
//
// 2) QILICH TRACE'I (Ep.#10)
//    Collision Box emas, HAR KADR ikki socket orasida sweep qilamiz.
//    Sabab: 60 FPS da qilich bir kadrda 2 metr yo'l bosadi va Collision Box
//    dushmandan "sakrab o'tib ketadi". Sweep esa butun yo'lni tekshiradi.
//    Bir zarbada bir dushmanni ikki marta urmaslik uchun AlreadyHitActors ro'yxati.
//
// 3) ANIM NOTIFY ULANISHI
//    EnableWeaponTrace/DisableWeaponTrace va OpenComboWindow ni montajdagi
//    Anim Notify lar chaqiradi. Ya'ni "qachon urish o'tadi" ni animator hal qiladi,
//    dasturchi emas — bu to'g'ri mas'uliyat taqsimoti.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ACRPGTypes.h"
#include "ACRPGCombatComponent.generated.h"

class AACRPGCharacterBase;
class AACRPGWeaponBase;
class AACRPGThrowableActor;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamageDealt, AActor*, Victim, float, Damage);

UCLASS(ClassGroup = (ACRPG), meta = (BlueprintSpawnableComponent))
class ACRPG_API UACRPGCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACRPGCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event")
	FOnDamageDealt OnDamageDealt;

	// -----------------------------------------------------------------------
	// HUJUM (Ep.#9)
	// -----------------------------------------------------------------------

	/** Hujum tugmasi. Kombo holatini o'zi boshqaradi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Jang")
	void RequestAttack();

	/** Anim Notify: kombo oynasi ochildi — keyingi zarbani qabul qilish mumkin. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Jang")
	void OpenComboWindow();

	/** Anim Notify: kombo oynasi yopildi. Bosilmagan bo'lsa — kombo tugaydi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Jang")
	void CloseComboWindow();

	UFUNCTION(BlueprintPure, Category = "ACRPG|Jang")
	bool IsAttacking() const { return bIsAttacking; }

	// -----------------------------------------------------------------------
	// QILICH TRACE'I (Ep.#10)
	// -----------------------------------------------------------------------

	/** Anim Notify State BEGIN — zarba "o'tadigan" oyna boshlandi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Jang")
	void EnableWeaponTrace();

	/** Anim Notify State END. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Jang")
	void DisableWeaponTrace();

	// -----------------------------------------------------------------------
	// BLOK (Ep.#69-70)
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Blok") void StartBlocking();
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Blok") void StopBlocking();

	UFUNCTION(BlueprintPure, Category = "ACRPG|Blok")
	bool IsBlocking() const { return bIsBlocking; }

	/**
	 * Ep.#70 — kelayotgan urishni o'zgartiradi.
	 * Bloklangan bo'lsa va zarba OLDINDAN kelsa — kamayadi va chidamlilik yeydi.
	 * @return qolgan urish (0 bo'lsa — to'liq to'sildi)
	 */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Blok")
	float ModifyIncomingDamage(float IncomingDamage, const FVector& AttackerLocation);

	// -----------------------------------------------------------------------
	// CHALG'ITISH (Ep.#27)
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Chalg'itish")
	void ThrowDistraction();

protected:
	virtual void BeginPlay() override;

	// --- Kombo sozlamalari (Ep.#9, #23) ---

	/** Kombo zanjiri. Bo'sh bo'lsa hujum ishlamaydi. */
	UPROPERTY(EditDefaultsOnly, Category = "Kombo")
	TArray<FACRPGComboStep> ComboSteps;

	/** Qurolsiz bazaviy urish (qurol bo'lmasa ishlatiladi). */
	UPROPERTY(EditDefaultsOnly, Category = "Kombo", meta = (ClampMin = "0"))
	float UnarmedDamage = 10.f;

	// --- Trace sozlamalari (Ep.#10) ---

	/** Qilichning dastasi va uchi — qurol mesh'idagi socket nomlari. */
	UPROPERTY(EditDefaultsOnly, Category = "Trace") FName WeaponTraceStartSocket = TEXT("TraceStart");
	UPROPERTY(EditDefaultsOnly, Category = "Trace") FName WeaponTraceEndSocket = TEXT("TraceEnd");

	UPROPERTY(EditDefaultsOnly, Category = "Trace", meta = (ClampMin = "1"))
	float WeaponTraceRadius = 12.f;

	UPROPERTY(EditAnywhere, Category = "Trace|Debug")
	bool bShowTraceDebug = false;

	// --- Blok sozlamalari (Ep.#69-70) ---

	/** Bloklanganda urishning necha foizi o'tadi (0.2 = 20%). */
	UPROPERTY(EditDefaultsOnly, Category = "Blok", meta = (ClampMin = "0", ClampMax = "1"))
	float BlockDamageMultiplier = 0.2f;

	/** Blok qilingan har bir zarba shuncha chidamlilik yeydi. */
	UPROPERTY(EditDefaultsOnly, Category = "Blok", meta = (ClampMin = "0"))
	float BlockStaminaCost = 15.f;

	/** Bloklanadigan burchak (gradus, oldindan). 120 = old yarim doiraga yaqin. */
	UPROPERTY(EditDefaultsOnly, Category = "Blok", meta = (ClampMin = "10", ClampMax = "180"))
	float BlockAngleDegrees = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Blok")
	TObjectPtr<UAnimMontage> BlockImpactMontage;

	// --- Ep.#75: hujum yo'nalishi ---

	/** Hujum boshlanganda eng yaqin dushmanga burilish. */
	UPROPERTY(EditDefaultsOnly, Category = "Hujum yo'nalishi")
	bool bOrientToNearestEnemy = true;

	UPROPERTY(EditDefaultsOnly, Category = "Hujum yo'nalishi", meta = (ClampMin = "50"))
	float OrientSearchRadius = 400.f;

	// --- Ep.#27: otiladigan buyum ---

	UPROPERTY(EditDefaultsOnly, Category = "Chalg'itish")
	TSubclassOf<AACRPGThrowableActor> ThrowableClass;

	UPROPERTY(EditDefaultsOnly, Category = "Chalg'itish")
	TObjectPtr<UAnimMontage> ThrowMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Chalg'itish", meta = (ClampMin = "100"))
	float ThrowSpeed = 1200.f;

private:
	void StartComboStep(int32 StepIndex);
	void PerformWeaponTrace();
	void OrientTowardsNearestEnemy();

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Zarba o'tadigan qurolni topadi (qo'ldagi qilich yoki mesh). */
	USceneComponent* GetTraceSourceComponent() const;

	UPROPERTY(Transient)
	TObjectPtr<AACRPGCharacterBase> OwnerCharacter;

	/** Bir zarbada bir dushman bir marta uriladi (Ep.#10 dagi asosiy bug). */
	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> AlreadyHitActors;

	/** Oldingi kadrdagi trace pozitsiyalari — uzluksiz sweep uchun. */
	FVector PrevTraceStart = FVector::ZeroVector;
	FVector PrevTraceEnd = FVector::ZeroVector;
	bool bHasPrevTrace = false;

	int32 CurrentComboIndex = -1;
	bool bIsAttacking = false;
	bool bComboWindowOpen = false;
	bool bInputBuffered = false;		// Ep.#9 — "keyingisini ham xohladi"
	bool bWeaponTraceActive = false;
	bool bIsBlocking = false;
};
