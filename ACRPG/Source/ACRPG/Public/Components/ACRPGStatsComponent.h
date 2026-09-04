// ACRPGStatsComponent.h — Ep.#5 (Player Stats), #6 (Damage & Stamina),
//                          #7 (Levels & XP), #44 (Armor), #60 (Damage enhancements),
//                          #65 (XP rewards), #67 (Level up animation), #77 (Level up VFX)
//
// Sog'liq, chidamlilik va tajriba — uchalasi ham "hozirgi/maksimal" juftligi.
// Shuning uchun ularni bitta komponentga jamlaymiz.
//
// MUHIM QOIDA: bu komponent HECH QACHON UI ni to'g'ridan-to'g'ri o'zgartirmaydi.
// U faqat delegate (event) chiqaradi, UI esa o'zi obuna bo'ladi. Blueprint'da
// "Cast to WBP_HUD -> Set Health Bar" qilingan edi — bu HUD'ni personajga bog'lab
// qo'yadi va HUD o'zgarsa personaj ham buziladi.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACRPGStatsComponent.generated.h"

// --- Delegate'lar: UI va boshqa tizimlar shularga obuna bo'ladi ---

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, NewStamina, float, MaxStamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnXPChanged, int32, CurrentXP, int32, XPToNextLevel, int32, Level);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelUp, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeath, AActor*, Killer);

UCLASS(ClassGroup = (ACRPG), meta = (BlueprintSpawnableComponent))
class ACRPG_API UACRPGStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACRPGStatsComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// -----------------------------------------------------------------------
	// Event'lar (Ep.#8 — HUD shularga ulanadi)
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event") FOnHealthChanged OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event") FOnStaminaChanged OnStaminaChanged;
	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event") FOnXPChanged OnXPChanged;
	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event") FOnLevelUp OnLevelUp;
	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event") FOnDeath OnDeath;

	// -----------------------------------------------------------------------
	// SOG'LIQ (Ep.#5, #6, #44, #60)
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "ACRPG|Stats") float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "ACRPG|Stats") float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure, Category = "ACRPG|Stats") float GetHealthPercent() const;

	/**
	 * Urishni qo'llaydi. Zirh himoyasi SHU YERDA ayiriladi (Ep.#44).
	 * @param RawDamage — zirhgacha bo'lgan urish
	 */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Stats")
	void ApplyDamage(float RawDamage, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Stats")
	void Heal(float Amount);

	/** Ep.#44-45 — kiyilgan zirh yig'indisi. EquipmentComponent yangilaydi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Stats")
	void SetArmorValue(float NewArmor) { ArmorValue = FMath::Max(0.f, NewArmor); }

	UFUNCTION(BlueprintPure, Category = "ACRPG|Stats") float GetArmorValue() const { return ArmorValue; }

	// -----------------------------------------------------------------------
	// CHIDAMLILIK (Ep.#6)
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "ACRPG|Stats") float GetStamina() const { return Stamina; }
	UFUNCTION(BlueprintPure, Category = "ACRPG|Stats") float GetStaminaPercent() const;

	/** @return true — yetarli chidamlilik bor edi va sarflandi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Stats")
	bool ConsumeStamina(float Amount);

	UFUNCTION(BlueprintPure, Category = "ACRPG|Stats")
	bool HasStamina(float Amount) const { return Stamina >= Amount; }

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Stats")
	void SetSprinting(bool bNewSprinting);

	UFUNCTION(BlueprintPure, Category = "ACRPG|Stats")
	bool IsSprinting() const { return bIsSprinting; }

	// -----------------------------------------------------------------------
	// TAJRIBA VA DARAJA (Ep.#7, #65, #67)
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "ACRPG|Stats") int32 GetLevel() const { return Level; }
	UFUNCTION(BlueprintPure, Category = "ACRPG|Stats") int32 GetCurrentXP() const { return CurrentXP; }

	/** Shu darajadan keyingisiga o'tish uchun kerakli umumiy XP. */
	UFUNCTION(BlueprintPure, Category = "ACRPG|Stats")
	int32 GetXPToNextLevel() const { return CalculateXPForLevel(Level + 1); }

	/** Ep.#65 — dushman o'ldirilganda yoki kvest bajarilganda chaqiriladi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Stats")
	void AddXP(int32 Amount);

	/** Ep.#7 — daraja formulasi. Editor'da egri chiziq (Curve) bilan ham almashtirish mumkin. */
	UFUNCTION(BlueprintPure, Category = "ACRPG|Stats")
	int32 CalculateXPForLevel(int32 TargetLevel) const;

	// --- Ep.#80: saqlash/yuklash ---

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Save")
	void LoadFromSave(float InHealth, float InStamina, int32 InLevel, int32 InXP);

protected:
	virtual void BeginPlay() override;

	// --- Sog'liq ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, SaveGame, Category = "Sog'liq", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Sog'liq")
	float Health = 100.f;

	/** Ep.#60 — jangdan chiqqach sekin tiklanish (0 = tiklanmasin). */
	UPROPERTY(EditDefaultsOnly, Category = "Sog'liq", meta = (ClampMin = "0"))
	float HealthRegenRate = 2.f;

	/** Oxirgi zarbadan keyin necha soniya kutib tiklanish boshlanadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Sog'liq", meta = (ClampMin = "0"))
	float HealthRegenDelay = 5.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sog'liq")
	float ArmorValue = 0.f;

	/**
	 * Ep.#44 — zirh formulasi.
	 * Ayirish emas, foizli kamaytirish ishlatamiz: Damage * (100 / (100 + Armor)).
	 * Bu 100 zirhda urishni yarmiga tushiradi va hech qachon 0 ga olib bormaydi —
	 * ya'ni o'yinchi "o'lmas" bo'lib qolmaydi.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Sog'liq", meta = (ClampMin = "1"))
	float ArmorConstant = 100.f;

	// --- Chidamlilik (Ep.#6) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, SaveGame, Category = "Chidamlilik", meta = (ClampMin = "1"))
	float MaxStamina = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Chidamlilik")
	float Stamina = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Chidamlilik", meta = (ClampMin = "0"))
	float StaminaRegenRate = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Chidamlilik", meta = (ClampMin = "0"))
	float StaminaRegenDelay = 1.5f;

	/** Yugurganda soniyasiga qancha sarflanadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Chidamlilik", meta = (ClampMin = "0"))
	float SprintStaminaDrain = 10.f;

	// --- Daraja (Ep.#7) ---
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Daraja", meta = (ClampMin = "1"))
	int32 Level = 1;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Daraja")
	int32 CurrentXP = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Daraja", meta = (ClampMin = "1"))
	int32 MaxLevel = 50;

	/** Ep.#7 — XP formulasi: BaseXP * (Level ^ Exponent). */
	UPROPERTY(EditDefaultsOnly, Category = "Daraja", meta = (ClampMin = "1"))
	int32 BaseXPPerLevel = 100;

	UPROPERTY(EditDefaultsOnly, Category = "Daraja", meta = (ClampMin = "1.0", ClampMax = "3.0"))
	float XPCurveExponent = 1.5f;

	/** Ep.#67 — har darajada maksimal sog'liq shuncha oshadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Daraja", meta = (ClampMin = "0"))
	float HealthPerLevel = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Daraja", meta = (ClampMin = "0"))
	float StaminaPerLevel = 5.f;

private:
	void HandleRegeneration(float DeltaTime);
	void CheckLevelUp();

	bool bIsSprinting = false;
	float TimeSinceLastDamage = 0.f;
	float TimeSinceLastStaminaUse = 0.f;
};
