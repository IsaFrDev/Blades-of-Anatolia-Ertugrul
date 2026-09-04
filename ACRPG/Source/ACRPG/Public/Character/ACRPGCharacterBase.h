// ACRPGCharacterBase.h
//
// O'yinchi ham, dushman ham, tinch aholi ham, hayvon ham — hammasi shu klassdan meros oladi.
//
// Blueprint seriyasida BP_Player va BP_Enemy alohida qurilgan edi va "take damage",
// "hit reaction", "death" mantiqi ikkalasida ham qayta yozilgan. Bu takrorlanish
// (#11 va #60 epizodlarda aynan shu sabab bir necha bug chiqqan).
// C++ da bu mantiq bitta joyda — bazaviy klassda.
//
// Qamrab olingan epizodlar: #5-6 (stats/damage), #10-11 (hit reaction, qon VFX),
// #55 (qadam tovushlari), #60 (urish yaxshilanishi), #68 (o'lim).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/ACRPGTypes.h"
#include "ACRPGCharacterBase.generated.h"

class UACRPGStatsComponent;
class UACRPGCombatComponent;
class UMotionWarpingComponent;
class UNiagaraSystem;
class UAnimMontage;

UCLASS(Abstract)
class ACRPG_API AACRPGCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AACRPGCharacterBase();

	// --- Komponentlarga kirish ---

	UFUNCTION(BlueprintPure, Category = "ACRPG")
	UACRPGStatsComponent* GetStats() const { return StatsComponent; }

	UFUNCTION(BlueprintPure, Category = "ACRPG")
	UACRPGCombatComponent* GetCombat() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category = "ACRPG")
	UMotionWarpingComponent* GetMotionWarping() const { return MotionWarpingComponent; }

	// --- Holat ---

	UFUNCTION(BlueprintPure, Category = "ACRPG|Holat")
	ECombatState GetCombatState() const { return CombatState; }

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Holat")
	void SetCombatState(ECombatState NewState);

	UFUNCTION(BlueprintPure, Category = "ACRPG|Holat")
	bool IsAlive() const;

	/** Harakat qila oladimi? (o'lgan, zarba yegan, assassination'da bo'lsa — yo'q) */
	UFUNCTION(BlueprintPure, Category = "ACRPG|Holat")
	virtual bool CanMove() const;

	/** Yangi harakat boshlashi mumkinmi (hujum, sakrash, tirmashish...)? */
	UFUNCTION(BlueprintPure, Category = "ACRPG|Holat")
	virtual bool CanStartAction() const;

	// --- Urish (Ep.#6, #10, #11, #60) ---

	/**
	 * UE ning standart urish kanali. AnyDamage/PointDamage — hammasi shu yerga keladi.
	 * Blueprint'dagi "Event AnyDamage" ning aynan o'zi.
	 */
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	/** Ep.#4 — assassination bir zarbada o'ldiradi, hit reaction'siz. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Urish")
	virtual void ReceiveAssassination(AACRPGCharacterBase* Assassin);

	/** Ep.#10 — zarba yo'nalishini hisoblaydi (old/orqa/chap/o'ng). */
	UFUNCTION(BlueprintPure, Category = "ACRPG|Urish")
	EHitDirection GetHitDirectionFrom(const FVector& SourceLocation) const;

	// --- O'lim (Ep.#68) ---

	UFUNCTION(BlueprintCallable, Category = "ACRPG|O'lim")
	virtual void HandleDeath(AActor* Killer);

	/** Ep.#41 — kvest tizimi "kim o'ldi" ni bilishi uchun teg (masalan "Bandit"). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACRPG")
	FName CharacterTag = NAME_None;

protected:
	virtual void BeginPlay() override;

	// --- Komponentlar ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UACRPGStatsComponent> StatsComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UACRPGCombatComponent> CombatComponent;

	/** Ep.#3, #4 — vault va assassination'da nishonga aniq "yopishish" uchun. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	/** Ep.#55 — sirtga qarab qadam tovushi va chang effekti. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<class UACRPGFootstepComponent> FootstepComponent;

	// --- Animatsiyalar ---

	/** Ep.#10 — yo'nalish bo'yicha 4 ta hit reaction montaji. */
	UPROPERTY(EditDefaultsOnly, Category = "Animatsiya|Zarba")
	TMap<EHitDirection, TObjectPtr<UAnimMontage>> HitReactionMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Animatsiya|O'lim")
	TArray<TObjectPtr<UAnimMontage>> DeathMontages;

	/** Ep.#4 — assassination'da qurbon o'ynaydigan animatsiya. */
	UPROPERTY(EditDefaultsOnly, Category = "Animatsiya|O'lim")
	TObjectPtr<UAnimMontage> AssassinatedMontage;

	// --- VFX (Ep.#11) ---

	UPROPERTY(EditDefaultsOnly, Category = "Effekt")
	TObjectPtr<UNiagaraSystem> BloodEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effekt")
	TObjectPtr<USoundBase> HitSound;

	/** Ep.#68 — o'lgandan keyin tana necha soniya turadi (0 = o'chmasin). */
	UPROPERTY(EditDefaultsOnly, Category = "O'lim", meta = (ClampMin = "0"))
	float CorpseLifeSpan = 10.f;

	/** Zarba yeganda qisqa vaqt harakatsiz qolish (Ep.#10). */
	UPROPERTY(EditDefaultsOnly, Category = "Urish", meta = (ClampMin = "0"))
	float HitStunDuration = 0.4f;

	/** Holat o'zgarganda hosila klasslar reaksiya qilishi uchun. */
	virtual void OnCombatStateChanged(ECombatState OldState, ECombatState NewState);

	/** Ep.#11 — qon effekti va tovush. */
	virtual void PlayHitFeedback(const FVector& HitLocation, const FVector& HitNormal);

private:
	void PlayHitReaction(const FVector& SourceLocation);
	void ClearHitStun();

	UPROPERTY(VisibleInstanceOnly, Category = "Holat")
	ECombatState CombatState = ECombatState::Idle;

	FTimerHandle HitStunTimer;
};
