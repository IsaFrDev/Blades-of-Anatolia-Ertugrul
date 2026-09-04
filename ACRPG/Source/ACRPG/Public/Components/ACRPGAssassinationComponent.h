// ACRPGAssassinationComponent.h — Ep.#4 "Assassinations"
//
// Assassin's Creed uslubidagi asosiy mexanika: dushmanning orqasidan sezdirmay
// yaqinlashib, bir zarbada yo'q qilish.
//
// To'g'ri ishlashi uchun UCHTA shart bir vaqtda bajarilishi kerak:
//   1) MASOFA   — dushman yetarlicha yaqinmi;
//   2) BURCHAK  — biz uning ORQASIDAmizmi (oldidan bo'lsa — oddiy jang);
//   3) SEZGIRLIK— dushman bizni hali ko'rmaganmi (Ep.#21 dagi AI holati).
//
// Uchinchi shart Blueprint versiyasida yo'q edi va shuning uchun jang o'rtasida
// ham assassination qilish mumkin bo'lib qolgandi. Bu yerda uni tuzatamiz.
//
// Motion Warping (Ep.#3 dagi kabi) ikkala personajni bir-biriga aniq joylashtiradi —
// aks holda "havoni kesish" animatsiyasi chiqadi.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACRPGAssassinationComponent.generated.h"

class AACRPGCharacterBase;
class UAnimMontage;

UCLASS(ClassGroup = (ACRPG), meta = (BlueprintSpawnableComponent))
class ACRPG_API UACRPGAssassinationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACRPGAssassinationComponent();

	/**
	 * Hujum tugmasi bosilganda, oddiy hujumdan OLDIN chaqiriladi.
	 * @return true — assassination boshlandi, oddiy hujum bekor qilinsin.
	 */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Assassination")
	bool TryAssassinate();

	/** UI ga "E — Assassinate" belgisini ko'rsatish uchun (Ep.#8). */
	UFUNCTION(BlueprintPure, Category = "ACRPG|Assassination")
	AACRPGCharacterBase* FindAssassinationTarget() const;

	UFUNCTION(BlueprintPure, Category = "ACRPG|Assassination")
	bool IsAssassinating() const { return bIsAssassinating; }

protected:
	virtual void BeginPlay() override;

	/** Dushman shu masofadan yaqin bo'lishi kerak. */
	UPROPERTY(EditDefaultsOnly, Category = "Assassination", meta = (ClampMin = "50"))
	float MaxRange = 200.f;

	/**
	 * Orqa tomon konusi (gradus). 90 = dushmanning orqa yarim doirasi.
	 * Kichikroq qiymat (60) — faqat tik orqadan; qiyinroq, lekin adolatliroq.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Assassination", meta = (ClampMin = "10", ClampMax = "180"))
	float BehindAngleDegrees = 90.f;

	/** Assassination'dan keyin personaj dushmanning orqasida shu masofada turadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Assassination", meta = (ClampMin = "40"))
	float WarpDistanceBehind = 90.f;

	/** Hujumchi (o'yinchi) o'ynaydigan animatsiya. */
	UPROPERTY(EditDefaultsOnly, Category = "Assassination")
	TObjectPtr<UAnimMontage> AssassinMontage;

	/** Montajdagi Motion Warping target nomi. */
	UPROPERTY(EditDefaultsOnly, Category = "Assassination")
	FName WarpTargetName = TEXT("AssassinationTarget");

	/** Ep.#2 — cho'kkalab turganda assassination osonroq bo'lsin (masofa kengayadi). */
	UPROPERTY(EditDefaultsOnly, Category = "Assassination", meta = (ClampMin = "1"))
	float CrouchedRangeMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Assassination|Debug")
	bool bShowDebug = false;

private:
	UFUNCTION()
	void OnAssassinationEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Bitta nomzod barcha shartlarni qanoatlantiradimi? */
	bool IsValidTarget(const AACRPGCharacterBase* Candidate) const;

	UPROPERTY(Transient)
	TObjectPtr<AACRPGCharacterBase> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<AACRPGCharacterBase> CurrentVictim;

	bool bIsAssassinating = false;
};
