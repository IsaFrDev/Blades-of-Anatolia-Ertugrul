// ACRPGClimbingComponent.h — Ep.#30 (Climbing Movement), #31 (Climbing Animations),
//                             #32 (Ledge Climb / Mantling), #33 (bug fixes)
//
// Assassin's Creed uslubidagi devorga tirmashish — seriyaning eng murakkab qismi.
//
// ASOSIY G'OYA: tirmashish paytida UE ning oddiy yurish fizikasi ISHLAMAYDI.
// Personajni MOVE_Flying rejimiga o'tkazamiz va uni O'ZIMIZ devor yuzasiga
// "yopishtirib" turamiz. Har kadr:
//
//   1) Oldinga trace  -> devor hali bormi? Normal vektori qanday?
//   2) Personajni devordan ClimbDistance masofada ushlab turamiz
//   3) Uni devor normaliga qarama-qarshi buramiz (yuzi devorga qaragan)
//   4) Yuqoriga trace -> qirra topildimi? Topilsa mantling (Ep.#32)
//
// Ep.#33 dagi buglar aynan shu yerdan chiqqan edi: rejimni qaytarishni unutish,
// devor tugaganda tushib ketmaslik, qirradan o'tolmay qolish.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACRPGClimbingComponent.generated.h"

class AACRPGCharacterBase;
class UAnimMontage;
class UCharacterMovementComponent;

UCLASS(ClassGroup = (ACRPG), meta = (BlueprintSpawnableComponent))
class ACRPG_API UACRPGClimbingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACRPGClimbingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Ep.#30 — tirmashishni boshlashga urinadi.
	 * @return true — devor topildi va tirmashish boshlandi.
	 */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Tirmashish")
	bool TryStartClimb();

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Tirmashish")
	void StopClimb();

	UFUNCTION(BlueprintPure, Category = "ACRPG|Tirmashish")
	bool IsClimbing() const { return bIsClimbing; }

	/** Tirmashish paytidagi kirish (o'yinchi personajidan keladi). */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Tirmashish")
	void AddClimbInput(const FVector2D& Input);

	/** Ep.#32 — qirraga chiqish. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Tirmashish")
	bool TryMantle();

protected:
	virtual void BeginPlay() override;

	// --- Devor aniqlash ---

	UPROPERTY(EditDefaultsOnly, Category = "Tirmashish", meta = (ClampMin = "20"))
	float WallDetectionDistance = 80.f;

	/** Personaj devordan shu masofada turadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Tirmashish", meta = (ClampMin = "10"))
	float ClimbDistanceFromWall = 45.f;

	/**
	 * Devor qanchalik tik bo'lishi kerak.
	 * Normal.Z shu qiymatdan kichik bo'lsa — bu devor (tirmashsa bo'ladi).
	 * 0.3 ≈ 72 gradusdan tik.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Tirmashish", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxWallNormalZ = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Tirmashish", meta = (ClampMin = "10"))
	float ClimbSpeed = 120.f;

	/** Devorga burilish tezligi. */
	UPROPERTY(EditDefaultsOnly, Category = "Tirmashish", meta = (ClampMin = "1"))
	float WallAlignSpeed = 10.f;

	/** Ep.#6 — tirmashish chidamlilik yeydi (soniyasiga). */
	UPROPERTY(EditDefaultsOnly, Category = "Tirmashish", meta = (ClampMin = "0"))
	float ClimbStaminaDrain = 6.f;

	// --- Ep.#32: mantling ---

	/** Qirra qidirish uchun boshdan qancha yuqoriga trace. */
	UPROPERTY(EditDefaultsOnly, Category = "Mantling", meta = (ClampMin = "20"))
	float LedgeCheckHeight = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Mantling")
	TObjectPtr<UAnimMontage> MantleMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Mantling")
	FName MantleWarpTargetName = TEXT("MantleTarget");

	UPROPERTY(EditAnywhere, Category = "Tirmashish|Debug")
	bool bShowDebug = false;

private:
	/** Oldinda tirmashsa bo'ladigan devor bormi? */
	bool DetectWall(FHitResult& OutHit) const;

	/** Har kadr: devorga yopishib turish va burilish. */
	void UpdateClimbMovement(float DeltaTime);

	UFUNCTION()
	void OnMantleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(Transient) TObjectPtr<AACRPGCharacterBase> OwnerCharacter;
	UPROPERTY(Transient) TObjectPtr<UCharacterMovementComponent> MovementComponent;

	FVector CurrentWallNormal = FVector::ZeroVector;
	FVector2D ClimbInput = FVector2D::ZeroVector;

	bool bIsClimbing = false;
	bool bIsMantling = false;
};
